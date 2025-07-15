#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>

/**
 * @brief
 * Classe responsável por desenhar objetos na tela.
 * Gerencia o renderizador SDL, texturas e fontes para exibição de gráficos e textos.
 */
class Renderer
{
public:
    Renderer();  ///< Construtor: inicializa ponteiros dos recursos gráficos.
    ~Renderer(); ///< Destrutor: libera todos os recursos gráficos alocados.

    /**
     * Carrega a textura principal do jogo a partir de um arquivo e cria o renderizador associado à janela.
     * @param window Ponteiro para o objeto da janela SDL.
     */
    void loadTexture(SDL_Window* window);

    /**
     * Carrega as fontes utilizadas para renderização de texto em três tamanhos diferentes.
     */
    void loadFont();

    /**
     * Limpa o buffer de renderização, preenchendo-o com a cor de fundo padrão.
     */
    void clear();

    /**
     * Apresenta o conteúdo do buffer de renderização na tela (swap dos buffers).
     */
    void flush();

    /**
     * Desenha um fragmento da textura principal em uma região específica do buffer de renderização.
     * @param texture_src Retângulo fonte na textura.
     * @param window_dest Retângulo de destino no buffer de renderização.
     */
    void drawObject(const SDL_Rect *texture_src, const SDL_Rect *window_dest);

    /**
     * Define o fator de escala do renderizador e centraliza o conteúdo no meio da janela.
     * Mantém as proporções do mapa.
     * @param xs Fator de escala horizontal (largura da janela / largura do mapa).
     * @param ys Fator de escala vertical (altura da janela / altura do mapa).
     * @see AppConfig::map_rect
     */
    void setScale(float xs, float ys);

    /**
     * Desenha um texto na tela em uma posição específica.
     * @param start Posição inicial do texto; valores negativos centralizam o texto naquele eixo.
     * @param text Texto a ser desenhado.
     * @param text_color Cor do texto.
     * @param font_size Tamanho da fonte: 1 (grande), 2 (média), 3 (pequena).
     */
    void drawText(const SDL_Point* start, std::string text, SDL_Color text_color, int font_size = 1);

    /**
     * Desenha um retângulo na tela.
     * @param rect Retângulo a ser desenhado (posição e tamanho).
     * @param rect_color Cor do retângulo.
     * @param fill Se true, o retângulo será preenchido; caso contrário, apenas a borda será desenhada.
     */
    void drawRect(const SDL_Rect* rect, SDL_Color rect_color, bool fill = false);

private:
    /**
     * Ponteiro para o renderizador SDL associado ao buffer da janela.
     */
    SDL_Renderer* m_renderer;

    /**
     * Ponteiro para a textura principal contendo todos os elementos visuais do jogo.
     */
    SDL_Texture* m_texture;

    /**
     * Ponteiro para a textura auxiliar utilizada na renderização de textos.
     */
    SDL_Texture* m_text_texture;

    /**
     * Fonte de tamanho 28 (grande).
     */
    TTF_Font* m_font1;

    /**
     * Fonte de tamanho 14 (média).
     */
    TTF_Font* m_font2;

    /**
     * Fonte de tamanho 10 (pequena).
     */
    TTF_Font* m_font3;
};

#endif // RENDERER_H
