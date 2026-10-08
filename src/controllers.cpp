#include "controllers.h"
#include "appconfig.h"
#include "input/pad_info.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <utility>

std::vector<SDL_GameController*> Controllers::m_controllers;
int Controllers::m_player_count = 1;
std::map<SDL_JoystickID, int> Controllers::m_first_input;
int Controllers::m_inputs = 0;

// TANK_PAD_LOG=1 liga o registro dos botões no log (ver logPadEvent)
static bool padLogEnabled()
{
    static const bool enabled = [] {
        const char* v = std::getenv("TANK_PAD_LOG");
        return v != nullptr && v[0] != '\0' && v[0] != '0';
    }();
    return enabled;
}

// Abre o controle do índice de dispositivo informado, se ainda não estiver aberto
static SDL_GameController* openDevice(int device_index)
{
    if(!SDL_IsGameController(device_index)) return nullptr;
    SDL_GameController* c = SDL_GameControllerOpen(device_index);
    if(c == nullptr)
        std::cerr << "Controle " << device_index << ": " << SDL_GetError() << "\n";
    else
        // De onde veio (USB, Bluetooth, virtual da ponte): só informação, o jogo não decide
        // nada por isso (CONTROLES.md)
    {
        std::cout << "Controle conectado: " << PadInfo::summary(PadInfo::describe(device_index)) << std::endl;
        if(padLogEnabled())
        {
            // O mapeamento em uso (nome e botões): é o que decide qual botão físico vira A, B, X, Y...
            char* mapping = SDL_GameControllerMapping(c);
            std::cout << "  mapeamento: " << (mapping != nullptr ? mapping : "?") << std::endl;
            SDL_free(mapping);
        }
    }
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

// TANK_PAD_LOG=1 (o lançador do console de TV liga): cada botão apertado vai para o log, cru (o
// número que o controle manda) e como o jogo o vê (A, B, X, Y...); e o D-pad e o analógico
// quando mudam de direção. Serve para descobrir por que um controle genérico "troca" botões
static void logPadEvent(const SDL_Event* ev)
{
    static std::map<std::pair<int, int>, int> axis_side; // (controle, eixo) -> -1, 0, +1
    switch(ev->type)
    {
    case SDL_JOYBUTTONDOWN:
        std::cout << "  controle " << ev->jbutton.which << ": botao cru " << int(ev->jbutton.button) << std::endl;
        break;
    case SDL_JOYHATMOTION:
        std::cout << "  controle " << ev->jhat.which << ": direcional cru " << int(ev->jhat.value) << std::endl;
        break;
    case SDL_CONTROLLERBUTTONDOWN:
        std::cout << "  controle " << ev->cbutton.which << ": o jogo ve "
                  << SDL_GameControllerGetStringForButton(SDL_GameControllerButton(ev->cbutton.button)) << std::endl;
        break;
    case SDL_CONTROLLERAXISMOTION:
    {
        int side = ev->caxis.value > 16000 ? 1 : (ev->caxis.value < -16000 ? -1 : 0);
        int& last = axis_side[{ev->caxis.which, ev->caxis.axis}];
        if(side != last)
        {
            last = side;
            std::cout << "  controle " << ev->caxis.which << ": eixo "
                      << SDL_GameControllerGetStringForAxis(SDL_GameControllerAxis(ev->caxis.axis))
                      << (side > 0 ? " +" : (side < 0 ? " -" : " solto")) << std::endl;
        }
        break;
    }
    default:
        break;
    }
}

void Controllers::handleEvent(const SDL_Event* ev)
{
    if(padLogEnabled()) logPadEvent(ev);
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
    else if(ev->type == SDL_CONTROLLERBUTTONDOWN)
        noteInput(ev->cbutton.which);
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
        m_first_input.erase(ev->cdevice.which); // reconectado, volta ao fim até apertar algo
        while(!m_controllers.empty() && m_controllers.back() == nullptr) m_controllers.pop_back();
    }
}

void Controllers::noteInput(SDL_JoystickID id)
{
    // Só botões contam (não o analógico): um controle parado na mesa com folga no
    // analógico não pode passar na frente de quem está jogando
    if(m_first_input.count(id)) return;
    m_first_input[id] = ++m_inputs;

    // Troca de vaga com o primeiro controle parado (que nunca apertou nada) que esteja
    // antes dele. Quem já apertou nunca muda de vaga: reordenar tudo trocaria de jogador
    // quem está no meio da partida
    auto used = [](SDL_GameController* c) {
        return m_first_input.count(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(c))) > 0;
    };
    size_t mine = m_controllers.size();
    for(size_t k = 0; k < m_controllers.size(); k++)
        if(m_controllers[k] != nullptr && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(m_controllers[k])) == id) mine = k;
    // Botão de um controle que já não está na lista: o aperto ficou na fila e chegou depois
    // do DEVICEREMOVED (o controle repassado pelo usbipd ou pela ponte caiu e voltou). Sem
    // isto, a troca abaixo escrevia em m_controllers[size()], fora do vetor
    if(mine == m_controllers.size()) return;
    for(size_t k = 0; k < mine; k++)
        if(m_controllers[k] != nullptr && !used(m_controllers[k]))
        {
            std::swap(m_controllers[k], m_controllers[mine]);
            break;
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
