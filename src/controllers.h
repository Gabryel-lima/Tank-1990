#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include <SDL2/SDL.h>
#include <vector>

/**
 * @brief Gerencia os controles (gamepads) conectados e decide qual jogador usa qual.
 *
 * Abre todos os controles conectados ao iniciar e acompanha conexões e
 * desconexões (hotplug) pelos eventos SDL_CONTROLLERDEVICEADDED/REMOVED.
 * O SDL só gera eventos de controle para controles abertos, então o menu
 * também depende desta classe para receber os botões.
 *
 * Distribuição dos controles (na ordem em que foram conectados):
 *  - primeiro para os jogadores sem teclado (Jogador 3 e 4);
 *  - depois para os jogadores de teclado, na ordem (Jogador 1, Jogador 2).
 *
 * Exemplos:
 *  - 1 ou 2 jogadores: controle 1 -> J1, controle 2 -> J2
 *  - 3 jogadores:      controle 1 -> J3, controle 2 -> J1, controle 3 -> J2
 *  - 4 jogadores:      controle 1 -> J3, controle 2 -> J4, controle 3 -> J1, controle 4 -> J2
 */
class Controllers
{
public:
    /** Abre todos os controles já conectados. Chamar depois do SDL_Init. */
    static void init();

    /** Fecha todos os controles. Chamar antes do SDL_Quit. */
    static void shutdown();

    /** Trata conexão/desconexão de controles. Deve receber todos os eventos. */
    static void handleEvent(const SDL_Event* ev);

    /** Define quantos jogadores estão na partida (afeta a distribuição). */
    static void setPlayerCount(int count);

    /**
     * Retorna o controle do jogador, ou nullptr se ele não tiver um.
     * @param player_index - índice do jogador (0 = Jogador 1)
     */
    static SDL_GameController* forPlayer(int player_index);

    /** Quantidade de controles conectados. */
    static int count();

private:
    static std::vector<SDL_GameController*> m_controllers;
    static int m_player_count;
};

#endif // CONTROLLERS_H
