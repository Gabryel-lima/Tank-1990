#ifndef OBJECT_H
#define OBJECT_H

#include "../engine/engine.h"

/**
 * @brief Classe base para todos os objetos do jogo.
 */
class Object {
public:
    /**
     * Cria um objeto na posição (0, 0).
     */
    Object();
    /**
     * Cria um objeto.
     * @param x - posição horizontal inicial
     * @param y - posição vertical inicial
     * @param type - tipo do objeto
     */
    Object(double x, double y, SpriteType type);
    /**
     * Cria um objeto com dados de sprite customizados.
     * @param x - posição horizontal inicial
     * @param y - posição vertical inicial
     * @param sprite - dados da animação
     */
    Object(double x, double y, const SpriteData* sprite);
    virtual ~Object();

    /**
     * Desenha o objeto na tela.
     */
    virtual void draw();
    /**
     * Atualiza o estado do objeto (animação, posição, etc).
     * @param dt - tempo desde a última atualização
     */
    virtual void update(Uint32 dt);

    /**
     * Indica se o objeto deve ser removido do jogo.
     */
    bool to_erase;
    /**
     * Retângulo de colisão (pode ser menor que o retângulo de desenho).
     */
    SDL_Rect collision_rect;
    /**
     * Retângulo de destino na tela.
     */
    SDL_Rect dest_rect;
    /**
     * Retângulo fonte na textura.
     */
    SDL_Rect src_rect;
    /**
     * Tipo do objeto.
     */
    SpriteType type;
    /**
     * Posição exata (float) do objeto.
     */
    double pos_x, pos_y;
protected:
    /**
     * Move um retângulo base por múltiplos do seu tamanho.
     */
    SDL_Rect moveRect(const SDL_Rect &rect, int x, int y);
    /**
     * Dados da animação do objeto.
     */
    const SpriteData* m_sprite;
    /**
     * Tempo de exibição do frame atual.
     */
    Uint32 m_frame_display_time;
    /**
     * Frame atual da animação.
     */
    int m_current_frame;
};

/**
 * Calcula a interseção entre dois retângulos.
 */
SDL_Rect intersectRect(SDL_Rect* rect1, SDL_Rect* rect2);

#endif // OBJECT_H 