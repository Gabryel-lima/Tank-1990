// As partidas de demonstração do menu (Demo::modes()) numa janela, sem o menu por cima, sem
// escurecer e com som: um modo atrás do outro, cada um por alguns segundos. Serve para gravar
// o vídeo que o EmuELEC mostra com o jogo selecionado (tools/emuelec/art/record_video.sh,
// ver EMUELEC.md). Rode a partir de build/bin (onde estão os mapas e a textura):
//
//   make attract
//   cd build/bin && ./attract --seed 1990 --switch 10000
//
// Opções:
//   --seed S      semente do sorteio (mapas, fases, jogadores; padrão 1990)
//   --switch MS   tempo de cada modo, em ms (padrão 10000)
//   --scale N     tamanho da janela: N vezes a tela lógica de 464x416 (padrão 2)
//
// Fecha sozinho depois de passar por todos os modos.

#include "../src/app_state/demo.h"
#include "../src/app_state/duel_layout.h"
#include "../src/app_state/survival_layout.h"
#include "../src/appconfig.h"
#include "../src/engine/engine.h"
#include "../src/soundmanager.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char* argv[])
{
    unsigned seed = 1990;
    Uint32 switch_ms = 10000;
    int scale = 2;
    for(int i = 1; i + 1 < argc; i += 2)
    {
        if(std::strcmp(argv[i], "--seed") == 0) seed = std::strtoul(argv[i + 1], nullptr, 10);
        else if(std::strcmp(argv[i], "--switch") == 0) switch_ms = std::strtoul(argv[i + 1], nullptr, 10);
        else if(std::strcmp(argv[i], "--scale") == 0) scale = std::atoi(argv[i + 1]);
        else { std::fprintf(stderr, "Opção desconhecida: %s\n", argv[i]); return 2; }
    }
    if(scale < 1) scale = 1;
    srand(seed);

    // A mesma inicialização do App::run, sem controles (a demonstração é toda da IA)
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        std::fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }
    if(SoundManager::getInstance().init()) SoundManager::getInstance().loadSounds();
    DuelLayout::loadMapList();
    SurvivalLayout::loadMapList();

    int w = AppConfig::windows_rect.w * scale, h = AppConfig::windows_rect.h * scale;
    SDL_Window* window = SDL_CreateWindow("TANKS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN);
    if(window == nullptr || !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) || TTF_Init() == -1)
    {
        std::fprintf(stderr, "Falha ao iniciar: %s\n", SDL_GetError());
        return 1;
    }
    Engine& engine = Engine::getEngine();
    engine.initModules();
    engine.getRenderer()->loadTexture(window);
    engine.getRenderer()->loadFont();
    AppConfig::windows_rect.w = w;
    AppConfig::windows_rect.h = h;
    engine.getRenderer()->setScale(static_cast<float>(scale), static_cast<float>(scale));

    const Uint32 STEP = 16;
    for(const Demo::Mode& mode : Demo::modes())
    {
        AppState* state = mode.create();
        // Pula a abertura, mudo, como o Demo::random do menu
        SoundManager::getInstance().setMuted(true);
        for(Uint32 t = 0; t < 3200; t += STEP) state->update(STEP);
        SoundManager::getInstance().setMuted(false);

        Uint32 start = SDL_GetTicks(), last = start;
        while(SDL_GetTicks() - start < switch_ms && !state->finished())
        {
            SDL_Event ev;
            while(SDL_PollEvent(&ev))
                if(ev.type == SDL_QUIT) { delete state; SDL_Quit(); return 0; }
            Uint32 now = SDL_GetTicks();
            state->update(now - last);
            last = now;
            state->draw();
            SDL_Delay(STEP);
        }
        delete state;
    }

    engine.destroyModules();
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
