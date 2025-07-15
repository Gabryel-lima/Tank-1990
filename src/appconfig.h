#ifndef APPCONFIG_H
#define APPCONFIG_H

#include "objects/player.h"
#include <iostream>
#include <SDL2/SDL_rect.h>
#include <vector>

using namespace std;

/**
 * @brief Classe que armazena todos os parâmetros globais de configuração do aplicativo/jogo.
 * 
 * Esta classe contém apenas membros estáticos, servindo como um repositório central de constantes
 * e configurações utilizadas em todo o jogo, como caminhos de arquivos, dimensões de tela, tempos de animação,
 * limites de entidades, etc.
 */
class AppConfig
{
public:
    /**
     * Caminho do arquivo de textura dos objetos do jogo.
     */
    static string texture_path;

    /**
     * Caminho do diretório onde estão os arquivos de fases/níveis.
     */
    static string levels_path;

    /**
     * Caminho do arquivo de fonte utilizada no jogo.
     */
    static string font_name;

    /**
     * Texto exibido na tela de Game Over.
     */
    static string game_over_text;

    /**
     * Retângulo que define a área do mapa do jogo (em pixels).
     * Usado para delimitar onde os objetos do jogo podem ser desenhados.
     */
    static SDL_Rect map_rect;

    /**
     * Retângulo que define a área de status (HUD) ao lado do mapa.
     * Normalmente exibe informações como vidas, pontuação, etc.
     */
    static SDL_Rect status_rect;

    /**
     * Retângulo que define o tamanho total da janela do aplicativo.
     * Inclui o mapa e a área de status.
     */
    static SDL_Rect windows_rect;

    /**
     * Retângulo que define o tamanho de uma célula (tile) do mapa.
     * Usado para calcular posições e colisões.
     */
    static SDL_Rect tile_rect;

    /**
     * Vetor com as duas posições iniciais dos jogadores (Player 1 e Player 2).
     * Cada posição é um SDL_Point (x, y).
     */
    static vector<SDL_Point> player_starting_point;

    /**
     * Vetor com as três posições iniciais dos tanques inimigos.
     * Cada posição é um SDL_Point (x, y).
     */
    static vector<SDL_Point> enemy_starting_point;

    /**
     * Vetor com as configurações de teclas de controle para cada jogador.
     * Cada elemento é uma struct Player::PlayerKeys.
     */
    static vector<Player::PlayerKeys> player_keys;

    /**
     * Tempo (em milissegundos) que o número da fase é exibido ao iniciar uma nova rodada.
     */
    static unsigned level_start_time;

    /**
     * Tempo de duração do "escorregão" no gelo (em milissegundos).
     * Usado para simular inércia ao mover tanques sobre gelo.
     */
    static unsigned slip_time;

    /**
     * Quantidade total de inimigos que devem ser derrotados em uma rodada.
     */
    static unsigned enemy_start_count;

    /**
     * Tempo mínimo (em milissegundos) entre o surgimento de novos inimigos no mapa.
     */
    static unsigned enemy_redy_time;

    /**
     * Quantidade máxima de projéteis que um jogador pode ter ativos simultaneamente.
     */
    static unsigned player_bullet_max_size;

    /**
     * Tempo (em milissegundos) que a tela de pontuação é exibida após o término da contagem de pontos.
     */
    static unsigned score_show_time;

    /**
     * Tempo (em milissegundos) que um bônus permanece visível no mapa.
     */
    static unsigned bonus_show_time;

    /**
     * Tempo de duração do escudo protetor do tanque (em milissegundos).
     */
    static unsigned tank_shield_time;

    /**
     * Tempo (em milissegundos) que os inimigos permanecem congelados após pegar o bônus de congelamento.
     */
    static unsigned tank_frozen_time;

    /**
     * Tempo (em milissegundos) entre eliminar todos os inimigos e ir para a tela de resultados.
     */
    static unsigned level_end_time;

    /**
     * Tempo de duração da proteção de pedra ao redor da águia (em milissegundos).
     */
    static unsigned protect_eagle_time;

    /**
     * Intervalo de tempo (em milissegundos) para o efeito de piscar do bônus.
     */
    static unsigned bonus_blink_time;

    /**
     * Tempo mínimo (em milissegundos) entre disparos do jogador.
     * Controla a cadência de tiro.
     */
    static unsigned player_reload_time;

    /**
     * Quantidade máxima de tanques inimigos que podem estar simultaneamente no mapa.
     */
    static int enemy_max_count_on_map;

    /**
     * Velocidade de entrada do texto "GAME OVER" na tela.
     * Usado para animar a mensagem de fim de jogo.
     */
    static double game_over_entry_speed;

    /**
     * Velocidade padrão de movimento dos tanques.
     */
    static double tank_default_speed;

    /**
     * Velocidade padrão dos projéteis disparados.
     */
    static double bullet_default_speed;

    /**
     * Indica se a visualização dos alvos dos inimigos está ativada (debug/cheat).
     */
    static bool show_enemy_target;
};

#endif // APPCONFIG_H
