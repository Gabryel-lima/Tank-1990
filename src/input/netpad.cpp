#include "netpad.h"
#include "pad_info.h"

#include <cstdlib>
#include <iostream>

PadSocket::Handle NetPad::s_listener = PadSocket::INVALID;
PadSocket::Handle NetPad::s_client = PadSocket::INVALID;
bool NetPad::s_greeted = false;
Uint32 NetPad::s_last_data = 0;
PadProtocol::Reader NetPad::s_reader;
NetPad::Pad NetPad::s_pads[PadProtocol::MAX_PADS];

namespace
{
    // A ponte manda o estado de todos os controles pelo menos a cada 250 ms. Sem nada por
    // este tempo, a ponte travou ou a máquina dela dormiu: os controles remotos saem
    const Uint32 SILENCE_LIMIT = 5000;
}

bool NetPad::supported()
{
#if SDL_VERSION_ATLEAST(2, 24, 0)
    return true;
#else
    return false;
#endif
}

bool NetPad::start(const char* host, int port)
{
    if(!supported())
    {
        std::cerr << "NetPad: este SDL não tem joysticks virtuais com mapeamento (precisa do 2.24+)\n";
        return false;
    }
    stop();
    s_listener = PadSocket::listenOn(host, port);
    if(s_listener == PadSocket::INVALID)
    {
        std::cerr << "NetPad: não foi possível escutar em " << host << ":" << port << "\n";
        return false;
    }
    std::cout << "NetPad: esperando a ponte de controles em " << host << ":" << port << std::endl;
    return true;
}

bool NetPad::startFromEnvironment()
{
    const char* port = std::getenv("TANK_NETPAD");
    if(port == nullptr || std::atoi(port) <= 0) return false;
    const char* host = std::getenv("TANK_NETPAD_BIND");
    return start(host != nullptr && *host ? host : "127.0.0.1", std::atoi(port));
}

int NetPad::pads()
{
    int n = 0;
    for(const Pad& p : s_pads)
        if(p.joystick != nullptr) n++;
    return n;
}

void NetPad::attach(Uint8 slot, Uint8 bus, Uint16 vendor, Uint16 product, const std::string& name)
{
#if SDL_VERSION_ATLEAST(2, 24, 0)
    if(slot >= PadProtocol::MAX_PADS) return;
    detach(slot);

    // Gamepad virtual com todos os botões e eixos: com as máscaras zeradas, o SDL mapeia o
    // índice i para o SDL_GameControllerButton/Axis i, o mesmo número que a ponte manda.
    // VID:PID ficam zerados de propósito: com os do controle original, o SDL poderia aplicar o
    // mapeamento do banco de dados para ele, que numera os botões de outro jeito
    std::string label = name + " (ponte, " + PadInfo::busName(static_cast<PadInfo::Bus>(bus)) + ")";
    SDL_VirtualJoystickDesc desc;
    SDL_zero(desc);
    desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.naxes = SDL_CONTROLLER_AXIS_MAX;
    desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
    desc.name = label.c_str();
    int index = SDL_JoystickAttachVirtualEx(&desc);
    if(index < 0)
    {
        std::cerr << "NetPad: falha ao criar o controle virtual: " << SDL_GetError() << "\n";
        return;
    }
    s_pads[slot].joystick = SDL_JoystickOpen(index);
    s_pads[slot].state = PadProtocol::State();
    std::cout << "NetPad: controle remoto " << int(slot) << " conectado: " << name << " ["
              << PadInfo::busName(static_cast<PadInfo::Bus>(bus)) << " " << std::hex << vendor << ":" << product
              << std::dec << "]" << std::endl;
#else
    (void)slot; (void)bus; (void)vendor; (void)product; (void)name;
#endif
}

void NetPad::detach(Uint8 slot)
{
#if SDL_VERSION_ATLEAST(2, 24, 0)
    if(slot >= PadProtocol::MAX_PADS || s_pads[slot].joystick == nullptr) return;
    // O índice de um joystick muda quando outros conectam ou saem; o id da instância, não
    SDL_JoystickID id = SDL_JoystickInstanceID(s_pads[slot].joystick);
    SDL_JoystickClose(s_pads[slot].joystick);
    s_pads[slot].joystick = nullptr;
    for(int i = 0; i < SDL_NumJoysticks(); i++)
        if(SDL_JoystickGetDeviceInstanceID(i) == id)
        {
            SDL_JoystickDetachVirtual(i);
            break;
        }
    std::cout << "NetPad: controle remoto " << int(slot) << " desconectado" << std::endl;
#else
    (void)slot;
#endif
}

void NetPad::apply(Uint8 slot, const PadProtocol::State& state)
{
#if SDL_VERSION_ATLEAST(2, 24, 0)
    if(slot >= PadProtocol::MAX_PADS || s_pads[slot].joystick == nullptr) return;
    Pad& pad = s_pads[slot];
    for(int b = 0; b < SDL_CONTROLLER_BUTTON_MAX; b++)
    {
        Uint8 now = (state.buttons >> b) & 1, before = (pad.state.buttons >> b) & 1;
        if(now != before) SDL_JoystickSetVirtualButton(pad.joystick, b, now);
    }
    for(int a = 0; a < PadProtocol::AXES; a++)
        if(state.axes[a] != pad.state.axes[a]) SDL_JoystickSetVirtualAxis(pad.joystick, a, state.axes[a]);
    pad.state = state;
#else
    (void)slot; (void)state;
#endif
}

void NetPad::handle(const PadProtocol::Message& m)
{
    if(!s_greeted)
    {
        // Antes de tudo, a ponte se identifica; se for outra coisa (ou outra versão), desliga
        if(!PadProtocol::parseHello(m))
        {
            std::cerr << "NetPad: conexão recusada (não é a ponte de controles ou é outra versão)\n";
            dropClient();
            return;
        }
        s_greeted = true;
        std::cout << "NetPad: ponte de controles conectada" << std::endl;
        return;
    }
    switch(m.type)
    {
    case PadProtocol::MSG_ATTACH:
    {
        Uint8 bus;
        Uint16 vendor, product;
        std::string name;
        if(PadProtocol::parseAttach(m, bus, vendor, product, name)) attach(m.slot, bus, vendor, product, name);
        break;
    }
    case PadProtocol::MSG_STATE:
    {
        PadProtocol::State state;
        if(PadProtocol::parseState(m, state)) apply(m.slot, state);
        break;
    }
    case PadProtocol::MSG_DETACH:
        detach(m.slot);
        break;
    default:
        break; // tipo de uma versão mais nova: ignora
    }
}

void NetPad::dropClient()
{
    for(Uint8 slot = 0; slot < PadProtocol::MAX_PADS; slot++) detach(slot);
    PadSocket::close(s_client);
    s_client = PadSocket::INVALID;
    s_greeted = false;
    s_reader = PadProtocol::Reader();
}

void NetPad::poll()
{
    if(s_listener == PadSocket::INVALID) return;

    // Uma ponte nova substitui a anterior (por exemplo, depois de a anterior travar)
    PadSocket::Handle incoming = PadSocket::acceptPending(s_listener);
    if(incoming != PadSocket::INVALID)
    {
        if(s_client != PadSocket::INVALID) dropClient();
        s_client = incoming;
        s_last_data = SDL_GetTicks();
    }
    if(s_client == PadSocket::INVALID) return;

    Uint8 buffer[4096];
    for(;;)
    {
        int n = PadSocket::receive(s_client, buffer, sizeof buffer);
        if(n == PadSocket::WOULD_BLOCK) break;
        if(n == PadSocket::CLOSED)
        {
            std::cout << "NetPad: a ponte de controles desconectou" << std::endl;
            dropClient();
            return;
        }
        s_reader.feed(buffer, static_cast<size_t>(n));
        s_last_data = SDL_GetTicks();
    }

    PadProtocol::Message m;
    while(s_client != PadSocket::INVALID && s_reader.next(m)) handle(m);

    if(s_client != PadSocket::INVALID && SDL_GetTicks() - s_last_data > SILENCE_LIMIT)
    {
        std::cout << "NetPad: a ponte de controles parou de responder" << std::endl;
        dropClient();
    }
}

void NetPad::stop()
{
    if(s_client != PadSocket::INVALID) dropClient();
    PadSocket::close(s_listener);
    s_listener = PadSocket::INVALID;
}
