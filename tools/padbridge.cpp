// Ponte de controles: lê os controles deste computador com o SDL e manda o estado de cada um,
// por TCP, para o jogo em outro lugar (no WSL, em outra máquina), onde eles viram controles
// virtuais (src/input/netpad.*). Ver CONTROLES.md.
//
// O uso principal é o Windows com o jogo no WSL: o Windows enxerga os controles Bluetooth, o
// WSL não. O play.cmd abre esta ponte em segundo plano antes do jogo.
//
//   padbridge                      conecta em 127.0.0.1:47990 e tenta de novo até conseguir
//   padbridge --host H --port P    outro endereço
//   padbridge --once               sai quando o jogo fechar a conexão
//   padbridge --wait 60            desiste se o jogo não aparecer em 60 s
//   padbridge --quiet              sem mensagens (para rodar em segundo plano)
//   padbridge --fake               um controle de mentira (testes): aperta o A e mexe o analógico
//   padbridge --list               lista os controles que o SDL vê e sai

#include "../src/input/pad_info.h"
#include "../src/input/pad_protocol.h"
#include "../src/input/pad_socket.h"

#include <SDL2/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

namespace
{
    struct Options
    {
        std::string host = "127.0.0.1";
        int port = PadProtocol::DEFAULT_PORT;
        int wait = 0;
        bool once = false;
        bool quiet = false;
        bool fake = false;
        bool list = false;
    };

    // Reenvia o estado de todos a cada 250 ms mesmo sem mudança: o jogo sabe que a ponte está viva
    const Uint32 KEEPALIVE = 250;
    // Tempo entre leituras: ~250 Hz, mais rápido que qualquer quadro do jogo
    const Uint32 TICK = 4;

    bool g_quiet = false;
    void say(const std::string& text)
    {
        if(!g_quiet) std::printf("padbridge: %s\n", text.c_str());
        std::fflush(stdout);
    }

    struct Slot
    {
        SDL_GameController* pad = nullptr;
        SDL_JoystickID id = -1;
        PadInfo::Device info;
        PadProtocol::State sent;
        bool sent_once = false;
    };

    Slot g_slots[PadProtocol::MAX_PADS];

    int slotOf(SDL_JoystickID id)
    {
        for(int i = 0; i < PadProtocol::MAX_PADS; i++)
            if(g_slots[i].pad != nullptr && g_slots[i].id == id) return i;
        return -1;
    }

    PadProtocol::State read(SDL_GameController* pad)
    {
        PadProtocol::State s;
        for(int b = 0; b < SDL_CONTROLLER_BUTTON_MAX && b < 32; b++)
            if(SDL_GameControllerGetButton(pad, static_cast<SDL_GameControllerButton>(b)))
                s.buttons |= (1u << b);
        for(int a = 0; a < PadProtocol::AXES; a++)
            s.axes[a] = SDL_GameControllerGetAxis(pad, static_cast<SDL_GameControllerAxis>(a));
        return s;
    }

    // Controle de mentira para testes: A apertado em pulsos de 300 ms, analógico esquerdo
    // para a direita enquanto o A está apertado
    PadProtocol::State fakeState(Uint32 now)
    {
        PadProtocol::State s;
        bool pressed = (now / 300) % 2 == 0;
        if(pressed) s.buttons |= (1u << SDL_CONTROLLER_BUTTON_A);
        s.axes[SDL_CONTROLLER_AXIS_LEFTX] = pressed ? 30000 : 0;
        s.axes[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] = pressed ? 32767 : 0;
        return s;
    }

    bool sendText(PadSocket::Handle& sock, const std::string& data)
    {
        if(sock == PadSocket::INVALID) return false;
        if(PadSocket::sendAll(sock, data.data(), data.size())) return true;
        PadSocket::close(sock);
        sock = PadSocket::INVALID;
        return false;
    }

    void openPad(int device_index, PadSocket::Handle& sock)
    {
        if(!SDL_IsGameController(device_index)) return;
        SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(device_index);
        if(slotOf(id) >= 0) return;
        for(int i = 0; i < PadProtocol::MAX_PADS; i++)
        {
            if(g_slots[i].pad != nullptr) continue;
            SDL_GameController* pad = SDL_GameControllerOpen(device_index);
            if(pad == nullptr) return;
            g_slots[i].pad = pad;
            g_slots[i].id = id;
            g_slots[i].info = PadInfo::describe(device_index);
            g_slots[i].sent_once = false;
            say("controle " + std::to_string(i) + ": " + PadInfo::summary(g_slots[i].info));
            const PadInfo::Device& d = g_slots[i].info;
            sendText(sock, PadProtocol::attach(static_cast<Uint8>(i), d.bus, d.vendor, d.product, d.name));
            return;
        }
    }

    void closePad(SDL_JoystickID id, PadSocket::Handle& sock)
    {
        int i = slotOf(id);
        if(i < 0) return;
        SDL_GameControllerClose(g_slots[i].pad);
        g_slots[i].pad = nullptr;
        say("controle " + std::to_string(i) + " desconectado");
        sendText(sock, PadProtocol::detach(static_cast<Uint8>(i)));
    }

    // Conexão nova: apresenta-se e conta quais controles já estão abertos
    bool greet(PadSocket::Handle& sock, const Options& opt)
    {
        if(!sendText(sock, PadProtocol::hello())) return false;
        if(opt.fake)
            return sendText(sock, PadProtocol::attach(0, PadInfo::BUS_VIRTUAL, 0, 0, "Fake Pad"));
        for(int i = 0; i < PadProtocol::MAX_PADS; i++)
        {
            Slot& s = g_slots[i];
            if(s.pad == nullptr) continue;
            s.sent_once = false;
            if(!sendText(sock, PadProtocol::attach(static_cast<Uint8>(i), s.info.bus, s.info.vendor, s.info.product, s.info.name)))
                return false;
        }
        return true;
    }

    int listDevices()
    {
        int n = SDL_NumJoysticks();
        std::printf("%d dispositivo(s):\n", n);
        for(int i = 0; i < n; i++)
        {
            PadInfo::Device d = PadInfo::describe(i);
            std::printf("  %d. %s%s\n", i, PadInfo::summary(d).c_str(), d.gamepad ? "" : "  (sem mapeamento de gamepad)");
        }
        return 0;
    }

    bool parse(int argc, char** argv, Options& opt)
    {
        for(int i = 1; i < argc; i++)
        {
            std::string a = argv[i];
            auto value = [&](const char* what) -> const char* {
                if(i + 1 >= argc) { std::fprintf(stderr, "padbridge: falta o valor de %s\n", what); std::exit(2); }
                return argv[++i];
            };
            if(a == "--host") opt.host = value("--host");
            else if(a == "--port") opt.port = std::atoi(value("--port"));
            else if(a == "--wait") opt.wait = std::atoi(value("--wait"));
            else if(a == "--once") opt.once = true;
            else if(a == "--quiet") opt.quiet = true;
            else if(a == "--fake") opt.fake = true;
            else if(a == "--list") opt.list = true;
            else
            {
                std::fprintf(stderr, "uso: padbridge [--host H] [--port P] [--wait S] [--once] [--quiet] [--fake] [--list]\n");
                return false;
            }
        }
        return opt.port > 0;
    }
}

int main(int argc, char** argv)
{
    Options opt;
    if(!parse(argc, argv, opt)) return 2;
    g_quiet = opt.quiet;

    // A ponte roda em segundo plano, sem janela: sem esta dica o SDL só entregaria os
    // controles à janela em foco (que é a do jogo)
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if(SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0)
    {
        std::fprintf(stderr, "padbridge: SDL_Init falhou: %s\n", SDL_GetError());
        return 1;
    }
    if(opt.list)
    {
        int rc = listDevices();
        SDL_Quit();
        return rc;
    }
    if(!PadSocket::startup())
    {
        std::fprintf(stderr, "padbridge: a rede não iniciou\n");
        return 1;
    }

    PadSocket::Handle sock = PadSocket::INVALID;
    bool ever_connected = false, running = true;
    Uint32 started = SDL_GetTicks(), last_try = 0, last_keepalive = 0;
    PadProtocol::State fake_sent;
    int rc = 0;

    say("enviando para " + opt.host + ":" + std::to_string(opt.port) + (opt.fake ? " (controle de mentira)" : ""));
    while(running)
    {
        SDL_Event e;
        if(SDL_WaitEventTimeout(&e, static_cast<int>(TICK)))
        {
            do
            {
                if(e.type == SDL_QUIT) running = false;
                else if(e.type == SDL_CONTROLLERDEVICEADDED && !opt.fake) openPad(e.cdevice.which, sock);
                else if(e.type == SDL_CONTROLLERDEVICEREMOVED && !opt.fake) closePad(e.cdevice.which, sock);
            } while(SDL_PollEvent(&e));
        }
        Uint32 now = SDL_GetTicks();

        // Sem conexão: tenta de novo a cada 500 ms (o jogo pode ainda estar abrindo)
        if(sock == PadSocket::INVALID)
        {
            if(ever_connected && opt.once) break;
            if(opt.wait > 0 && !ever_connected && now - started > static_cast<Uint32>(opt.wait) * 1000)
            {
                say("o jogo não apareceu em " + std::to_string(opt.wait) + " s; saindo");
                rc = 1;
                break;
            }
            if(now - last_try < 500) continue;
            last_try = now;
            sock = PadSocket::connectTo(opt.host.c_str(), opt.port);
            if(sock == PadSocket::INVALID) continue;
            if(!greet(sock, opt)) continue;
            if(!ever_connected) say("conectado ao jogo");
            ever_connected = true;
            last_keepalive = 0;
            fake_sent = PadProtocol::State();
            fake_sent.buttons = ~0u; // força o primeiro envio
        }

        bool keepalive = now - last_keepalive >= KEEPALIVE;
        if(keepalive) last_keepalive = now;
        if(opt.fake)
        {
            PadProtocol::State s = fakeState(now - started);
            if(s != fake_sent || keepalive)
            {
                sendText(sock, PadProtocol::state(0, s));
                fake_sent = s;
            }
            continue;
        }
        for(int i = 0; i < PadProtocol::MAX_PADS && sock != PadSocket::INVALID; i++)
        {
            Slot& slot = g_slots[i];
            if(slot.pad == nullptr) continue;
            PadProtocol::State s = read(slot.pad);
            if(!slot.sent_once || s != slot.sent || keepalive)
            {
                if(!sendText(sock, PadProtocol::state(static_cast<Uint8>(i), s))) break;
                slot.sent = s;
                slot.sent_once = true;
            }
        }
        if(sock == PadSocket::INVALID) say("o jogo fechou a conexão");
    }

    PadSocket::close(sock);
    for(Slot& s : g_slots)
        if(s.pad != nullptr) SDL_GameControllerClose(s.pad);
    SDL_Quit();
    return rc;
}
