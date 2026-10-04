#include "message_box.h"
#include "../engine/renderer.h"
#include "../appconfig.h"

#include <algorithm>

void drawMessageBox(Renderer* r, const std::vector<MessageLine>& lines, SDL_Color border)
{
    const SDL_Color BLACK = {0, 0, 0, 255};
    const int PAD_X = 18, PAD_Y = 14;
    int width = 0, height = 0;
    for(const MessageLine& line : lines)
    {
        SDL_Point size = r->textSize(line.text, line.size);
        width = std::max(width, size.x);
        height += size.y + line.gap;
    }
    if(!lines.empty()) height -= lines.back().gap;

    SDL_Rect box = {AppConfig::map_rect.x + (AppConfig::map_rect.w - width - 2 * PAD_X) / 2,
                    AppConfig::map_rect.y + (AppConfig::map_rect.h - height - 2 * PAD_Y) / 2,
                    width + 2 * PAD_X, height + 2 * PAD_Y};
    r->drawRect(&box, BLACK, true);
    r->drawRect(&box, border, false);
    SDL_Rect inner = {box.x + 1, box.y + 1, box.w - 2, box.h - 2};
    r->drawRect(&inner, border, false);

    int center_x = box.x + box.w / 2;
    int y = box.y + PAD_Y;
    for(const MessageLine& line : lines)
    {
        if(line.visible) r->drawTextCentered(center_x, y, line.text, line.color, line.size);
        y += r->textSize(line.text, line.size).y + line.gap;
    }
}
