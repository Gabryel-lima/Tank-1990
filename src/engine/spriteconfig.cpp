#include "spriteconfig.h"

// Construtor da classe SpriteConfig
// Inicializa o mapa de configurações de sprites com os dados de cada tipo de sprite utilizado no jogo.
// Cada chamada de insert adiciona um SpriteType com suas propriedades:
// (tipo, x, y, largura, altura, frames, frame_delay, animado)
SpriteConfig::SpriteConfig()
{
    // Tanques inimigos
    insert(ST_TANK_A, 128, 0, 32, 32, 2, 100, true);      // Tanque tipo A
    insert(ST_TANK_B, 128, 64, 32, 32, 2, 100, true);     // Tanque tipo B
    insert(ST_TANK_C, 128, 128, 32, 32, 2, 100, true);    // Tanque tipo C
    insert(ST_TANK_D, 128, 192, 32, 32, 2, 100, true);    // Tanque tipo D

    // Jogadores
    insert(ST_PLAYER_1, 640, 0, 32, 32, 2, 50, true);     // Jogador 1
    insert(ST_PLAYER_2, 768, 0, 32, 32, 2, 50, true);     // Jogador 2
    insert(ST_PLAYER_3, 768, 0, 32, 32, 2, 50, true);     // Jogador 3 mesma sprite do player 2

    // Elementos do cenário
    insert(ST_BRICK_WALL, 928, 0, 16, 16, 1, 200, false);     // Parede de tijolo
    insert(ST_STONE_WALL, 928, 144, 16, 16, 1, 200, false);   // Parede de pedra
    insert(ST_WATER, 928, 160, 16, 16, 2, 350, true);         // Água (animada)
    insert(ST_BUSH, 928, 192, 16, 16, 1, 200, false);         // Arbusto
    insert(ST_ICE, 928, 208, 16, 16, 1, 200, false);          // Gelo

    // Itens bônus
    insert(ST_BONUS_GRENADE, 896, 0, 32, 32, 1, 200, false);      // Granada
    insert(ST_BONUS_HELMET, 896, 32, 32, 32, 1, 200, false);      // Capacete
    insert(ST_BONUS_CLOCK, 896, 64, 32, 32, 1, 200, false);       // Relógio
    insert(ST_BONUS_SHOVEL, 896, 96, 32, 32, 1, 200, false);      // Pá
    insert(ST_BONUS_TANK, 896, 128, 32, 32, 1, 200, false);       // Tanque extra
    insert(ST_BONUS_STAR, 896, 160, 32, 32, 1, 200, false);       // Estrela
    insert(ST_BONUS_GUN, 896, 192, 32, 32, 1, 200, false);        // Canhão
    insert(ST_BONUS_BOAT, 896, 224, 32, 32, 1, 200, false);       // Barco

    // Efeitos e animações
    insert(ST_SHIELD, 976, 0, 32, 32, 2, 45, true);               // Escudo (animado)
    insert(ST_CREATE, 1008, 0, 32, 32, 10, 100, false);           // Animação de criação
    insert(ST_DESTROY_TANK, 1040, 0, 64, 64, 7, 70, false);       // Explosão de tanque
    insert(ST_DESTROY_BULLET, 1108, 0, 32, 32, 5, 40, false);     // Explosão de tiro
    insert(ST_BOAT_P1, 944, 96, 32, 32, 1, 200, false);           // Barco do jogador 1
    insert(ST_BOAT_P2, 976, 96, 32, 32, 1, 200, false);           // Barco do jogador 2

    // Objetivos e status
    insert(ST_EAGLE, 944, 0, 32, 32, 1, 200, false);              // Águia (base)
    insert(ST_DESTROY_EAGLE, 1040, 0, 64, 64, 7, 100, false);     // Explosão da águia
    insert(ST_FLAG, 944, 32, 32, 32, 1, 200, false);              // Bandeira

    // Projéteis e status
    insert(ST_BULLET, 944, 128, 8, 8, 1, 200, false);             // Tiro

    // Indicadores e status
    insert(ST_LEFT_ENEMY, 944, 144, 16, 16, 1, 200, false);       // Indicador de inimigos restantes
    insert(ST_STAGE_STATUS, 976, 64, 32, 32, 1, 200, false);      // Status da fase

    // Logo do jogo
    insert(ST_TANKS_LOGO, 0, 260, 406, 72, 1, 200, false);        // Logo do jogo
}

// Retorna um ponteiro constante para os dados do sprite correspondente ao tipo informado.
// Utiliza o método at() do std::map, que lança exceção se o tipo não existir.
const SpriteData* SpriteConfig::getSpriteData(SpriteType st) const
{
    return &m_configs.at(st);
}

// Insere uma nova configuração de sprite no mapa m_configs.
// Parâmetros:
//   st  - tipo do sprite
//   x,y - posição do sprite na textura
//   w,h - largura e altura do sprite
//   fc  - quantidade de frames de animação
//   fd  - delay entre frames (ms)
//   l   - indica se o sprite é animado (loop)
void SpriteConfig::insert(SpriteType st, int x, int y, int w, int h, int fc, int fd, bool l)
{
    m_configs[st] = SpriteData(x, y, w, h, fc, fd, l);
}
