// Diagnóstico de controles: mostra o que o SDL enxerga neste computador (nome, barramento
// USB/Bluetooth/virtual, VID:PID, GUID, se tem mapeamento de gamepad) e os eventos ao vivo.
// Serve para responder "o meu controle Bluetooth chegou até o jogo?" em qualquer sistema.
// Ver CONTROLES.md.
//
//   padprobe                     lista e mostra os eventos (Ctrl+C sai)
//   padprobe --list              só lista
//   padprobe --seconds N         mostra os eventos por N segundos
//   padprobe --netpad PORTA      também recebe controles da ponte (padbridge) nesta porta
//   padprobe --selftest PORTA    teste de ponta a ponta: espera o controle de mentira da ponte
//                                (padbridge --fake), confere o botão A, o analógico e o gatilho
//                                e a desconexão. Sai com 0 se tudo chegou.

#include "../src/input/netpad.h"
#include "../src/input/pad_info.h"

#include <SDL2/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{
    void list()
    {
        int n = SDL_NumJoysticks();
        std::printf("%d dispositivo(s):\n", n);
        for(int i = 0; i < n; i++)
        {
            PadInfo::Device d = PadInfo::describe(i);
            std::printf("  %d. %s\n", i, PadInfo::summary(d).c_str());
            std::printf("     GUID %s  %s%s\n", d.guid.c_str(),
                        d.gamepad ? "gamepad (mapeado)" : "joystick sem mapeamento de gamepad",
                        d.is_virtual ? ", virtual" : "");
#if SDL_VERSION_ATLEAST(2, 24, 0)
            const char* path = SDL_JoystickPathForIndex(i);
            if(path != nullptr) std::printf("     caminho %s\n", path);
#endif
        }
        std::fflush(stdout);
    }

    const char* buttonName(int b)
    {
        const char* name = SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(b));
        return name ? name : "?";
    }

    const char* axisName(int a)
    {
        const char* name = SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(a));
        return name ? name : "?";
    }

    // Teste de ponta a ponta: ponte (--fake) → TCP → NetPad → joystick virtual → evento de gamepad
    int selftest(int port, int seconds)
    {
        if(!NetPad::supported())
        {
            std::printf("selftest: SDL %d.%d.%d sem joysticks virtuais (precisa do 2.24+)\n",
                        SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
            return 1;
        }
        if(!NetPad::start("127.0.0.1", port)) return 1;

        SDL_GameController* pad = nullptr;
        bool a_down = false, a_up = false, stick = false, trigger = false, removed = false;
        Uint32 deadline = SDL_GetTicks() + static_cast<Uint32>(seconds) * 1000;
        while(SDL_GetTicks() < deadline && !removed)
        {
            NetPad::poll();
            SDL_Event e;
            while(SDL_PollEvent(&e))
            {
                if(e.type == SDL_CONTROLLERDEVICEADDED && pad == nullptr)
                {
                    PadInfo::Device d = PadInfo::describe(e.cdevice.which);
                    std::printf("selftest: chegou %s (%s)\n", PadInfo::summary(d).c_str(), d.gamepad ? "gamepad" : "sem mapeamento");
                    if(d.is_virtual && d.gamepad) pad = SDL_GameControllerOpen(e.cdevice.which);
                }
                else if(e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_A) a_down = true;
                else if(e.type == SDL_CONTROLLERBUTTONUP && e.cbutton.button == SDL_CONTROLLER_BUTTON_A && a_down) a_up = true;
                else if(e.type == SDL_CONTROLLERAXISMOTION)
                {
                    if(e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX && e.caxis.value > 20000) stick = true;
                    if(e.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT && e.caxis.value > 20000) trigger = true;
                }
                else if(e.type == SDL_CONTROLLERDEVICEREMOVED && pad != nullptr &&
                        e.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad)))
                    removed = true;
            }
            // Tudo conferido: fecha a porta, o que tem de soltar o controle virtual
            if(pad != nullptr && a_down && a_up && stick && trigger && NetPad::listening())
                NetPad::stop();
            SDL_Delay(5);
        }
        if(pad != nullptr) SDL_GameControllerClose(pad);
        NetPad::stop();

        std::printf("selftest: controle %s, A apertado %s, A solto %s, analógico %s, gatilho %s, desconexão %s\n",
                    pad ? "ok" : "NÃO", a_down ? "ok" : "NÃO", a_up ? "ok" : "NÃO", stick ? "ok" : "NÃO",
                    trigger ? "ok" : "NÃO", removed ? "ok" : "NÃO");
        bool ok = pad && a_down && a_up && stick && trigger && removed;
        std::printf("selftest: %s\n", ok ? "PASSOU" : "FALHOU");
        return ok ? 0 : 1;
    }
}

int main(int argc, char** argv)
{
    bool only_list = false;
    int seconds = 0, netpad = 0, test_port = 0;
    for(int i = 1; i < argc; i++)
    {
        std::string a = argv[i];
        if(a == "--list") only_list = true;
        else if(a == "--seconds" && i + 1 < argc) seconds = std::atoi(argv[++i]);
        else if(a == "--netpad" && i + 1 < argc) netpad = std::atoi(argv[++i]);
        else if(a == "--selftest" && i + 1 < argc) test_port = std::atoi(argv[++i]);
        else
        {
            std::fprintf(stderr, "uso: padprobe [--list] [--seconds N] [--netpad PORTA] [--selftest PORTA]\n");
            return 2;
        }
    }

    // Sem janela: os eventos dos controles chegam mesmo com outra janela em foco
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if(SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0)
    {
        std::fprintf(stderr, "padprobe: SDL_Init falhou: %s\n", SDL_GetError());
        return 1;
    }
    SDL_version linked;
    SDL_GetVersion(&linked);
    std::printf("SDL %d.%d.%d, plataforma %s\n", linked.major, linked.minor, linked.patch, SDL_GetPlatform());

    if(test_port > 0)
    {
        int rc = selftest(test_port, seconds > 0 ? seconds : 20);
        SDL_Quit();
        return rc;
    }

    list();
    if(only_list)
    {
        SDL_Quit();
        return 0;
    }
    if(netpad > 0) NetPad::start("127.0.0.1", netpad);

    std::printf("Eventos (Ctrl+C sai):\n");
    std::fflush(stdout);
    Uint32 deadline = seconds > 0 ? SDL_GetTicks() + static_cast<Uint32>(seconds) * 1000 : 0;
    bool running = true;
    while(running && (deadline == 0 || SDL_GetTicks() < deadline))
    {
        NetPad::poll();
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            switch(e.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_CONTROLLERDEVICEADDED:
                if(SDL_GameControllerOpen(e.cdevice.which))
                    std::printf("+ conectado: %s\n", PadInfo::summary(PadInfo::describe(e.cdevice.which)).c_str());
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                std::printf("- desconectado (instância %d)\n", e.cdevice.which);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                std::printf("  [%d] botão %-12s %s\n", e.cbutton.which, buttonName(e.cbutton.button),
                            e.type == SDL_CONTROLLERBUTTONDOWN ? "apertado" : "solto");
                break;
            case SDL_CONTROLLERAXISMOTION:
                // Só movimentos grandes: o analógico em repouso oscila um pouco
                if(e.caxis.value > 8000 || e.caxis.value < -8000)
                    std::printf("  [%d] eixo  %-12s %6d\n", e.caxis.which, axisName(e.caxis.axis), e.caxis.value);
                break;
            case SDL_JOYDEVICEADDED:
                if(!SDL_IsGameController(e.jdevice.which))
                    std::printf("+ joystick sem mapeamento de gamepad: %s\n",
                                PadInfo::summary(PadInfo::describe(e.jdevice.which)).c_str());
                break;
            default:
                break;
            }
            std::fflush(stdout);
        }
        SDL_Delay(5);
    }
    NetPad::stop();
    SDL_Quit();
    return 0;
}
