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

// Coloca o controle na primeira vaga vazia (ou numa nova vaga no fim)
static void addToSlot(std::vector<SDL_GameController*>& slots, SDL_GameController* c)
{
    auto hole = std::find(slots.begin(), slots.end(), nullptr);
    if(hole != slots.end()) *hole = c;
    else slots.push_back(c);
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
        if(c != nullptr) addToSlot(m_controllers, c);
    }
}

void Controllers::shutdown()
{
    for(auto c : m_controllers)
        if(c != nullptr) SDL_GameControllerClose(c);
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
        if(c != nullptr) addToSlot(m_controllers, c);
    }
    else if(ev->type == SDL_CONTROLLERDEVICEREMOVED)
    {
        // Em DEVICEREMOVED, "which" é o instance id do joystick. A vaga fica vazia
        // (em vez de ser removida) para os outros controles não trocarem de jogador.
        SDL_GameController* c = SDL_GameControllerFromInstanceID(ev->cdevice.which);
        auto it = std::find(m_controllers.begin(), m_controllers.end(), c);
        if(c != nullptr && it != m_controllers.end())
        {
            SDL_GameControllerClose(c);
            *it = nullptr;
        }
        while(!m_controllers.empty() && m_controllers.back() == nullptr) m_controllers.pop_back();
    }
}

void Controllers::setPlayerCount(int count)
{
    m_player_count = count;
    // Início de partida: os controles conectados ocupam as primeiras vagas
    m_controllers.erase(std::remove(m_controllers.begin(), m_controllers.end(), nullptr), m_controllers.end());
}

std::vector<Controllers::Assignment> Controllers::assign(int player_count)
{
    std::vector<Assignment> result(std::max(0, player_count));
    int layouts = static_cast<int>(AppConfig::keyboard_layouts.size());
    std::vector<bool> layout_used(layouts, false);

    // 1. Controle primeiro: a vaga i é do jogador i
    for(int i = 0; i < player_count; i++)
        if(i < static_cast<int>(m_controllers.size()) && m_controllers[i] != nullptr)
            result[i].pad_slot = i;

    // 2. Teclado como reserva, na ordem dos jogadores sem controle
    int next_layout = 0;
    for(int i = 0; i < player_count && next_layout < layouts; i++)
        if(result[i].pad_slot < 0)
        {
            result[i].keyboard = next_layout;
            layout_used[next_layout++] = true;
        }

    // 3. Layouts que sobraram ficam com o dono original (layout 0 -> J1, layout 1 -> J2)
    for(int l = 0; l < layouts; l++)
        if(!layout_used[l] && l < player_count && result[l].keyboard < 0)
            result[l].keyboard = l;

    return result;
}

SDL_GameController* Controllers::forPlayer(int player_index)
{
    if(player_index < 0 || player_index >= m_player_count) return nullptr;
    int slot = assign(m_player_count)[player_index].pad_slot;
    return slot >= 0 ? m_controllers[slot] : nullptr;
}

const Player::PlayerKeys* Controllers::keyboardFor(int player_index)
{
    if(player_index < 0 || player_index >= m_player_count) return nullptr;
    int layout = assign(m_player_count)[player_index].keyboard;
    return layout >= 0 ? &AppConfig::keyboard_layouts[layout] : nullptr;
}

std::string Controllers::inputName(int player_count, int player_index)
{
    std::vector<Assignment> a = assign(player_count);
    if(player_index < 0 || player_index >= static_cast<int>(a.size())) return "NO PAD";
    if(a[player_index].pad_slot >= 0) return "PAD " + std::to_string(a[player_index].pad_slot + 1);
    if(a[player_index].keyboard >= 0) return AppConfig::keyboard_layouts[a[player_index].keyboard].name;
    return "NO PAD";
}

int Controllers::playersWithoutInput(int player_count)
{
    int n = 0;
    for(const Assignment& a : assign(player_count))
        if(a.pad_slot < 0 && a.keyboard < 0) n++;
    return n;
}

int Controllers::count()
{
    return static_cast<int>(m_controllers.size() - std::count(m_controllers.begin(), m_controllers.end(), nullptr));
}
