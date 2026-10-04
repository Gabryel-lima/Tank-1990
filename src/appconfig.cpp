#include "appconfig.h"

// Caminho do arquivo de textura principal do jogo
string AppConfig::texture_path = "texture.png";
// Caminho da pasta onde estão os arquivos de fases/níveis
string AppConfig::levels_path = "levels/";
// Caminho da pasta dos mapas do modo duelo (separados da campanha)
string AppConfig::duel_levels_path = "duel_levels/";
// Mapas do duelo: arquivo dentro de duel_levels_path e nome mostrado no menu.
// Todos espelhados na horizontal e na vertical, com as bases, muralhas, pontos
// de renascimento e pontos de bônus nas mesmas posições.
vector<pair<string, string>> AppConfig::duel_maps =
{
    {"1", "Arena"},
    {"2", "Fortress"},
    {"3", "River"},
    {"4", "Maze"},
    {"5", "Open Field"},
    {"6", "Crossroads"},
    {"7", "Archipelago"},
    {"8", "Bunkers"},
    {"9", "Frozen Lake"},
    {"10", "Gauntlet"},
};
// Caminho da pasta dos mapas do modo sobrevivência e a lista padrão (a real vem de
// survival_levels/maps.txt, ver SurvivalLayout::loadMapList)
string AppConfig::survival_levels_path = "survival_levels/";
vector<pair<string, string>> AppConfig::survival_maps =
{
    {"1", "Classic"},
    {"2", "Trenches"},
    {"3", "Canyon"},
    {"4", "Forest"},
    {"5", "Ice Rink"},
    {"6", "Citadel"},
    {"7", "Labyrinth"},
    {"8", "Islands"},
    {"9", "Crossfire"},
    {"10", "Last Stand"},
};
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
    // W, S, A, D + Espaço (tiro) + Shift esquerdo (poder dos modos extras)
    v.push_back(Player::PlayerKeys(SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, P1_FIRE_KEY, SDL_SCANCODE_LSHIFT, "WASD"));
    // Setas + Ctrl direito (Alt direito no Mac) + Shift direito (poder dos modos extras)
    v.push_back(Player::PlayerKeys(SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, P2_FIRE_KEY, SDL_SCANCODE_RSHIFT, "ARROWS"));
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
// Tempo (ms) que o "GAME OVER" fica parado no centro antes da tela de pontuação
unsigned AppConfig::game_over_hold_time = 2500;
// Intervalo (ms) do piscar do "PAUSE"
unsigned AppConfig::pause_blink_time = 400;
// Velocidade padrão dos tanques
double AppConfig::tank_default_speed = 0.08;
// Velocidade padrão dos projéteis
double AppConfig::bullet_default_speed = 0.23;

// ======================== Modo duelo ========================
// Colunas (x) de nascimento da equipe A, em ordem de preferência, alternando os
// lados da base: interna esquerda, externa direita, externa esquerda, interna direita.
// A equipe B usa o espelho em ponto (x -> largura do mapa - 32 - x): interna direita,
// externa esquerda... Com até 4 jogadores e 4 colunas, cada jogador recebe uma coluna
// só dele (ver Duel::assignSpawns), então ninguém nasce na linha de tiro de outro.
vector<int> AppConfig::duel_spawn_columns = {128, 320, 64, 256};
// Linha (y) de nascimento de cada equipe: A embaixo, B em cima
vector<int> AppConfig::duel_spawn_rows = {384, 0};
// Vidas de cada jogador em uma equipe do tamanho da maior equipe
int AppConfig::duel_tank_lives = 3;
// Rodadas vencidas necessárias para ganhar a partida (melhor de 3)
int AppConfig::duel_rounds_to_win = 2;
// Tempo (ms) com o mapa sem bônus até aparecer o próximo
unsigned AppConfig::duel_bonus_interval = 10000;
// Duração (ms) do escudo do capacete no duelo (na campanha, tank_shield_time = 10 s).
// Com 10 s dava para atravessar o mapa e destruir a base sem risco: na simulação
// (tools/duel_sim), quem pegava o capacete cinza no 1 contra 1 vencia 78% das rodadas
// (65% com 6 s)
unsigned AppConfig::duel_helmet_time = 6000;
// Com 3 estrelas, um tiro só tira uma estrela em vez de destruir o tanque? No duelo, não:
// essa "armadura" fazia do canhão uma vida extra que ainda quebra pedra (quem pegava
// vencia 70-74% das rodadas; sem ela, 49-68%). O barco e o capacete continuam protegendo
bool AppConfig::duel_star_armor = false;
// A equipe menor ganha base de pedra se a outra tiver pelo menos esta proporção de tanques
int AppConfig::duel_stone_wall_ratio = 2;
// Chance de um bônus surgir cinza (qualquer equipe pega); no resto das vezes ele
// surge na cor de uma equipe e só ela pega
double AppConfig::duel_neutral_bonus_chance = 0.35;
// Chance de um bônus de equipe surgir na metade do mapa do adversário
// (no resto das vezes, surge na metade da própria equipe)
double AppConfig::duel_team_bonus_enemy_side_chance = 0.7;
// Reforços (bots aliados do bônus de tanque) em campo ao mesmo tempo, por equipe
int AppConfig::duel_max_allies = 2;
// Cadência máxima de tiro de cada jogador no duelo (tiros por segundo). Na campanha
// o limite é AppConfig::player_reload_time (até ~8 tiros/s com 3 estrelas); no duelo,
// isso derrubava uma base em menos de 1 s
double AppConfig::duel_max_shots_per_second = 3.0;
// Duração (ms) do relógio no duelo: só imobiliza, e por menos tempo que na campanha
unsigned AppConfig::duel_freeze_time = 4000;

// Poderes dos modos extras (ver Powers)
unsigned AppConfig::power_turret_time = 20000;
unsigned AppConfig::power_turret_reload = 700;
int AppConfig::power_turret_ammo = 10;
int AppConfig::duel_demolisher_stars = 1;
int AppConfig::power_turret_range = 12;
unsigned AppConfig::power_mine_time = 30000;
unsigned AppConfig::power_turbo_time = 8000;
double AppConfig::power_turbo_factor = 1.5;
unsigned AppConfig::power_truce_time = 10000;
bool AppConfig::survival_store_powers = true;

// Modo sobrevivência: ondas cada vez maiores, com inimigos mais blindados (a dificuldade
// segue a escala das fases da campanha: onda N ~ fase 2N + 1, até a 35)
int AppConfig::survival_first_wave_enemies = 6;
int AppConfig::survival_wave_enemy_step = 2;
int AppConfig::survival_max_wave_enemies = 40;
int AppConfig::survival_first_on_map = 4;
int AppConfig::survival_on_map_every_waves = 3;
int AppConfig::survival_max_on_map = 10;
unsigned AppConfig::survival_first_spawn_delay = 1500;
unsigned AppConfig::survival_min_spawn_delay = 500;
unsigned AppConfig::survival_wave_intro_time = 3000;
int AppConfig::survival_life_every_waves = 5;

// Exibe ou não o alvo do inimigo (debug)
bool AppConfig::show_enemy_target = false;
