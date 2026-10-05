#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include "objects/player.h"

#include <SDL2/SDL.h>
#include <map>
#include <string>
#include <vector>

/**
 * @brief Gerencia os controles (gamepads) e decide qual dispositivo cada jogador usa.
 *
 * Abre todos os controles conectados ao iniciar e acompanha conexões e
 * desconexões (hotplug) pelos eventos SDL_CONTROLLERDEVICEADDED/REMOVED.
 * O SDL só gera eventos de controle para controles abertos, então o menu
 * também depende desta classe para receber os botões.
 *
 * Distribuição automática, sem configuração:
 *  1. Controle primeiro: o controle da vaga 1 vai para o Jogador 1, o da vaga 2 para o Jogador 2...
 *     As vagas seguem quem aperta um botão, não a ordem de conexão: no primeiro aperto, o
 *     controle passa na frente dos controles parados (que nunca apertaram nada: um pareado
 *     e esquecido ligado, um receptor sem fio sem controle). Quem pega um controle e aperta
 *     primeiro é o Jogador 1; um controle parado nunca rouba a vaga de quem está jogando.
 *  2. Teclado como reserva: quem ficou sem controle recebe, na ordem, os layouts de
 *     AppConfig::keyboard_layouts (WASD + Espaço, depois setas + Ctrl direito).
 *  3. Layouts que sobrarem continuam com o dono original (WASD com J1, setas com J2),
 *     para que quem joga sozinho possa usar o teclado mesmo com um controle conectado.
 *
 * As vagas são estáveis durante a partida: quem já apertou um botão nunca muda de vaga, e
 * se um controle desconecta, a vaga dele fica
 * vazia (o jogador cai para o teclado, se houver layout livre) e os outros controles não
 * mudam de dono. Um controle conectado ocupa a primeira vaga vazia. As vagas vazias só
 * são compactadas no início de uma partida (setPlayerCount).
 *
 * Exemplos:
 *  - 1 jogador, 1 controle:   J1 controle 1 (e também WASD)
 *  - 2 jogadores, 0 controles: J1 WASD, J2 setas
 *  - 3 jogadores, 1 controle:  J1 controle 1, J2 WASD, J3 setas
 *  - 4 jogadores, 2 controles: J1 controle 1, J2 controle 2, J3 WASD, J4 setas
 *  - 4 jogadores, 1 controle:  J1 controle 1, J2 WASD, J3 setas, J4 sem dispositivo
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

    /**
     * Define quantos jogadores estão na partida e compacta as vagas vazias
     * (chamar no início de cada partida).
     */
    static void setPlayerCount(int count);

    /**
     * Controle do jogador na partida atual, ou nullptr se ele não tiver um.
     * @param player_index - índice do jogador (0 = Jogador 1)
     */
    static SDL_GameController* forPlayer(int player_index);

    /**
     * Teclas do jogador na partida atual, ou nullptr se ele não usar teclado.
     * @param player_index - índice do jogador (0 = Jogador 1)
     */
    static const Player::PlayerKeys* keyboardFor(int player_index);

    /**
     * Nome do dispositivo principal do jogador numa partida com @a player_count jogadores,
     * para mostrar na tela: "PAD 1".."PAD 4", "WASD", "ARROWS" ou "NO PAD".
     */
    static std::string inputName(int player_count, int player_index);

    /**
     * Quantos jogadores ficariam sem nenhum dispositivo numa partida com @a player_count jogadores.
     */
    static int playersWithoutInput(int player_count);

    /** Quantidade de controles conectados. */
    static int count();

private:
    /**
     * Dispositivos de um jogador: vaga de controle (-1 se nenhum) e
     * layout de teclado (-1 se nenhum).
     */
    struct Assignment
    {
        int pad_slot = -1;
        int keyboard = -1;
    };

    /** Calcula a distribuição para uma partida com @a player_count jogadores. */
    static std::vector<Assignment> assign(int player_count);

    /** Vagas de controle; nullptr marca uma vaga vazia (controle desconectado). */
    static std::vector<SDL_GameController*> m_controllers;

    /** Ordem do primeiro botão apertado em cada controle (instance id → 1, 2, 3...). */
    static std::map<SDL_JoystickID, int> m_first_input;
    static int m_inputs;

    /**
     * Primeiro botão do controle: ele troca de vaga com o primeiro controle parado (que
     * nunca apertou nada) numa vaga anterior. Quem já apertou não muda de vaga.
     */
    static void noteInput(SDL_JoystickID id);
    static int m_player_count;
};

#endif // CONTROLLERS_H
