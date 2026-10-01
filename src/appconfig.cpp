#include "appconfig.h"

// Caminho do arquivo de textura principal do jogo
string AppConfig::texture_path = "texture.png";
// Caminho da pasta onde estão os arquivos de fases/níveis
string AppConfig::levels_path = "levels/";
// Caminho da pasta dos mapas do modo duelo (separados da campanha)
string AppConfig::duel_levels_path = "duel_levels/";
// Nome do arquivo de fonte utilizada no jogo
string AppConfig::font_name = "prstartk.ttf";
// Texto exibido na tela de Game Over
string AppConfig::game_over_text = "GAME OVER";

// Retângulo que define a área do mapa do jogo (26x26 tiles de 16 pixels)
SDL_Rect AppConfig::map_rect = {0, 0, 26*16, 26*16};
// Retângulo que define a área de status (painel lateral)
SDL_Rect AppConfig::status_rect = {26*16, 0, 3*16, AppConfig::map_rect.h};
// Retângulo que define o tamanho total da janela (mapa + painel)
SDL_Rect AppConfig::windows_rect = {0, 0, AppConfig::map_rect.w + AppConfig::status_rect.w, AppConfig::map_rect.h};
// Retângulo padrão de um tile (16x16 pixels)
SDL_Rect AppConfig::tile_rect = {0, 0, 16, 16};

// Teclas de disparo adaptadas para Macbooks (não possuem tecla Ctrl direita)
// Macbook: usa Alt direito; outros: usa Ctrl direito
#if defined(__APPLE__) && defined(__MACH__)
    #define P1_FIRE_KEY SDL_SCANCODE_SPACE
    #define P2_FIRE_KEY SDL_SCANCODE_RALT
#else
    #define P1_FIRE_KEY SDL_SCANCODE_SPACE
    #define P2_FIRE_KEY SDL_SCANCODE_RCTRL
#endif

// Pontos iniciais dos jogadores (posição de respawn)
vector<SDL_Point> AppConfig::player_starting_point =
[]{
    vector<SDL_Point> v;
    v.push_back({128, 384}); // Jogador 1 - Canto inferior esquerdo
    v.push_back({256, 384}); // Jogador 2 - Canto inferior direito
    v.push_back({128, 320}); // Jogador 3 - Canto superior esquerdo
    v.push_back({256, 320}); // Jogador 4 - Canto superior direito
    return v;
}();

// Pontos iniciais dos inimigos (posição de respawn)
vector<SDL_Point> AppConfig::enemy_starting_point =
[]{
    vector<SDL_Point> v;
    v.push_back({1, 1});     // Inimigo 1
    v.push_back({192, 1});   // Inimigo 2
    v.push_back({384, 1});   // Inimigo 3
    return v;
}();

// Layouts de teclado, a reserva de quem não tem controle. A classe Controllers
// distribui: controles primeiro, depois estes layouts na ordem, para os
// jogadores que ficaram sem controle. Sem jogador precisando, o layout 0
// fica com o Jogador 1 e o layout 1 com o Jogador 2.
vector<Player::PlayerKeys> AppConfig::keyboard_layouts =
[]{
    vector<Player::PlayerKeys> v;
    // W, S, A, D + Espaço
    v.push_back(Player::PlayerKeys(SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, P1_FIRE_KEY, "WASD"));
    // Setas + Ctrl direito (Alt direito no Mac)
    v.push_back(Player::PlayerKeys(SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, P2_FIRE_KEY, "ARROWS"));
    return v;
}();

// Tempo de espera (ms) antes do início da fase
unsigned AppConfig::level_start_time = 2000;
// Tempo de "escorregão" no gelo (ms)
unsigned AppConfig::slip_time = 380;
// Deslize máximo (px) para contornar quinas de paredes
int AppConfig::tank_corner_slide_max = 10;
// Quantidade total de inimigos por fase
unsigned AppConfig::enemy_start_count = 20;
// Tempo de espera (ms) para o próximo inimigo aparecer
unsigned AppConfig::enemy_redy_time = 500;
// Quantidade máxima de projéteis do jogador simultâneos
unsigned AppConfig::player_bullet_max_size = 1;
// Tempo de exibição da pontuação (ms)
unsigned AppConfig::score_show_time = 3000;
// Tempo de exibição do bônus (ms)
unsigned AppConfig::bonus_show_time = 10000;
// Duração do escudo do tanque (ms)
unsigned AppConfig::tank_shield_time = 10000;
// Duração do congelamento dos tanques (ms)
unsigned AppConfig::tank_frozen_time = 8000;
// Tempo de exibição da tela de fim de fase (ms)
unsigned AppConfig::level_end_time = 5000;
// Duração da proteção da águia (ms)
unsigned AppConfig::protect_eagle_time = 15000;
// Intervalo de piscar do bônus (ms)
unsigned AppConfig::bonus_blink_time = 350;
// Tempo de recarga do disparo do jogador (ms)
unsigned AppConfig::player_reload_time = 120;
// Quantidade máxima de inimigos simultâneos no mapa
int AppConfig::enemy_max_count_on_map = 4;
// Velocidade de entrada do texto "Game Over"
double AppConfig::game_over_entry_speed = 0.13;
// Velocidade padrão dos tanques
double AppConfig::tank_default_speed = 0.08;
// Velocidade padrão dos projéteis
double AppConfig::bullet_default_speed = 0.23;

// ======================== Modo duelo ========================
// Pontos de renascimento de cada equipe (A embaixo, B em cima), na ordem em
// que as vagas são preenchidas: primeiro ao lado da base, depois nas pontas.
vector<vector<SDL_Point>> AppConfig::duel_spawn_points =
{
    {{128, 384}, {256, 384}, {64, 384}, {320, 384}}, // Equipe A
    {{128, 0},   {256, 0},   {64, 0},   {320, 0}},   // Equipe B
};
// Cor de cada equipe: tinge os bots e os textos (A amarela, B verde, como P1 e P2 no original)
vector<SDL_Color> AppConfig::duel_team_colors = {{255, 210, 60, 255}, {90, 230, 90, 255}};
// Vidas de cada jogador em uma equipe do tamanho da maior equipe
int AppConfig::duel_tank_lives = 3;
// Rodadas vencidas necessárias para ganhar a partida (melhor de 3)
int AppConfig::duel_rounds_to_win = 2;
// Tempo (ms) com o mapa sem bônus até aparecer o próximo
unsigned AppConfig::duel_bonus_interval = 10000;
// Reforços (bots aliados do bônus de tanque) em campo ao mesmo tempo, por equipe
int AppConfig::duel_max_allies = 2;
// Duração (ms) do relógio no duelo: só imobiliza, e por menos tempo que na campanha
unsigned AppConfig::duel_freeze_time = 4000;

// Exibe ou não o alvo do inimigo (debug)
bool AppConfig::show_enemy_target = false;
