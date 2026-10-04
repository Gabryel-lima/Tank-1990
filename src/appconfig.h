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
     * Vetor com as duas posições iniciais dos jogadores (Player 1, Player 2 e Player 3).
     * Cada posição é um SDL_Point (x, y).
     */
    static vector<SDL_Point> player_starting_point;

    /**
     * Vetor com as três posições iniciais dos tanques inimigos.
     * Cada posição é um SDL_Point (x, y).
     */
    static vector<SDL_Point> enemy_starting_point;

    /**
     * Layouts de teclado (WASD, setas). São a reserva de quem não tem controle;
     * a classe Controllers decide qual jogador usa cada um.
     */
    static vector<Player::PlayerKeys> keyboard_layouts;

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
     * Distância máxima (em pixels) que o tanque do jogador pode deslizar de lado
     * para contornar a quina de um obstáculo e entrar num corredor.
     * Com 0 o deslize fica desativado.
     */
    static int tank_corner_slide_max;

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
     * Tempo (ms) que o texto "GAME OVER" fica parado no centro do mapa antes da pontuação.
     */
    static unsigned game_over_hold_time;

    /**
     * Intervalo (ms) do piscar do texto "PAUSE".
     */
    static unsigned pause_blink_time;

    /**
     * Velocidade padrão de movimento dos tanques.
     */
    static double tank_default_speed;

    /**
     * Caminho da pasta dos mapas do modo duelo.
     */
    static string duel_levels_path;

    /**
     * Mapas do duelo: {arquivo em duel_levels_path, nome mostrado no menu}.
     */
    static vector<pair<string, string>> duel_maps;

    /**
     * Caminho da pasta dos mapas do modo sobrevivência (separados da campanha e do duelo).
     */
    static string survival_levels_path;

    /**
     * Mapas do modo sobrevivência: {arquivo em survival_levels_path, nome mostrado no menu}.
     */
    static vector<pair<string, string>> survival_maps;

    /**
     * Colunas (x) de nascimento do duelo para a equipe A, em ordem de preferência
     * (lados alternados). A equipe B usa o espelho em ponto.
     */
    static vector<int> duel_spawn_columns;

    /**
     * Linha (y) de nascimento de cada equipe do duelo: 0 (A) embaixo, 1 (B) em cima.
     */
    static vector<int> duel_spawn_rows;

    /**
     * Vidas de cada tanque no duelo (equipes menores recebem mais, ver Duel).
     */
    static int duel_tank_lives;

    /**
     * Rodadas vencidas necessárias para ganhar o duelo.
     */
    static int duel_rounds_to_win;

    /**
     * Tempo (ms) com o mapa vazio de bônus até surgir o próximo.
     */
    static unsigned duel_bonus_interval;

    /**
     * Duração (ms) do congelamento causado pelo relógio no duelo.
     */
    static unsigned duel_freeze_time;

    /**
     * Cadência máxima de tiro de cada jogador no duelo (tiros por segundo).
     */
    static double duel_max_shots_per_second;

    /**
     * Máximo de bots aliados (bônus de reforço) em campo ao mesmo tempo, por equipe.
     */
    static int duel_max_allies;

    /**
     * Duração (ms) do escudo dado pelo capacete no duelo.
     */
    static unsigned duel_helmet_time;

    /**
     * No duelo, com 3 estrelas um tiro só tira uma estrela (como na campanha)?
     * Desligado, o canhão quebra pedra mas não vale uma vida extra.
     */
    static bool duel_star_armor;

    /**
     * A equipe menor do duelo defende a base com pedra se a outra tiver pelo menos
     * esta proporção de tanques (2 = o dobro).
     */
    static int duel_stone_wall_ratio;

    /**
     * Chance (0 a 1) de um bônus surgir cinza, como no jogo original: qualquer equipe pega.
     * No resto das vezes ele surge na cor de uma equipe e só ela pega.
     */
    static double duel_neutral_bonus_chance;

    /**
     * Chance (0 a 1) de um bônus de equipe surgir na metade do mapa da equipe adversária.
     */
    static double duel_team_bonus_enemy_side_chance;

    // ======================== Poderes dos modos extras ========================

    /** Duração (ms) da torreta no mapa. */
    static unsigned power_turret_time;
    /** Intervalo (ms) entre os tiros da torreta. */
    static unsigned power_turret_reload;
    /** Alcance da torreta, em tiles. */
    static int power_turret_range;
    /** Duração (ms) da mina no mapa, se ninguém passar por cima. */
    static unsigned power_mine_time;
    /** Duração (ms) e multiplicador de velocidade do turbo. */
    static unsigned power_turbo_time;
    static double power_turbo_factor;
    /** Duração (ms) da trégua (sobrevivência: os inimigos param de surgir). */
    static unsigned power_truce_time;
    /**
     * Sobrevivência: os poderes de lugar e momento (mina, barricada, torreta, retorno e turbo)
     * ficam guardados até o jogador usar (true) ou valem na hora em que são pegos (false).
     */
    static bool survival_store_powers;

    // ======================== Modo sobrevivência ========================

    /** Inimigos da primeira onda. */
    static int survival_first_wave_enemies;

    /** Inimigos a mais em cada onda seguinte. */
    static int survival_wave_enemy_step;

    /** Limite de inimigos numa onda. */
    static int survival_max_wave_enemies;

    /** Inimigos no mapa ao mesmo tempo na primeira onda (um a mais a cada jogador extra). */
    static int survival_first_on_map;

    /** A cada quantas ondas cabe mais um inimigo no mapa ao mesmo tempo. */
    static int survival_on_map_every_waves;

    /** Limite de inimigos no mapa ao mesmo tempo. */
    static int survival_max_on_map;

    /** Intervalo (ms) entre o surgimento de inimigos na primeira onda e o mínimo nas últimas. */
    static unsigned survival_first_spawn_delay;
    static unsigned survival_min_spawn_delay;

    /** Pausa (ms) com o aviso "WAVE N" antes de cada onda. */
    static unsigned survival_wave_intro_time;

    /** A cada quantas ondas vencidas todos ganham uma vida (e quem caiu volta). */
    static int survival_life_every_waves;

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
