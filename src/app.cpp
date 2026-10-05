#include "app.h"
#include "appconfig.h"
#include "engine/engine.h"
#include "app_state/game.h"
#include "app_state/menu.h"
#include "soundmanager.h"
#include "controllers.h"
#include "input/netpad.h"
#include "app_state/duel_layout.h"
#include "app_state/survival_layout.h"

#include <ctime>
#include <iostream>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define VERSION "1.2.1"

// Construtor da classe App. Inicializa ponteiro da janela como nulo.
App::App()
{
    m_window = nullptr;
    m_app_state = nullptr; // evita delete de ponteiro nao inicializado no destrutor
    is_running = false;
}

// Destrutor da classe App. Libera o estado atual da aplicação, se existir.
App::~App()
{
    if(m_app_state != nullptr)
        delete m_app_state;
}

// Função principal que executa o loop do aplicativo.
void App::run()
{
    is_running = true;
    // Inicialização do SDL e criação da janela

    // Os controles iniciam à parte: juntos no mesmo SDL_Init, uma falha deles (no WSL, sem
    // udev, com o controle repassado pela metade) derrubava o SDL inteiro, e o jogo fechava
    // sem abrir a janela e sem dizer nada
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        std::cerr << "Falha ao iniciar o SDL: " << SDL_GetError() << "\n";
    else
    {
        // Inicializa e carrega sons. Sem áudio o jogo continua, só que mudo.
        if (SoundManager::getInstance().init())
            SoundManager::getInstance().loadSounds();

        if(SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0)
            std::cerr << "Controles indisponíveis (" << SDL_GetError() << "): o jogo segue só com o teclado\n";
        else
        {
            // Abre os controles já conectados (os demais chegam por hotplug)
            Controllers::init();
            // Controles que chegam pela rede (ponte do Windows para o WSL): só se o ambiente
            // pedir (TANK_NETPAD=porta). Eles viram controles virtuais do SDL (CONTROLES.md)
            NetPad::startFromEnvironment();
        }

        // Lista de mapas do duelo (duel_levels/maps.txt), sem os que não passam na validação
        DuelLayout::loadMapList();
        // Idem para os mapas do modo sobrevivência (survival_levels/maps.txt)
        SurvivalLayout::loadMapList();

        // Cria a janela principal do jogo
        m_window = SDL_CreateWindow("TANKS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    AppConfig::windows_rect.w, AppConfig::windows_rect.h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

        // Confere a janela e inicializa imagens PNG e fontes TrueType;
        // se algo falhar, libera o que já foi criado e sai.
        if(m_window == nullptr ||
           !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) ||
           TTF_Init() == -1)
        {
            std::cerr << "Falha ao iniciar: " << SDL_GetError() << "\n";
            cleanup();
            return;
        }

        // Inicializa o gerador de números aleatórios
        srand(time(NULL));

        // Inicializa os módulos do motor gráfico
        Engine& engine = Engine::getEngine();
        engine.initModules();
        engine.getRenderer()->loadTexture(m_window);
        engine.getRenderer()->loadFont();

        // Estado inicial do aplicativo é o menu principal
        m_app_state = new Menu;

        double FPS;
        Uint32 time1, time2, dt, fps_time = 0, fps_count = 0, delay = 15;
        time1 = SDL_GetTicks();

        // Loop principal do aplicativo
        while(is_running)
        {
            time2 = SDL_GetTicks();
            dt = time2 - time1;
            time1 = time2;

            // Verifica se o estado atual terminou e troca para o próximo estado
            if(m_app_state->finished())
            {
                AppState* new_state = m_app_state->nextState();
                delete m_app_state;
                m_app_state = new_state;
            }
            if(m_app_state == nullptr) break;

            // Processa eventos de entrada (teclado, mouse, janela)
            eventProces();

            // Atualiza a lógica do estado atual
            m_app_state->update(dt);
            // Desenha o estado atual na tela
            m_app_state->draw();

            // Aguarda um pequeno tempo para controlar o FPS
            SDL_Delay(delay);

            // Cálculo e ajuste dinâmico do FPS
            fps_time += dt; 
            fps_count++;
            if(fps_time > 200)
            {
                FPS = (double)fps_count / fps_time * 1000;
                if(FPS > 60) delay++;
                else if(delay > 0) delay--;
                fps_time = 0; 
                fps_count = 0;
            }
        }
    }

    cleanup();
}

// Libera tudo na ordem certa. O estado atual (menu/jogo) é destruído antes
// do renderizador e do SDL_Quit, pois seus objetos ainda usam o SDL.
void App::cleanup()
{
    delete m_app_state;
    m_app_state = nullptr;

    Engine::getEngine().destroyModules();

    if(m_window != nullptr)
        SDL_DestroyWindow(m_window);
    m_window = nullptr;

    SoundManager::getInstance().cleanup();
    Controllers::shutdown();
    NetPad::stop();

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

// Processa todos os eventos SDL (teclado, mouse, janela, etc)
void App::eventProces()
{
    // Estado que a ponte de controles mandou: atualiza os controles virtuais antes de ler os
    // eventos, que é quando o SDL os transforma em eventos de controle
    NetPad::poll();

    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        if(event.type == SDL_QUIT)
        {
            // Evento de fechamento da janela: encerra o loop principal
            is_running = false;
        }
        else if(event.type == SDL_WINDOWEVENT)
        {
            // Eventos de redimensionamento ou exibição da janela
            if(event.window.event == SDL_WINDOWEVENT_RESIZED ||
               event.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
               event.window.event == SDL_WINDOWEVENT_RESTORED ||
               event.window.event == SDL_WINDOWEVENT_SHOWN)
            {
                // Atualiza as dimensões da janela no AppConfig. Só o RESIZED
                // traz o tamanho em data1/data2; nos outros eventos eles vêm
                // zerados, então lê o tamanho direto da janela.
                SDL_GetWindowSize(m_window, &AppConfig::windows_rect.w, &AppConfig::windows_rect.h);
                // Ajusta a escala do renderizador conforme o novo tamanho da janela
                Engine::getEngine().getRenderer()->setScale(
                    (float)AppConfig::windows_rect.w / (AppConfig::map_rect.w + AppConfig::status_rect.w),
                    (float)AppConfig::windows_rect.h / AppConfig::map_rect.h
                );
            }
        }

        // Conexão/desconexão de controles
        Controllers::handleEvent(&event);

        // Encaminha o evento para o estado atual do aplicativo
        m_app_state->eventProcess(&event);
    }
}
