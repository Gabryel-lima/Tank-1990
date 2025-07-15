#ifndef SCORES_H
#define SCORES_H
#include "appstate.h"
#include "../objects/player.h"

#include <vector>
#include <string>

/**
 * @brief
 * Classe responsável por exibir os resultados (pontuações) dos jogadores da rodada anterior.
 */
class Scores : public AppState
{
public:
    Scores();
    /**
     * Construtor chamado pelo Game ao final da partida.
     * @param players - vetor contendo todos os jogadores que participaram da partida
     * @param level - número do último nível jogado
     * @param game_over - indica se o último nível foi perdido (true) ou vencido (false)
     */
    Scores(std::vector<Player*> players, int level, bool game_over);

    /**
     * Retorna @a true após um tempo determinado de exibição da tela de pontuação.
     * @return @a true ou @a false
     */
    bool finished() const;

    /**
     * Responsável por desenhar os textos e tanques dos jogadores na tela de pontuação.
     */
    void draw();

    /**
     * Atualiza o contador de pontos animado.
     * @param dt - tempo desde a última chamada da função, usado para controlar o tempo de exibição da tela de resultados
     */
    void update(Uint32 dt);

    /**
     * Processa eventos de teclado, em especial o Enter, que finaliza a contagem animada dos pontos e acelera a transição para o próximo estado.
     * @param ev - ponteiro para a união SDL_Event contendo o tipo e parâmetros dos eventos
     */
    void eventProcess(SDL_Event* ev);

    /**
     * Retorna um ponteiro para o próximo estado da aplicação.
     * Se o jogador perdeu, o próximo estado é @a Menu; se venceu, o próximo estado é @a Game.
     * @return ponteiro para o próximo estado
     */
    AppState* nextState();

private:
    /**
     * Vetor com todos os jogadores (mortos e vivos) da rodada.
     */
    std::vector<Player*> m_players;

    /**
     * Número do último nível jogado.
     */
    int m_level;

    /**
     * Indica se o último nível foi perdido (true) ou vencido (false).
     */
    bool m_game_over;

    /**
     * Valor atual do contador de pontos (usado para animação da contagem).
     */
    unsigned m_score_counter;

    /**
     * Indica se o contador de pontos está ativo (animação em andamento).
     */
    bool m_score_counter_run;

    /**
     * Maior pontuação entre os jogadores, usada como meta para o contador animado.
     */
    unsigned m_max_score;

    /**
     * Tempo (em milissegundos) desde o fim da contagem de pontos.
     */
    Uint32 m_show_time;
};

#endif // SCORES_H
