#ifndef APP_H
#define APP_H

#include "app_state/appstate.h"

/**
 * @brief
 * Classe principal da aplicação. Sua execução representa o ciclo de vida da aplicação.
 */
class App
{
public:
    App();  ///< Construtor: inicializa ponteiros e variáveis principais.
    ~App(); ///< Destrutor: libera recursos alocados.

    /**
     * Executa a aplicação.
     * Responsável por inicializar a biblioteca SDL2, o motor do jogo, carregar texturas e fontes.
     * Após a inicialização bem-sucedida, entra no loop principal do programa, que:
     *   - processa eventos,
     *   - atualiza o estado atual da aplicação,
     *   - desenha os objetos na tela.
     */
    void run();

    /**
     * Processa eventos pendentes na fila de eventos do SDL.
     * Responsável por capturar e tratar entradas do usuário e outros eventos do sistema.
     */
    void eventProces();

private:
    /**
     * Variável que mantém o loop principal do programa em execução.
     * Quando false, o loop principal é encerrado e a aplicação termina.
     */
    bool is_running;

    /**
     * Ponteiro para o estado atual da aplicação (menu, jogo, etc).
     * Permite alternar entre diferentes estados da aplicação.
     */
    AppState* m_app_state;

    /**
     * Ponteiro para o objeto da janela principal da aplicação.
     * Usado para renderização e manipulação da janela SDL.
     */
    SDL_Window* m_window;
};

#endif // APP_H
