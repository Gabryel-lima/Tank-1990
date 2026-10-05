#ifndef MESSAGE_BOX_H
#define MESSAGE_BOX_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>

class Renderer;

/**
 * @brief Linha de texto de uma caixa de mensagem (modos extras).
 */
struct MessageLine
{
    std::string text;
    SDL_Color color;
    int size;
    int gap;              ///< espaço até a próxima linha (px)
    bool visible = true;  ///< linha invisível ainda ocupa espaço (a caixa não muda de tamanho)
};

/**
 * Caixa preta com borda dupla, do tamanho do texto, centralizada no mapa. Assim a mensagem
 * fica legível em qualquer mapa (texto branco direto sobre gelo ou pedra dava ~1,7:1).
 * Com @a top >= 0, a caixa fica nessa altura em vez de no meio (para não tapar a ação).
 */
void drawMessageBox(Renderer* r, const std::vector<MessageLine>& lines, SDL_Color border, int top = -1);

#endif // MESSAGE_BOX_H
