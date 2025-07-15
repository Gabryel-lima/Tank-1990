#include "appconfig.h" // Ok

// Caminho para a textura dos objetos do jogo
string AppConfig::texture_path = "texture.png";

// Caminho para o diretório dos níveis
string AppConfig::levels_path = "levels/";

// Caminho para a fonte utilizada no jogo
string AppConfig::font_name = "prstartk.ttf";

// Texto exibido quando o jogador perde
string AppConfig::game_over_text = "GAME OVER";

// Retângulo que define a área do mapa do jogo (tabuleiro)
SDL_Rect AppConfig::map_rect = {0, 0, 26 * 16, 26 * 16};

// Retângulo que define a área de status ao lado do mapa
SDL_Rect AppConfig::status_rect = {26 * 16, 0, 3 * 16, AppConfig::map_rect.h};

// Retângulo que define o tamanho total da janela da aplicação
SDL_Rect AppConfig::windows_rect = {0, 0, AppConfig::map_rect.w + AppConfig::status_rect.w, AppConfig::map_rect.h};

// Retângulo que define o tamanho de cada célula (tile) do mapa
SDL_Rect AppConfig::tile_rect = {0, 0, 16, 16};

// Definição das teclas de disparo para cada jogador, considerando compatibilidade com MacOS
// Macbook padrão não possui tecla Ctrl direita
#if defined(__APPLE__) && defined(__MACH__)
    #define P1_FIRE_KEY SDL_SCANCODE_RALT
    #define P2_FIRE_KEY SDL_SCANCODE_LALT
#else
    #define P1_FIRE_KEY SDL_SCANCODE_RCTRL
    #define P2_FIRE_KEY SDL_SCANCODE_LCTRL
#endif

// Posições iniciais dos jogadores no mapa
vector<SDL_Point> AppConfig::player_starting_point = []{
    vector<SDL_Point> v;
    v.push_back({128, 384}); // Jogador 1
    v.push_back({256, 384}); // Jogador 2
    return v;
}();

// Posições iniciais dos inimigos no mapa
vector<SDL_Point> AppConfig::enemy_starting_point = []{
    vector<SDL_Point> v;
    v.push_back({1, 1});     // Inimigo 1
    v.push_back({192, 1});   // Inimigo 2
    v.push_back({384, 1});   // Inimigo 3
    return v;
}();

// Teclas de controle para cada jogador
vector<Player::PlayerKeys> AppConfig::player_keys = []{
    vector<Player::PlayerKeys> v;
    // Jogador 1: setas direcionais + tecla de disparo
    v.push_back({SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, P1_FIRE_KEY});
    // Jogador 2: teclas WASD + tecla de disparo
    v.push_back({SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, P2_FIRE_KEY});
    return v;
}();

// Tempo de exibição do número da rodada ao iniciar (ms)
unsigned AppConfig::level_start_time = 2000;

// Tempo de deslizamento no gelo (ms)
unsigned AppConfig::slip_time = 380;

// Quantidade de inimigos por rodada
unsigned AppConfig::enemy_start_count = 20;

// Tempo para surgimento de novo inimigo (ms)
unsigned AppConfig::enemy_redy_time = 500;

// Máximo de projéteis que o jogador pode disparar simultaneamente
unsigned AppConfig::player_bullet_max_size = 2;

// Tempo de exibição dos pontos após contagem (ms)
unsigned AppConfig::score_show_time = 3000;

// Tempo de exibição do bônus no mapa (ms)
unsigned AppConfig::bonus_show_time = 10000;

// Duração do escudo do tanque (ms)
unsigned AppConfig::tank_shield_time = 10000;

// Duração do congelamento dos inimigos (ms)
unsigned AppConfig::tank_frozen_time = 8000;

// Tempo entre eliminar todos os inimigos e mostrar a tela de resultados (ms)
unsigned AppConfig::level_end_time = 5000;

// Duração da proteção de pedra ao redor da águia (ms)
unsigned AppConfig::protect_eagle_time = 15000;

// Tempo de piscar do bônus (ms)
unsigned AppConfig::bonus_blink_time = 350;

// Tempo mínimo entre disparos do jogador (ms)
unsigned AppConfig::player_reload_time = 120;

// Número máximo de tanques inimigos no mapa ao mesmo tempo
int AppConfig::enemy_max_count_on_map = 4;

// Velocidade de entrada do texto "GAME OVER"
double AppConfig::game_over_entry_speed = 0.13;

// Velocidade padrão do tanque
double AppConfig::tank_default_speed = 0.08;

// Velocidade padrão do projétil
double AppConfig::bullet_default_speed = 0.23;

// Exibir ou não o alvo dos inimigos (debug)
bool AppConfig::show_enemy_target = false;
