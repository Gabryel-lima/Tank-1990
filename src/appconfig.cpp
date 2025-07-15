#include "appconfig.h"

// Caminho do arquivo de textura principal do jogo
string AppConfig::texture_path = "texture.png";
// Caminho da pasta onde estão os arquivos de fases/níveis
string AppConfig::levels_path = "levels/";
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
// Macbook: usa Alt direito/esquerdo; outros: usa Ctrl direito/esquerdo
#if defined(__APPLE__) && defined(__MACH__)
    #define P1_FIRE_KEY SDL_SCANCODE_SPACE
    #define P2_FIRE_KEY SDL_SCANCODE_RALT
    #define P3_FIRE_KEY SDL_SCANCODE_N
#else
    #define P1_FIRE_KEY SDL_SCANCODE_SPACE
    #define P2_FIRE_KEY SDL_SCANCODE_RCTRL
    #define P3_FIRE_KEY SDL_SCANCODE_N
#endif

// Pontos iniciais dos jogadores (posição de respawn)
vector<SDL_Point> AppConfig::player_starting_point =
[]{
    vector<SDL_Point> v;
    v.push_back({128, 384}); // Jogador 1
    v.push_back({256, 384}); // Jogador 2
    v.push_back({256, 320}); // Jogador 3
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

// Configuração das teclas de controle dos jogadores
vector<Player::PlayerKeys> AppConfig::player_keys =
[]{
    vector<Player::PlayerKeys> v;
    // Jogador 1: setas + tecla de disparo
    v.push_back({SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, P1_FIRE_KEY});
    // Jogador 2: WASD + tecla de disparo
    v.push_back({SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, P2_FIRE_KEY});
    // Jogador 3: IJKL + tecla de disparo 
    v.push_back({SDL_SCANCODE_I, SDL_SCANCODE_K, SDL_SCANCODE_J, SDL_SCANCODE_L, P3_FIRE_KEY});
    return v;
}();

// Tempo de espera (ms) antes do início da fase
unsigned AppConfig::level_start_time = 2000;
// Tempo de "escorregão" no gelo (ms)
unsigned AppConfig::slip_time = 380;
// Quantidade total de inimigos por fase
unsigned AppConfig::enemy_start_count = 20;
// Tempo de espera (ms) para o próximo inimigo aparecer
unsigned AppConfig::enemy_redy_time = 500;
// Quantidade máxima de projéteis do jogador simultâneos
unsigned AppConfig::player_bullet_max_size = 2;
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
// Exibe ou não o alvo do inimigo (debug)
bool AppConfig::show_enemy_target = false;
