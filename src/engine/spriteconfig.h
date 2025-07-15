#ifndef SPRITECONFIG_H
#define SPRITECONFIG_H

#include "../type.h"
#include <map>
#include <SDL2/SDL.h>

/**
 * @brief
 * Estrutura que armazena informações sobre um tipo de animação de sprite.
 * Cada SpriteData representa uma animação específica (ex: tanque, explosão, item).
 */
struct SpriteData
{
    // Construtor padrão: inicializa todos os campos com valores neutros
    SpriteData() { rect.x = 0; rect.y = 0; rect.w = 0; rect.h = 0; frames_count = 0; frame_duration = 0; loop = false;}
    // Construtor parametrizado: define todos os campos de acordo com os argumentos
    SpriteData(int x, int y, int w, int h, int fc, int fd, bool l)
        { rect.x = x; rect.y = y; rect.w = w; rect.h = h; frames_count = fc; frame_duration = fd; loop = l;}

    /**
     * Posição e dimensões do retângulo da primeira frame da animação na textura.
     * (x, y) = canto superior esquerdo na textura.
     * w, h = largura e altura de cada frame.
     */
    SDL_Rect rect;
    /**
     * Quantidade de frames (quadros) na animação.
     */
    int frames_count;
    /**
     * Duração de cada frame em milissegundos.
     */
    unsigned frame_duration;
    /**
     * Indica se a animação é em loop (verdadeiro) ou executa apenas uma vez (falso).
     */
    bool loop;
};

/**
 * @brief
 * Classe responsável por armazenar e fornecer informações sobre todos os tipos de animação do jogo.
 * Permite consultar rapidamente os dados de cada sprite/objeto animado.
 */
class SpriteConfig
{
public:
    /**
     * O construtor adiciona todas as animações utilizadas no jogo,
     * utilizando a função privada SpriteConfig::insert.
     */
    SpriteConfig();

    /**
     * Retorna um ponteiro para os dados da animação do tipo solicitado.
     * @param sp - tipo de sprite/objeto animado desejado
     * @return ponteiro constante para SpriteData correspondente
     */
    const SpriteData* getSpriteData(SpriteType sp) const;
private:
    /**
     * Mapa que armazena todos os tipos de animação, indexados pelo tipo SpriteType.
     * Permite busca eficiente dos dados de cada sprite.
     */
    std::map<SpriteType, SpriteData> m_configs;

    /**
     * Função auxiliar para adicionar um novo tipo de animação ao mapa.
     * @param st - tipo de sprite/objeto animado
     * @param x - posição horizontal da primeira frame na textura
     * @param y - posição vertical da primeira frame na textura
     * @param w - largura de cada frame
     * @param h - altura de cada frame
     * @param fc - quantidade de frames na animação
     * @param fd - duração de cada frame (ms)
     * @param l - se a animação é em loop (true) ou não (false)
     */
    void insert(SpriteType st , int x, int y, int w, int h, int fc, int fd, bool l);
};

#endif // SPRITECONFIG_H
