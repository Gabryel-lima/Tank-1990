#ifndef APPSTATE_H
#define APPSTATE_H

#include <SDL2/SDL_events.h>
#include <string>

/**
 * @brief
 * Classe que serve como interface, da qual herdam as classes @a Game, @a Menu, @a Scores
 */
class AppState {
public:
    virtual ~AppState() {}

    /**
     * Função que verifica se o estado atual do jogo terminou.
     * @return @a true se o estado atual do jogo terminou, caso contrário @a false.
     */
    virtual bool finished() const = 0;
    /**
     * Função que desenha os elementos do jogo pertencentes a este estado
     */
    virtual void draw() = 0;
    /**
     * Função que atualiza o estado dos objetos e contadores do jogo
     * @param dt - tempo desde a última chamada da função em milissegundos
     */
    virtual void update(Uint32 dt) = 0;
    /**
     * Função que permite o tratamento de eventos detectados pela biblioteca SDL2.
     * @param ev - ponteiro para a união SDL_Event contendo o tipo e parâmetros de vários eventos
     */
    virtual void eventProcess(SDL_Event* ev) = 0;
    /**
     * Função que retorna o próximo estado após o término do atual. Deve ser chamada apenas quando a função @a finished retornar @a true.
     * @return próximo estado do jogo
     */
    virtual AppState* nextState() = 0;
};
#endif // APPSTATE_H
