#include "renderer.h"
#include "../appconfig.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>

// Construtor: inicializa todos os ponteiros dos recursos gráficos como nulos
Renderer::Renderer()
{
    m_texture = nullptr;
    m_renderer = nullptr;
    m_text_texture = nullptr;
    m_font1 = nullptr;
    m_font2 = nullptr;
    // m_font3 será inicializado em loadFont()
}

// Destrutor: libera todos os recursos gráficos alocados
Renderer::~Renderer()
{
    if(m_renderer != nullptr)
        SDL_DestroyRenderer(m_renderer); // Libera o renderizador SDL
    if(m_texture != nullptr)
        SDL_DestroyTexture(m_texture); // Libera a textura principal
    if(m_text_texture != nullptr)
        SDL_DestroyTexture(m_text_texture); // Libera a textura de texto
    if(m_font1 != nullptr)
        TTF_CloseFont(m_font1); // Fecha fonte 1
    if(m_font2 != nullptr)
        TTF_CloseFont(m_font2); // Fecha fonte 2
    if(m_font3 != nullptr)
        TTF_CloseFont(m_font3); // Fecha fonte 3
}

// Carrega a textura principal do jogo a partir do caminho definido em AppConfig
void Renderer::loadTexture(SDL_Window* window)
{
    SDL_Surface* surface = nullptr;
    // Cria o renderizador associado à janela
    m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Carrega a superfície da imagem da textura
    surface = IMG_Load(AppConfig::texture_path.c_str());

    // Se a superfície e o renderizador foram criados com sucesso, cria a textura
    if(surface != nullptr && m_renderer != nullptr)
        m_texture = SDL_CreateTextureFromSurface(m_renderer, surface);

    // Libera a superfície temporária
    SDL_FreeSurface(surface);
}

// Carrega as fontes utilizadas para renderização de texto
void Renderer::loadFont()
{
    m_font1 = TTF_OpenFont(AppConfig::font_name.c_str(), 28); // Fonte grande
    m_font2 = TTF_OpenFont(AppConfig::font_name.c_str(), 14); // Fonte média
    m_font3 = TTF_OpenFont(AppConfig::font_name.c_str(), 10); // Fonte pequena
}

// Limpa o buffer de renderização com uma cor de fundo padrão
void Renderer::clear()
{
    SDL_SetRenderDrawColor(m_renderer, 110, 110, 110, 255); // Define cor de fundo (cinza)
    SDL_RenderClear(m_renderer); // Limpa o buffer de renderização (back buffer)
}

// Apresenta o conteúdo do buffer de renderização na tela
void Renderer::flush()
{
    SDL_RenderPresent(m_renderer); // Troca os buffers (apresenta o back buffer)
}

// Desenha um objeto (sprite) na tela usando uma região da textura
void Renderer::drawObject(const SDL_Rect *texture_src, const SDL_Rect *window_dest)
{
    SDL_RenderCopy(m_renderer, m_texture, texture_src, window_dest); // Desenha no back buffer
}

// Desenha um objeto (sprite) na tela com uma cor específica aplicada
void Renderer::drawObjectWithColor(const SDL_Rect *texture_src, const SDL_Rect *window_dest, SDL_Color color)
{
    // Salva a cor atual da textura
    Uint8 r, g, b, a;
    SDL_GetTextureColorMod(m_texture, &r, &g, &b);
    SDL_GetTextureAlphaMod(m_texture, &a);
    
    // Aplica a nova cor
    SDL_SetTextureColorMod(m_texture, color.r, color.g, color.b);
    if(color.a != 255)
        SDL_SetTextureAlphaMod(m_texture, color.a);
    
    // Desenha o objeto com a cor aplicada
    SDL_RenderCopy(m_renderer, m_texture, texture_src, window_dest);
    
    // Restaura a cor original
    SDL_SetTextureColorMod(m_texture, r, g, b);
    SDL_SetTextureAlphaMod(m_texture, a);
}

// Define o fator de escala e o viewport do renderizador
void Renderer::setScale(float xs, float ys)
{
    float scale = min(xs, ys);
    if(scale < 0.1) return; // Evita escalas muito pequenas

    SDL_Rect viewport;
    // Calcula o deslocamento para centralizar o conteúdo
    viewport.x = ((float)AppConfig::windows_rect.w / scale - (AppConfig::map_rect.w + AppConfig::status_rect.w)) / 2.0;
    viewport.y = ((float)AppConfig::windows_rect.h / scale - AppConfig::map_rect.h) / 2.0;
    if(viewport.x < 0) viewport.x = 0;
    if(viewport.y < 0) viewport.y = 0;
    viewport.w = AppConfig::map_rect.w + AppConfig::status_rect.w;
    viewport.h = AppConfig::map_rect.h;

    SDL_RenderSetScale(m_renderer, scale, scale); // Aplica o fator de escala
    SDL_RenderSetViewport(m_renderer, &viewport); // Define o viewport
}

// Desenha um texto na tela em uma posição específica, usando a fonte e cor indicadas
void Renderer::drawText(const SDL_Point* start, string text, SDL_Color text_color, int font_size)
{
    // Verifica se as fontes estão carregadas
    if(m_font1 == nullptr || m_font2 == nullptr || m_font3 == nullptr) return;
    // Libera a textura de texto anterior, se existir
    if(m_text_texture != nullptr)
        SDL_DestroyTexture(m_text_texture);

    SDL_Surface* text_surface = nullptr;
    // Seleciona a fonte de acordo com o tamanho solicitado
    if(font_size == 2) 
        text_surface = TTF_RenderText_Solid(m_font2, text.c_str(), text_color);
    else if(font_size == 3) 
        text_surface = TTF_RenderText_Solid(m_font3, text.c_str(), text_color);
    else 
        text_surface = TTF_RenderText_Solid(m_font1, text.c_str(), text_color);

    if(text_surface == nullptr) return; // Falha ao criar superfície de texto

    // Cria a textura de texto a partir da superfície
    m_text_texture = SDL_CreateTextureFromSurface(m_renderer, text_surface);
    if(m_text_texture == nullptr) return; // Falha ao criar textura

    SDL_Rect window_dest;
    // Calcula a posição do texto: centralizado se start for nulo ou negativo
    if(start == nullptr)
    {
        window_dest.x = (AppConfig::map_rect.w + AppConfig::status_rect.w - text_surface->w)/2;
        window_dest.y = (AppConfig::map_rect.h - text_surface->h)/2;
    }
    else
    {
        if(start->x < 0) 
            window_dest.x = (AppConfig::map_rect.w + AppConfig::status_rect.w - text_surface->w)/2;
        else 
            window_dest.x = start->x;

        if(start->y < 0) 
            window_dest.y = (AppConfig::map_rect.h - text_surface->h)/2;
        else 
            window_dest.y = start->y;
    }
    window_dest.w = text_surface->w;
    window_dest.h = text_surface->h;

    // Renderiza o texto na tela
    SDL_RenderCopy(m_renderer, m_text_texture, NULL, &window_dest);
}

// Desenha um retângulo na tela, preenchido ou apenas contornado
void Renderer::drawRect(const SDL_Rect *rect, SDL_Color rect_color, bool fill)
{
    SDL_SetRenderDrawColor(m_renderer, rect_color.r, rect_color.g, rect_color.b, rect_color.a);

    if(fill)
        SDL_RenderFillRect(m_renderer, rect); // Desenha retângulo preenchido
    else
        SDL_RenderDrawRects(m_renderer, rect, 1); // Desenha apenas a borda do retângulo
}
