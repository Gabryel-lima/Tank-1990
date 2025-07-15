#include "engine.h"

// Construtor da Engine: inicializa ponteiros dos módulos principais como nulos
Engine::Engine()
{
    m_renderer = nullptr;         // Ponteiro para o renderizador (responsável pelo desenho na tela)
    m_sprite_config = nullptr;    // Ponteiro para a configuração dos sprites
}

// Retorna uma referência estática para a instância única da Engine (Singleton)
Engine &Engine::getEngine()
{
    static Engine engine; // Instância única da Engine
    return engine;
}

// Converte um número inteiro para string (base decimal)
// Suporta números negativos
std::string Engine::intToString(int num)
{
    if(num == 0) return "0"; // Caso especial para zero

    std::string buf;
    bool negative = false;
    if(num < 0)
    {
        negative = true; // Marca se o número é negativo
        num = -num;
    }
    // Constrói a string do número, dígito a dígito (da direita para a esquerda)
    for(; num; num /= 10) buf = "0123456789abcdef"[num % 10] + buf;
    if(negative) buf = "-" + buf; // Adiciona o sinal negativo, se necessário
    return buf;
}

// Inicializa os módulos principais da engine (renderizador e configuração de sprites)
void Engine::initModules()
{
    m_renderer = new Renderer;           // Cria o renderizador
    m_sprite_config = new SpriteConfig;  // Cria a configuração de sprites
}

// Libera a memória dos módulos principais e zera os ponteiros
void Engine::destroyModules()
{
    delete m_renderer;         // Libera o renderizador
    m_renderer = nullptr;
    delete m_sprite_config;    // Libera a configuração de sprites
    m_sprite_config = nullptr;
}

// Retorna o ponteiro para o renderizador
Renderer *Engine::getRenderer() const
{
    return m_renderer;
}

// Retorna o ponteiro para a configuração de sprites
SpriteConfig *Engine::getSpriteConfig() const
{
    return m_sprite_config;
}
