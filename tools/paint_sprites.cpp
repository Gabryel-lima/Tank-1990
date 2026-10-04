// Desenha a pixel art dos poderes dos modos extras em resources/png/texture.png.
//
//   make sprites      (compila e roda)
//
// A arte fica em tools/sprites/powers.txt, em texto, para ser editada sem programa de
// desenho. Cada "pixel" do arquivo vira 2x2 na textura, como os sprites do NES do jogo.
//
// Blocos do arquivo:
//   icon NOME X Y        ícone de bônus 32x32: o símbolo (12 colunas x 11 linhas) vai dentro
//                        da moldura padrão dos bônus originais (branco, cinza, azul-marinho)
//   sprite NOME X Y W H [rot]   sprite livre de W x H "pixels"; com rot, desenha também as
//                        rotações de 90° à direita (direções cima, direita, baixo, esquerda)
// Cores: . transparente  W branco  G cinza  N azul-marinho  K preto  R vermelho  Y amarelo
// Linhas começando com # são comentários.

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

static const std::map<char, SDL_Color> PALETTE = {
    {'W', {255, 255, 255, 255}}, {'G', {188, 188, 188, 255}}, {'N', {27, 63, 95, 255}},
    {'K', {0, 0, 0, 255}}, {'R', {230, 40, 40, 255}}, {'Y', {255, 200, 40, 255}},
};

// Moldura dos bônus originais (16 x 16 "pixels"): a linha 0 é transparente e o desenho
// começa na linha 1 da textura, como nos ícones do jogo; 'S' é onde entra o símbolo
static const char* FRAME[16] = {
    "................",
    ".WWWWWWWWWWWWWG.",
    "W.............WN",
    "W.SSSSSSSSSSSSWN",
    "W.SSSSSSSSSSSSWN",
    "W.SSSSSSSSSSSSWN",
    "W.SSSSSSSSSSSSWN",
    "W.SSSSSSSSSSSSWN",
    "W.SSSSSSSSSSSSWN",
    "W.SSSSSSSSSSSSWN",
    "W.SSSSSSSSSSSSWN",
    "W.SSSSSSSSSSSSWN",
    "W.SSSSSSSSSSSSWN",
    "W.SSSSSSSSSSSSWN",
    "GWWWWWWWWWWWWWGN",
    ".NNNNNNNNNNNNNN.",
};

static void put(SDL_Surface* s, int x, int y, char c)
{
    Uint32* p = static_cast<Uint32*>(s->pixels);
    Uint32 value = 0; // transparente
    auto it = PALETTE.find(c);
    if(it != PALETTE.end()) value = SDL_MapRGBA(s->format, it->second.r, it->second.g, it->second.b, it->second.a);
    // Cada "pixel" da arte ocupa 2x2 na textura
    for(int j = 0; j < 2; j++)
        for(int i = 0; i < 2; i++)
            p[(y + j) * (s->pitch / 4) + x + i] = value;
}

// Desenha a grade com o canto em (x, y); a primeira linha da grade fica na linha y - 1
// quando shift é true (moldura dos bônus: 1 pixel de folga em cima, como os originais)
static void paint(SDL_Surface* s, int x, int y, const std::vector<std::string>& grid, bool shift)
{
    for(size_t r = 0; r < grid.size(); r++)
        for(size_t c = 0; c < grid[r].size(); c++)
        {
            int py = y + 2 * static_cast<int>(r) - (shift ? 1 : 0);
            if(py < y) { // a linha 0 da moldura: só a metade de baixo cabe, e ela é transparente
                continue;
            }
            put(s, x + 2 * static_cast<int>(c), py, grid[r][c]);
        }
}

static std::vector<std::string> rotateRight(const std::vector<std::string>& g)
{
    int h = g.size(), w = g[0].size();
    std::vector<std::string> out(w, std::string(h, '.'));
    for(int r = 0; r < h; r++)
        for(int c = 0; c < w; c++) out[c][h - 1 - r] = g[r][c];
    return out;
}

int main(int argc, char** argv)
{
    std::string art = argc > 1 ? argv[1] : "tools/sprites/powers.txt";
    std::string texture = argc > 2 ? argv[2] : "resources/png/texture.png";
    SDL_Init(0);
    IMG_Init(IMG_INIT_PNG);
    SDL_Surface* loaded = IMG_Load(texture.c_str());
    if(loaded == nullptr) { std::printf("não abriu %s\n", texture.c_str()); return 1; }
    SDL_Surface* s = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(loaded);

    std::ifstream in(art);
    if(!in.is_open()) { std::printf("não abriu %s\n", art.c_str()); return 1; }
    std::string line;
    int drawn = 0;
    while(std::getline(in, line))
    {
        if(line.empty() || line[0] == '#') continue;
        std::istringstream head(line);
        std::string kind, name;
        int x = 0, y = 0;
        head >> kind >> name >> x >> y;
        if(kind == "icon")
        {
            std::vector<std::string> symbol;
            while(symbol.size() < 11 && std::getline(in, line))
                if(!line.empty() && line[0] != '#') symbol.push_back(line);
            std::vector<std::string> grid(FRAME, FRAME + 16);
            for(int r = 0; r < 11; r++)
                for(int c = 0; c < 12; c++)
                    grid[3 + r][2 + c] = (c < static_cast<int>(symbol[r].size()) ? symbol[r][c] : 'N');
            // Fundo do símbolo: azul-marinho
            for(auto& row : grid)
                for(auto& ch : row)
                    if(ch == 'S') ch = 'N';
            // A linha 0 (transparente) não é desenhada: o ícone começa na linha y da textura
            // com a linha 1 da grade (moldura de cima), igual aos bônus originais
            std::vector<std::string> body(grid.begin() + 1, grid.end());
            SDL_Rect clear = {x, y, 32, 32};
            SDL_FillRect(s, &clear, 0);
            for(size_t r = 0; r < body.size(); r++)
                for(size_t c = 0; c < body[r].size(); c++)
                    put(s, x + 2 * c, y + 1 + 2 * r, body[r][c]);
            drawn++;
            std::printf("ícone  %-12s em (%d, %d)\n", name.c_str(), x, y);
        }
        else if(kind == "sprite")
        {
            int w = 0, h = 0;
            std::string rot;
            head >> w >> h >> rot;
            std::vector<std::string> grid;
            while(static_cast<int>(grid.size()) < h && std::getline(in, line))
                if(!line.empty() && line[0] != '#') grid.push_back(line);
            int turns = (rot == "rot") ? 4 : 1;
            for(int k = 0; k < turns; k++)
            {
                SDL_Rect clear = {x + k * 2 * w, y, 2 * w, 2 * h};
                SDL_FillRect(s, &clear, 0);
                paint(s, x + k * 2 * w, y, grid, false);
                grid = rotateRight(grid);
            }
            drawn++;
            std::printf("sprite %-12s em (%d, %d), %dx%d%s\n", name.c_str(), x, y, 2 * w, 2 * h, turns > 1 ? ", 4 direções" : "");
        }
    }
    if(IMG_SavePNG(s, texture.c_str()) != 0) { std::printf("não salvou %s\n", texture.c_str()); return 1; }
    std::printf("%d desenho(s) gravados em %s\n", drawn, texture.c_str());
    return 0;
}
