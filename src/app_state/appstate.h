#ifndef APPSTATE_H
#define APPSTATE_H

#include <SDL2/SDL_events.h>
#include <string>

/**
 * @brief
 * Classe interface base para os estados do aplicativo/jogo.
 * As classes @a Game, @a Menu, @a Scores devem herdar desta interface.
 */
class AppState 
{
public:
    virtual ~AppState() {}

    /**
     * Verifica se o estado atual do jogo terminou.
     * @return @a true se o estado atual terminou, caso contrário @a false.
     */
    virtual bool finished() const = 0;

    /**
     * Desenha os elementos do jogo pertencentes a este estado.
     */
    virtual void draw() = 0;

    /**
     * Atualiza o estado dos objetos e contadores do jogo.
     * @param dt - tempo desde a última chamada da função, em milissegundos.
     */
    virtual void update(Uint32 dt) = 0;

    /**
     * Processa eventos detectados pela biblioteca SDL2.
     * @param ev - ponteiro para a união SDL_Event contendo o tipo e parâmetros de diferentes eventos.
     */
    virtual void eventProcess(SDL_Event* ev) = 0;

    /**
     * Retorna o próximo estado após o término do estado atual.
     * Esta função deve ser chamada apenas se @a finished retornar @a true.
     * @return próximo estado do jogo.
     */
    virtual AppState* nextState() = 0;
};

#endif // APPSTATE_H
