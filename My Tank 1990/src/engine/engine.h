#ifndef ENGINE_H
#define ENGINE_H

#include "renderer.h"
#include "spriteconfig.h"
#include <string>

/**
 * @brief Classe que conecta os elementos principais do funcionamento do jogo.
 */
class Engine {
public:
    Engine();
    /**
     * @return Instância estática do objeto Engine (singleton).
     */
    static Engine& getEngine();
    /**
     * Converte um valor inteiro para string.
     * @param num - valor inteiro
     * @return valor convertido em string
     */
    static std::string intToString(int num);
    /**
     * Inicializa os módulos do engine (renderer, sprites, etc).
     */
    void initModules();
    /**
     * Destroi os módulos do engine.
     */
    void destroyModules();
    /**
     * @return ponteiro para o objeto Renderer responsável pelo desenho na tela
     */
    Renderer* getRenderer() const;
    /**
     * @return ponteiro para o objeto SpriteConfig com as informações das sprites
     */
    SpriteConfig* getSpriteConfig() const;
private:
    Renderer* m_renderer;
    SpriteConfig* m_sprite_config;
};

#endif // ENGINE_H 