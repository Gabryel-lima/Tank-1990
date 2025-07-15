#ifndef APPCONFIG_H
#define APPCONFIG_H

#include "objects/player.h"
#include <iostream>
#include <SDL2/SDL_rect.h>
#include <vector>

using namespace std;

/**
 * @brief Classe que contém o conjunto de parâmetros da aplicação.
 */
class AppConfig
{
public:
    /**
     * Caminho para a textura dos objetos.
     */
    static string texture_path;
    /**
     * Caminho para o diretório dos níveis.
     */
    static string levels_path;
    /**
     * Caminho para a fonte.
     */
    static string font_name;
    /**
     * Texto exibido após a derrota.
     */
    static string game_over_text;
    /**
     * Dimensões do tabuleiro do jogo.
     */
    static SDL_Rect map_rect;
    /**
     * Dimensões e posição, em relação ao tabuleiro, da área de status do jogo.
     */
    static SDL_Rect status_rect;
    /**
     * Tamanho da janela da aplicação.
     */
    static SDL_Rect windows_rect;
    /**
     * Tamanho da célula no tabuleiro.
     */
    static SDL_Rect tile_rect;
    /**
     * Duas posições iniciais dos jogadores.
     */
    static vector<SDL_Point> player_starting_point;
    /**
     * Três posições iniciais dos inimigos.
     */
    static vector<SDL_Point> enemy_starting_point;
    /**
     * Mapeamento das teclas de controle dos tanques dos jogadores.
     */
    static vector <Player::PlayerKeys> player_keys;
    /**
     * Tempo de exibição do número da rodada ao seu início.
     */
    static unsigned level_start_time;
    /**
     * Duração do deslizamento no gelo em milissegundos.
     */
    static unsigned slip_time;
    /**
     * Número de inimigos a serem derrotados em uma rodada.
     */
    static unsigned enemy_start_count;
    /**
     * Tempo após o qual um novo inimigo pode aparecer no mapa, em milissegundos.
     */
    static unsigned enemy_redy_time;
    /**
     * Número máximo padrão de projéteis que o jogador pode disparar.
     */
    static unsigned player_bullet_max_size;
    /**
     * Tempo de exibição dos resultados após a contagem dos pontos, em milissegundos.
     */
    static unsigned score_show_time;
    /**
     * Tempo durante o qual o bônus é visível no mapa, em milissegundos.
     */
    static unsigned bonus_show_time;
    /**
     * Duração do escudo em milissegundos.
     */
    static unsigned tank_shield_time;
    /**
     * Tempo durante o qual os inimigos ficam congelados, em milissegundos.
     */
    static unsigned tank_frozen_time;
    /**
     * Tempo entre a eliminação de todos os inimigos e a transição para a tela de resultados, em milissegundos.
     */
    static unsigned level_end_time;
    /**
     * Duração da proteção de pedra ao redor da águia, em milissegundos.
     */
    static unsigned protect_eagle_time;
    /**
     * Tempo de piscar do bônus, em milissegundos.
     */
    static unsigned bonus_blink_time;
    /**
     * Tempo mínimo entre disparos do jogador, em milissegundos.
     */
    static unsigned player_reload_time;
    /**
     * Número máximo de tanques inimigos no mapa ao mesmo tempo.
     */
    static int enemy_max_count_on_map;
    /**
     * Velocidade de entrada do texto "GAME OVER".
     */
    static double game_over_entry_speed;
    /**
     * Velocidade nominal do tanque.
     */
    static double tank_default_speed;
    /**
     * Velocidade nominal do projétil.
     */
    static double bullet_default_speed;
    /**
     * Variável que armazena se a exibição dos alvos dos inimigos está ativada.
     */
    static bool show_enemy_target;
};

#endif // APPCONFIG_H
