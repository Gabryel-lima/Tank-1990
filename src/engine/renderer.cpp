#include "renderer.h"
#include "../appconfig.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <iostream>

// Construtor: inicializa todos os ponteiros dos recursos gráficos como nulos
Renderer::Renderer()
{
    m_texture = nullptr;
    m_renderer = nullptr;
    m_text_texture = nullptr;
    m_font1 = nullptr;
    m_font2 = nullptr;
    m_font3 = nullptr;
}

// Destrutor: libera todos os recursos gráficos alocados
Renderer::~Renderer()
{
    // As texturas precisam ser liberadas ANTES do renderizador:
    // SDL_DestroyRenderer já libera as texturas dele, e destruí-las depois
    // acessa memória liberada (segfault ao fechar o jogo).
    if(m_texture != nullptr)
        SDL_DestroyTexture(m_texture); // Libera a textura principal
    if(m_text_texture != nullptr)
        SDL_DestroyTexture(m_text_texture); // Libera a textura de texto
    if(m_renderer != nullptr)
        SDL_DestroyRenderer(m_renderer); // Libera o renderizador SDL
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
    float scale = std::min(xs, ys);
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
SDL_Point Renderer::textSize(const string& text, int font_size) const
{
    TTF_Font* font = (font_size == 2 ? m_font2 : (font_size == 3 ? m_font3 : m_font1));
    int w = 0, h = 0;
    if(font == nullptr || TTF_SizeText(font, text.c_str(), &w, &h) != 0) return {0, 0};
    return {w, h};
}

void Renderer::drawTextCentered(int center_x, int y, const string& text, SDL_Color text_color, int font_size)
{
    SDL_Point start = {center_x - textSize(text, font_size).x / 2, y};
    drawText(&start, text, text_color, font_size);
}

void Renderer::drawTextOutlined(SDL_Point start, const string& text, SDL_Color text_color, int font_size)
{
    for(int dy = -1; dy <= 1; dy++)
        for(int dx = -1; dx <= 1; dx++)
        {
            if(dx == 0 && dy == 0) continue;
            SDL_Point shadow = {start.x + dx, start.y + dy};
            drawText(&shadow, text, {0, 0, 0, 255}, font_size);
        }
    drawText(&start, text, text_color, font_size);
}

void Renderer::drawText(const SDL_Point* start, string text, SDL_Color text_color, int font_size)
{
    // Verifica se as fontes estão carregadas
    if(m_font1 == nullptr || m_font2 == nullptr || m_font3 == nullptr) return;
    // Libera a textura de texto anterior, se existir. Zera o ponteiro para
    // que um retorno antecipado abaixo não deixe o destrutor liberá-la de novo.
    if(m_text_texture != nullptr)
        SDL_DestroyTexture(m_text_texture);
    m_text_texture = nullptr;

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
    if(m_text_texture == nullptr)
    {
        SDL_FreeSurface(text_surface);
        return; // Falha ao criar textura
    }

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

    // Libera a superfície temporária (era vazada a cada frame)
    SDL_FreeSurface(text_surface);
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
