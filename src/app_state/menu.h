#ifndef MENU_H
#define MENU_H

#include "appstate.h"
#include "../objects/player.h"

#include <vector>
#include <string>

/**
 * @brief
 * Classe responsável pelo menu principal do jogo.
 * Permite ao usuário escolher entre os modos: 1 jogador, 2 jogadores ou sair do jogo.
 * Esta é a primeira tela exibida ao iniciar o aplicativo e permite a transição para o estado de jogo (classe Game).
 */
class Menu : public AppState
{
public:
    Menu();   // Construtor: inicializa o menu e o ponteiro visual (tanque)
    ~Menu();  // Destrutor: libera recursos alocados

    /**
     * Verifica se o estado do menu deve ser finalizado e se deve ocorrer a transição para o próximo estado do jogo.
     * @return true se alguma opção do menu foi selecionada ou se a tecla Esc foi pressionada, false caso contrário.
     */
    bool finished() const override;

    /**
     * Desenha o logo do jogo, as opções do menu e o ponteiro visual (tanque) indicando a seleção atual.
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
     * - Seta para cima/baixo ou analógico: altera a opção selecionada.
     * - Enter, Espaço, A ou Start: confirma a seleção atual.
     * - Esc, B ou Back: sai do programa.
     * @param ev Ponteiro para a união SDL_Event contendo o tipo e parâmetros dos eventos.
     */
    void eventProcess(SDL_Event* ev) override;

    /**
     * Realiza a transição para o próximo estado do jogo conforme a opção escolhida.
     * @return nullptr se "Exit" foi selecionado ou Esc pressionado; caso contrário, retorna ponteiro para o novo estado Game.
     */
    AppState* nextState() override;

private:
    /**
     * Container com todos os textos das opções do menu.
     */
    std::vector<std::string> m_menu_texts;

    /**
     * Índice da opção atualmente selecionada no menu.
     */
    int m_menu_index;

    /**
     * Ponteiro para o objeto Player que representa o tanque usado como ponteiro visual no menu.
     */
    Player* m_tank_pointer;

    /**
     * Indica se o menu deve ser finalizado e o estado deve ser trocado (iniciar jogo ou sair).
     */
    bool m_finished;

    /**
     * Flag para controlar se o botão "cima" do controle foi pressionado.
     * Evita repetição de eventos quando o analógico é mantido pressionado.
     */
    bool m_controller_up_pressed;

    /**
     * Flag para controlar se o botão "baixo" do controle foi pressionado.
     * Evita repetição de eventos quando o analógico é mantido pressionado.
     */
    bool m_controller_down_pressed;
};

#endif // MENU_H
