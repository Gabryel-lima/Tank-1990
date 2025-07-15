#include "app.h" // Ok
#include "appconfig.h"
#include "engine/engine.h"
#include "app_state/game.h"
#include "app_state/menu.h"

#include <ctime>
#include <iostream>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define VERSION "1.2.0"

App::App() {
    // Garante que m_window não aponte para lixo de memória ao criar o objeto App
    m_window = nullptr;
}

App::~App() {
    if (m_app_state != nullptr)
        delete m_app_state;
}

void App::run() {
    is_running = true;

    // Inicialização do SDL
    // ! é melhor do que == 0
    if (!SDL_Init(SDL_INIT_VIDEO)) {

        // Janela de memória
        m_window = SDL_CreateWindow("My Tank 1990", 
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            AppConfig::windows_rect.w, AppConfig::windows_rect.h, 
            (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE));
        
        // Se m_window for nullptr, significa que a janela não foi criada corretamente.
        if (m_window == nullptr) return;

        // Caregamento dos assets imgs
    }

}
