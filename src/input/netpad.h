#ifndef NETPAD_H
#define NETPAD_H

#include "pad_protocol.h"
#include "pad_socket.h"

#include <SDL2/SDL.h>

/**
 * @brief Controles que chegam pela rede (ponte padbridge) viram joysticks virtuais do SDL.
 *
 * É a "camada 3b" de CONTROLES.md: uma fonte de controles que não é o SDL lendo o hardware.
 * O uso principal é o WSL2: o Windows enxerga os controles Bluetooth e o WSL não; a ponte
 * os lê no Windows e manda o estado para cá. Cada controle remoto vira um joystick virtual do
 * tipo gamepad (SDL_JoystickAttachVirtualEx), com o mapeamento padrão, e chega ao resto do
 * jogo pelos mesmos eventos de um controle de verdade (Controllers não muda nada).
 *
 * Desligado por padrão: só escuta se for iniciado (start, ou TANK_NETPAD=porta no ambiente).
 * Uma ponte por vez; uma nova conexão substitui a anterior. Precisa do SDL 2.24 ou mais novo.
 */
class NetPad
{
public:
    /** O SDL desta compilação tem joysticks virtuais com mapeamento de gamepad (2.24+). */
    static bool supported();

    /**
     * Começa a escutar em @a host:@a port. Chamar depois do SDL_Init do joystick.
     * @return false se não suportado ou se a porta não abriu (o motivo vai para o stderr)
     */
    static bool start(const char* host, int port);

    /**
     * Começa se o ambiente pedir: TANK_NETPAD=porta (ex.: 47990) e, opcional,
     * TANK_NETPAD_BIND=endereço (padrão 127.0.0.1; no WSL o play.cmd usa 0.0.0.0).
     */
    static bool startFromEnvironment();

    /**
     * Aceita a ponte, lê o que chegou e atualiza os joysticks virtuais. Chamar a cada quadro,
     * antes de ler os eventos do SDL (os virtuais geram os eventos de controle normais).
     */
    static void poll();

    /** Solta os controles remotos e fecha a porta. Chamar antes do SDL_Quit. */
    static void stop();

    static bool listening() { return s_listener != PadSocket::INVALID; }

    /** Controles remotos conectados agora. */
    static int pads();

private:
    struct Pad
    {
        SDL_Joystick* joystick = nullptr;
        PadProtocol::State state;
    };

    static void dropClient();
    static void attach(Uint8 slot, Uint8 bus, Uint16 vendor, Uint16 product, const std::string& name);
    static void detach(Uint8 slot);
    static void apply(Uint8 slot, const PadProtocol::State& state);
    static void handle(const PadProtocol::Message& m);

    static PadSocket::Handle s_listener;
    static PadSocket::Handle s_client;
    static bool s_greeted;
    static Uint32 s_last_data;
    static PadProtocol::Reader s_reader;
    static Pad s_pads[PadProtocol::MAX_PADS];
};

#endif // NETPAD_H
