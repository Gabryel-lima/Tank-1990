#ifndef APP_H
#define APP_H

#include "app_state/appstate.h" // Ok

/**
 * @brief
 * Classe principal da aplicação. Seu funcionamento é equivalente ao funcionamento da aplicação.
 */
class App {
public: 
    App(); 
    ~App();
        /**
         * A função realiza a inicialização da biblioteca SDL2, do motor do jogo, o carregamento das texturas e das fontes.
         * Após a inicialização bem-sucedida, entra no loop principal do programa, que: reage a eventos,
         * atualiza o estado atual da aplicação e desenha objetos na tela.
         */
        void run();
        /**
         * Função que lida com os eventos que estão na fila.
         */
        void eventProces();

private:
    /**
     * Variável que mantém o funcionamento do loop principal do programa.
     */
    bool is_running;
    /**
     * Estado atual da aplicação.
     */
    AppState* m_app_state;
    /**
     * Objeto da janela da aplicação.
     */
    SDL_Window* m_window;
};

#endif // APP_H
