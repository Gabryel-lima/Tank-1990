#include "controllers.h"
#include "appconfig.h"

#include <algorithm>
#include <iostream>

std::vector<SDL_GameController*> Controllers::m_controllers;
int Controllers::m_player_count = 1;

// Abre o controle do índice de dispositivo informado, se ainda não estiver aberto
static SDL_GameController* openDevice(int device_index)
{
    if(!SDL_IsGameController(device_index)) return nullptr;
    SDL_GameController* c = SDL_GameControllerOpen(device_index);
    if(c == nullptr)
        std::cerr << "Controle " << device_index << ": " << SDL_GetError() << "\n";
    return c;
}

void Controllers::init()
{
    for(int i = 0; i < SDL_NumJoysticks(); i++)
    {
        // O SDL também envia DEVICEADDED para os controles já conectados;
        // handleEvent ignora os que já estão na lista.
        SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(i);
        if(SDL_GameControllerFromInstanceID(id) != nullptr) continue;
        SDL_GameController* c = openDevice(i);
        if(c != nullptr) m_controllers.push_back(c);
    }
}

void Controllers::shutdown()
{
    for(auto c : m_controllers) SDL_GameControllerClose(c);
    m_controllers.clear();
}

void Controllers::handleEvent(const SDL_Event* ev)
{
    if(ev->type == SDL_CONTROLLERDEVICEADDED)
    {
        // Em DEVICEADDED, "which" é o índice do dispositivo
        SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(ev->cdevice.which);
        SDL_GameController* existing = SDL_GameControllerFromInstanceID(id);
        if(existing != nullptr &&
           std::find(m_controllers.begin(), m_controllers.end(), existing) != m_controllers.end())
            return;
        SDL_GameController* c = openDevice(ev->cdevice.which);
        if(c != nullptr) m_controllers.push_back(c);
    }
    else if(ev->type == SDL_CONTROLLERDEVICEREMOVED)
    {
        // Em DEVICEREMOVED, "which" é o instance id do joystick
        SDL_GameController* c = SDL_GameControllerFromInstanceID(ev->cdevice.which);
        auto it = std::find(m_controllers.begin(), m_controllers.end(), c);
        if(it != m_controllers.end())
        {
            SDL_GameControllerClose(c);
            m_controllers.erase(it);
        }
    }
}

void Controllers::setPlayerCount(int count)
{
    m_player_count = count;
}

SDL_GameController* Controllers::forPlayer(int player_index)
{
    if(player_index < 0 || player_index >= m_player_count) return nullptr;

    // Ordem de distribuição: jogadores sem teclado primeiro, depois os de teclado
    std::vector<int> order;
    for(int pass = 0; pass < 2; pass++)
    {
        for(int i = 0; i < m_player_count; i++)
        {
            bool has_keyboard = i < static_cast<int>(AppConfig::player_keys.size()) &&
                                AppConfig::player_keys.at(i).hasKeyboard();
            if(has_keyboard == (pass == 1)) order.push_back(i);
        }
    }

    for(size_t slot = 0; slot < order.size() && slot < m_controllers.size(); slot++)
        if(order[slot] == player_index) return m_controllers[slot];
    return nullptr;
}

int Controllers::count()
{
    return static_cast<int>(m_controllers.size());
}
