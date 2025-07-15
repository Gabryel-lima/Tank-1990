#ifndef OBJECT_H
#define OBJECT_H

#include "../engine/engine.h"

/**
 * @brief
 * Classe base para todos os objetos do jogo.
 * Fornece interface e atributos comuns para renderização, atualização e colisão.
 */
class Object
{
public:
    /**
     * Construtor padrão.
     * Cria o objeto na posição (0, 0).
     */
    Object();

    /**
     * Construtor parametrizado.
     * Cria o objeto em uma posição específica e com um tipo de sprite.
     * @param x - posição horizontal inicial
     * @param y - posição vertical inicial
     * @param type - tipo do sprite/objeto
     */
    Object(double x, double y, SpriteType type);

    /**
     * Construtor alternativo.
     * Cria o objeto em uma posição específica com um ponteiro para SpriteData.
     * @param x - posição horizontal inicial
     * @param y - posição vertical inicial
     * @param sprite - ponteiro para os dados de animação do sprite
     */
    Object(double x, double y, const SpriteData* sprite);

    /**
     * Destrutor virtual.
     * Garante destruição correta em herança.
     */
    virtual ~Object();

    /**
     * Desenha o objeto na tela.
     * Utiliza o método drawObject da classe Renderer, desenhando a textura definida por src_rect na área dest_rect.
     * Pode ser sobrescrito por classes derivadas.
     */
    virtual void draw();

    /**
     * Atualiza o estado do objeto.
     * Atualiza dest_rect com base em pos_x e pos_y, gerencia animação (tempo de exibição de cada frame).
     * @param dt - tempo (em ms) desde a última chamada, usado para controlar a animação
     */
    virtual void update(Uint32 dt);

    /**
     * Indica se o objeto deve ser removido do jogo.
     * Se for true, o objeto não será atualizado nem desenhado.
     */
    bool to_erase;

    /**
     * Retângulo de colisão do objeto.
     * Pode ser menor que dest_rect para ajustar a área de colisão real.
     */
    SDL_Rect collision_rect;

    /**
     * Retângulo de destino na tela onde o objeto será desenhado.
     * Define a posição e tamanho do objeto no mundo do jogo.
     */
    SDL_Rect dest_rect;

    /**
     * Retângulo de origem na textura do sprite.
     * Define qual parte da textura será desenhada (frame atual da animação).
     */
    SDL_Rect src_rect;

    /**
     * Tipo do objeto (enum SpriteType).
     * Usado para identificar o tipo de sprite/objeto.
     */
    SpriteType type;

    /**
     * Posição horizontal precisa do objeto (em pixels ou unidades do mundo).
     */
    double pos_x;

    /**
     * Posição vertical precisa do objeto (em pixels ou unidades do mundo).
     */
    double pos_y;

protected:
    /**
     * Retorna um retângulo deslocado a partir de rect, útil para animação de sprites.
     * @param rect - retângulo base
     * @param x - deslocamento horizontal (em múltiplos do tamanho do frame)
     * @param y - deslocamento vertical (em múltiplos do tamanho do frame)
     * @return retângulo deslocado
     */
    SDL_Rect moveRect(const SDL_Rect &rect, int x, int y);

    /**
     * Ponteiro para os dados de animação do sprite associado ao objeto.
     */
    const SpriteData* m_sprite;

    /**
     * Tempo acumulado de exibição do frame atual da animação (em ms).
     */
    Uint32 m_frame_display_time;

    /**
     * Índice do frame atual da animação.
     */
    int m_current_frame;
};

/**
 * Calcula a interseção (área comum) entre dois retângulos.
 * @param rect1 - ponteiro para o primeiro retângulo
 * @param rect2 - ponteiro para o segundo retângulo
 * @return retângulo representando a área comum; se não houver interseção, o retângulo terá dimensões negativas
 */
SDL_Rect intersectRect(SDL_Rect* rect1, SDL_Rect* rect2);

#endif // OBJECT_H
