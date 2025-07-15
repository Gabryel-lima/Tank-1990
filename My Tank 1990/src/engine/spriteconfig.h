#ifndef SPRITECONFIG_H
#define SPRITECONFIG_H

#include "../type.h"
#include <map>
#include <SDL2/SDL.h>

/**
 * @brief Estrutura que armazena informações sobre um tipo de animação de objeto.
 */
struct SpriteData {
    SpriteData() { rect.x = 0; rect.y = 0; rect.w = 0; rect.h = 0; frames_count = 0; frame_duration = 0; loop = false; }
    SpriteData(int x, int y, int w, int h, int fc, int fd, bool l) {
        rect.x = x; rect.y = y; rect.w = w; rect.h = h; frames_count = fc; frame_duration = fd; loop = l;
    }
    /**
     * Posição e dimensões do primeiro frame da animação
     */
    SDL_Rect rect;
    /**
     * Quantidade de frames na animação
     */
    int frames_count;
    /**
     * Duração de cada frame em milissegundos
     */
    unsigned frame_duration;
    /**
     * Indica se a animação é em loop
     */
    bool loop;
};

/**
 * @brief Classe que armazena todas as informações de animação do jogo.
 */
class SpriteConfig {
public:
    /**
     * O construtor adiciona as animações via SpriteConfig::insert.
     */
    SpriteConfig();
    /**
     * Obtém os dados de uma animação específica.
     * @param sp - tipo de sprite
     * @return ponteiro para os dados da animação
     */
    const SpriteData* getSpriteData(SpriteType sp) const;
private:
    std::map<SpriteType, SpriteData> m_configs;
    /**
     * Adiciona um novo tipo de animação.
     */
    void insert(SpriteType st, int x, int y, int w, int h, int fc, int fd, bool l);
};

#endif // SPRITECONFIG_H 