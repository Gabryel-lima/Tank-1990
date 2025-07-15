#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>

/**
 * @brief Classe responsável por desenhar objetos na tela.
 */
class Renderer {
public:
    Renderer();
    ~Renderer();
    /**
     * Carrega a textura dos objetos e cria o renderer associado à janela.
     * @param window - ponteiro para a janela da aplicação
     */
    void loadTexture(SDL_Window* window);
    /**
     * Carrega as fontes em três tamanhos diferentes.
     */
    void loadFont();
    /**
     * Limpa o buffer de tela.
     */
    void clear();
    /**
     * Apresenta o buffer de tela.
     */
    void flush();
    /**
     * Desenha um fragmento da textura em uma área da tela.
     * @param texture_src - retângulo fonte na textura
     * @param window_dest - retângulo destino na tela
     */
    void drawObject(const SDL_Rect *texture_src, const SDL_Rect *window_dest);
    /**
     * Define a escala do buffer de renderização para manter proporções e centralizar.
     * @param xs - escala horizontal
     * @param ys - escala vertical
     */
    void setScale(float xs, float ys);
    /**
     * Desenha texto na tela em uma posição específica.
     * @param start - ponto inicial do texto; valor negativo centraliza
     * @param text - texto a ser desenhado
     * @param text_color - cor do texto
     * @param font_size - tamanho da fonte (1, 2 ou 3)
     */
    void drawText(const SDL_Point* start, std::string text, SDL_Color text_color, int font_size = 1);
    /**
     * Desenha um retângulo na tela.
     * @param rect - posição e tamanho
     * @param rect_color - cor
     * @param fill - se true, preenche o retângulo
     */
    void drawRect(const SDL_Rect* rect, SDL_Color rect_color, bool fill = false);
private:
    SDL_Renderer* m_renderer;
    SDL_Texture* m_texture;
    SDL_Texture* m_text_texture;
    TTF_Font* m_font1;
    TTF_Font* m_font2;
    TTF_Font* m_font3;
};

#endif // RENDERER_H 