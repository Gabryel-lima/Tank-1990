#ifndef MENU_H
#define MENU_H

#include "appstate.h"
#include "duel.h"
#include "../objects/player.h"

#include <vector>
#include <string>

/**
 * @brief
 * Classe responsável pelo menu principal do jogo.
 * Permite escolher a campanha (1 a 4 jogadores), os modos extras (duelo) ou sair do jogo.
 * Esta é a primeira tela exibida ao iniciar o aplicativo e permite a transição para o estado
 * de jogo (classe Game) ou de duelo (classe Duel).
 *
 * Telas do menu:
 * @li principal: 1 a 4 jogadores, Extra Modes, Exit
 * @li Extra Modes: Duel Mode
 * @li Duel Mode: 1 vs 1, 2 vs 2 ou equipes personalizadas (só jogadores humanos, até 4)
 * @li configuração do duelo: quantidade de jogadores (personalizado) e equipe de cada um
 */
class Menu : public AppState
{
public:
    /**
     * Telas do menu.
     */
    enum Screen
    {
        SCREEN_MAIN,
        SCREEN_EXTRA,
        SCREEN_DUEL_FORMAT,
        SCREEN_DUEL_SETUP
    };

    /**
     * @param screen - tela inicial (o duelo volta direto para a configuração, para a revanche)
     */
    explicit Menu(Screen screen = SCREEN_MAIN);
    ~Menu();  // Destrutor: libera recursos alocados

    /**
     * Verifica se o estado do menu deve ser finalizado e se deve ocorrer a transição para o próximo estado do jogo.
     * @return true se uma partida foi iniciada ou se o jogador saiu do jogo, false caso contrário.
     */
    bool finished() const override;

    /**
     * Desenha o logo do jogo (ou o título da tela), as opções do menu e o ponteiro visual (tanque) indicando a seleção atual.
     */
    void draw() override;

    /**
     * Atualiza a animação do ponteiro do menu (tanque).
     * @param dt Tempo desde a última atualização (em milissegundos).
     * @see Tank::update(Uint32 dt)
     */
    void update(Uint32 dt) override;

    /**
     * Processa eventos de teclado e controle para navegação e seleção no menu.
     * - Seta para cima/baixo, D-pad ou analógico: altera a opção selecionada.
     * - Seta para esquerda/direita: altera o valor da opção (configuração do duelo).
     * - Enter, Espaço, A ou Start: confirma a seleção atual.
     * - Esc, B ou Back: volta para a tela anterior; na tela principal, sai do programa.
     * @param ev Ponteiro para a união SDL_Event contendo o tipo e parâmetros dos eventos.
     */
    void eventProcess(SDL_Event* ev) override;

    /**
     * Realiza a transição para o próximo estado do jogo conforme a opção escolhida.
     * @return nullptr se "Exit" foi selecionado ou Esc pressionado na tela principal;
     * caso contrário, retorna ponteiro para o novo estado Game ou Duel.
     */
    AppState* nextState() override;

private:
    /**
     * Ação de cada item do menu.
     */
    enum Item
    {
        ITEM_CAMPAIGN_1, ITEM_CAMPAIGN_2, ITEM_CAMPAIGN_3, ITEM_CAMPAIGN_4,
        ITEM_EXTRA_MODES, ITEM_EXIT,
        ITEM_DUEL_MODE,
        ITEM_FORMAT_1V1, ITEM_FORMAT_2V2, ITEM_FORMAT_CUSTOM,
        ITEM_HUMANS,
        ITEM_HUMAN_1_TEAM, ITEM_HUMAN_2_TEAM, ITEM_HUMAN_3_TEAM, ITEM_HUMAN_4_TEAM,
        ITEM_START, ITEM_BACK
    };

    /**
     * Resultado escolhido no menu.
     */
    enum Result
    {
        RESULT_NONE, RESULT_EXIT, RESULT_CAMPAIGN, RESULT_DUEL
    };

    /** Monta a lista de itens da tela atual. */
    void buildItems();

    /** Troca de tela, mantendo a seleção no primeiro item. */
    void openScreen(Screen screen);

    /** Move a seleção (delta = -1 sobe, +1 desce). */
    void moveSelection(int delta);

    /** Altera o valor do item selecionado (configuração do duelo). */
    void changeValue(int delta);

    /** Confirma o item selecionado. */
    void confirm();

    /** Volta para a tela anterior (na principal, sai do jogo). */
    void back();

    /** Texto do item, incluindo o valor atual. */
    std::string itemText(Item item) const;

    /** Item tem valor ajustável com esquerda/direita. */
    bool isValueItem(Item item) const;

    /** Altura (y) do texto do item na posição i. */
    int itemY(int i) const;

    /** Define um formato de duelo N vs N com os jogadores alternando entre as equipes. */
    void applyDuelFormat(int team_size);

    /** Atualiza o tamanho de cada equipe a partir da equipe de cada jogador. */
    void syncTeamSizes();

    Screen m_screen;
    std::vector<Item> m_items;

    /**
     * Índice da opção atualmente selecionada no menu.
     */
    int m_menu_index;

    Result m_result;
    int m_campaign_players;

    /**
     * Configuração do duelo. Estática para ser lembrada entre partidas (revanche).
     */
    static DuelConfig s_duel_config;
    static bool s_duel_custom;

    /**
     * Ponteiro para o objeto Player que representa o tanque usado como ponteiro visual no menu.
     */
    Player* m_tank_pointer;

    /**
     * Indica se o menu deve ser finalizado e o estado deve ser trocado (iniciar jogo ou sair).
     */
    bool m_finished;

    /**
     * Flags do analógico do controle: evitam repetir o movimento enquanto ele é mantido inclinado.
     */
    bool m_controller_up_pressed;
    bool m_controller_down_pressed;
    bool m_controller_left_pressed;
    bool m_controller_right_pressed;
};

#endif // MENU_H
