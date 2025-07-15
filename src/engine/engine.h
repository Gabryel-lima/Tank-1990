#ifndef ENGINE_H
#define ENGINE_H

#include "renderer.h"
#include "spriteconfig.h"

/**
 * @brief Classe responsável por integrar e gerenciar os principais módulos do motor do jogo.
 * Permite acesso centralizado ao renderizador e à configuração de sprites.
 */
class Engine
{
public:
    /**
     * Construtor padrão: inicializa os ponteiros dos módulos como nulos.
     */
    Engine();

    /**
     * Retorna uma referência estática para a instância única da Engine (Singleton).
     * Permite que qualquer módulo acesse a engine sem necessidade de ponteiro global.
     * @return Referência para a instância única de Engine.
     */
    static Engine& getEngine();

    /**
     * Converte um valor inteiro para uma string.
     * @param num Valor inteiro a ser convertido.
     * @return Representação textual do número inteiro.
     */
    static std::string intToString(int num);

    /**
     * Inicializa os módulos principais da engine (renderizador e configuração de sprites).
     * Deve ser chamada antes de usar o renderizador ou sprites.
     */
    void initModules();

    /**
     * Libera a memória dos módulos principais e zera os ponteiros.
     * Deve ser chamada ao finalizar o programa para evitar vazamentos de memória.
     */
    void destroyModules();

    /**
     * Retorna o ponteiro para o renderizador, responsável pelo desenho na tela.
     * @return Ponteiro para o objeto Renderer.
     */
    Renderer* getRenderer() const;

    /**
     * Retorna o ponteiro para a configuração de sprites, que armazena informações sobre as texturas.
     * @return Ponteiro para o objeto SpriteConfig.
     */
    SpriteConfig* getSpriteConfig() const;

private:
    Renderer* m_renderer;             ///< Ponteiro para o renderizador gráfico.
    SpriteConfig* m_sprite_config;    ///< Ponteiro para a configuração dos sprites.
};

#endif // ENGINE_H
