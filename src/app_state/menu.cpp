#include "menu.h"
#include "../engine/engine.h"
#include "../engine/renderer.h"
#include "../appconfig.h"
#include "../type.h"
#include "../app_state/game.h"
#include "../app_state/duel.h"
#include "../soundmanager.h"

#include <algorithm>
#include <iostream>

DuelConfig Menu::s_duel_config;
bool Menu::s_duel_custom = false;

namespace
{
    const SDL_Color WHITE = {255, 255, 255, 255};
    const SDL_Color GRAY = {120, 120, 120, 255};

    // Grade do menu original: textos na coluna x = 180, uma linha a cada 32 px,
    // com o primeiro item em y = 152. Todas as telas usam essa mesma grade.
    const int TEXT_X = 180;
    const int ROW_HEIGHT = 32;
    int rowY(int row) { return 120 + row * ROW_HEIGHT; }
}

// Construtor do Menu: inicializa a tela, o índice e o tanque indicador
Menu::Menu(Screen screen)
{
    m_menu_index = 0;
    m_result = RESULT_NONE;
    m_campaign_players = 1;

    // Cria o tanque que serve como ponteiro visual no menu
    // Usa controle configurado para o primeiro jogador
    m_tank_pointer = new Player(AppConfig::player_keys.at(0), 0);
    m_tank_pointer->direction = D_RIGHT;
    m_tank_pointer->pos_x = 144;
    m_tank_pointer->setFlag(TSF_LIFE);
    m_tank_pointer->update(0);
    m_tank_pointer->clearFlag(TSF_LIFE);
    m_tank_pointer->clearFlag(TSF_SHIELD);
    m_tank_pointer->setFlag(TSF_MENU);
    m_finished = false;

    // Inicializa flags de controle
    m_controller_up_pressed = false;
    m_controller_down_pressed = false;
    m_controller_left_pressed = false;
    m_controller_right_pressed = false;

    openScreen(screen);
}

// Destrutor do Menu: libera o ponteiro do tanque
Menu::~Menu()
{
    delete m_tank_pointer;
}

// ======================== Itens e telas ========================

void Menu::buildItems()
{
    m_items.clear();
    switch(m_screen)
    {
    case SCREEN_MAIN:
        m_items = {ITEM_CAMPAIGN_1, ITEM_CAMPAIGN_2, ITEM_CAMPAIGN_3, ITEM_CAMPAIGN_4, ITEM_EXTRA_MODES, ITEM_EXIT};
        break;
    case SCREEN_EXTRA:
        m_items = {ITEM_DUEL_MODE, ITEM_BACK};
        break;
    case SCREEN_DUEL_FORMAT:
        m_items = {ITEM_FORMAT_1V1, ITEM_FORMAT_2V2, ITEM_FORMAT_3V3, ITEM_FORMAT_4V4, ITEM_FORMAT_CUSTOM, ITEM_BACK};
        break;
    case SCREEN_DUEL_SETUP:
        if(s_duel_custom)
        {
            m_items.push_back(ITEM_TEAM_A_SIZE);
            m_items.push_back(ITEM_TEAM_B_SIZE);
        }
        m_items.push_back(ITEM_HUMANS);
        for(int i = 0; i < s_duel_config.humans; i++)
            m_items.push_back(static_cast<Item>(ITEM_HUMAN_1_TEAM + i));
        m_items.push_back(ITEM_START);
        m_items.push_back(ITEM_BACK);
        break;
    }
    if(m_menu_index >= static_cast<int>(m_items.size())) m_menu_index = m_items.size() - 1;
}

void Menu::openScreen(Screen screen)
{
    m_screen = screen;
    m_menu_index = 0;
    buildItems();
    // Na configuração do duelo, começa no "Start" para a revanche ser rápida
    if(screen == SCREEN_DUEL_SETUP)
        m_menu_index = std::find(m_items.begin(), m_items.end(), ITEM_START) - m_items.begin();
}

int Menu::itemY(int i) const
{
    switch(m_screen)
    {
    case SCREEN_MAIN:
        return rowY(i + 1);   // como no original: 152, 184, 216...
    case SCREEN_EXTRA:
    case SCREEN_DUEL_FORMAT:
        return rowY(i + 2);   // a linha 1 (152) é o título da tela
    case SCREEN_DUEL_SETUP:
        // Até 9 itens + título + linha de bots: sem o logo, a lista começa
        // 3 linhas acima, na mesma grade (título em 56, itens a partir de 88)
        return rowY(i - 1);
    }
    return rowY(i + 1);
}

bool Menu::isValueItem(Item item) const
{
    return item == ITEM_TEAM_A_SIZE || item == ITEM_TEAM_B_SIZE || item == ITEM_HUMANS ||
           (item >= ITEM_HUMAN_1_TEAM && item <= ITEM_HUMAN_4_TEAM);
}

std::string Menu::itemText(Item item) const
{
    const DuelConfig& c = s_duel_config;
    switch(item)
    {
    case ITEM_CAMPAIGN_1: return "1 Player";
    case ITEM_CAMPAIGN_2: return "2 Players";
    case ITEM_CAMPAIGN_3: return "3 Players";
    case ITEM_CAMPAIGN_4: return "4 Players";
    case ITEM_EXTRA_MODES: return "Extra Modes";
    case ITEM_EXIT: return "Exit";
    case ITEM_DUEL_MODE: return "Duel Mode";
    case ITEM_FORMAT_1V1: return "1 vs 1";
    case ITEM_FORMAT_2V2: return "2 vs 2";
    case ITEM_FORMAT_3V3: return "3 vs 3";
    case ITEM_FORMAT_4V4: return "4 vs 4";
    case ITEM_FORMAT_CUSTOM: return "Custom Teams";
    case ITEM_TEAM_A_SIZE: return "Team A    < " + Engine::intToString(c.team_size[0]) + " >";
    case ITEM_TEAM_B_SIZE: return "Team B    < " + Engine::intToString(c.team_size[1]) + " >";
    case ITEM_HUMANS: return "Players   < " + Engine::intToString(c.humans) + " >";
    case ITEM_HUMAN_1_TEAM:
    case ITEM_HUMAN_2_TEAM:
    case ITEM_HUMAN_3_TEAM:
    case ITEM_HUMAN_4_TEAM:
    {
        int i = item - ITEM_HUMAN_1_TEAM;
        return "P" + Engine::intToString(i + 1) + " team   < " + (c.human_team[i] == 0 ? "A" : "B") + " >";
    }
    case ITEM_START: return "Start";
    case ITEM_BACK: return "Back";
    }
    return "";
}

void Menu::applyDuelFormat(int team_size)
{
    DuelConfig& c = s_duel_config;
    c.team_size[0] = c.team_size[1] = team_size;
    c.humans = std::max(1, std::min({c.humans, 4, 2 * team_size}));
    // Humanos alternam entre as equipes: P1 em A, P2 em B, P3 em A, P4 em B
    for(int i = 0; i < 4; i++) c.human_team[i] = i % 2;
}

// ======================== Ações ========================

void Menu::moveSelection(int delta)
{
    m_menu_index += delta;
    if(m_menu_index < 0) m_menu_index = m_items.size() - 1;
    else if(m_menu_index >= static_cast<int>(m_items.size())) m_menu_index = 0;
}

void Menu::changeValue(int delta)
{
    if(m_items.empty()) return;
    Item item = m_items.at(m_menu_index);
    if(!isValueItem(item)) return;

    DuelConfig& c = s_duel_config;
    if(item == ITEM_TEAM_A_SIZE || item == ITEM_TEAM_B_SIZE)
    {
        // A equipe não pode ficar menor do que a quantidade de humanos nela
        int team = (item == ITEM_TEAM_A_SIZE ? 0 : 1);
        int min_size = std::max(1, c.humansInTeam(team));
        int size = c.team_size[team] + delta;
        if(size > 4) size = min_size;
        else if(size < min_size) size = 4;
        c.team_size[team] = size;
    }
    else if(item == ITEM_HUMANS)
    {
        int max_humans = std::min(4, c.team_size[0] + c.team_size[1]);
        int humans = c.humans + delta;
        if(humans > max_humans) humans = 1;
        else if(humans < 1) humans = max_humans;

        // Recalcula as equipes do zero: P1 em A, os demais alternando, sempre respeitando as vagas
        c.humans = humans;
        int used[2] = {0, 0};
        for(int i = 0; i < humans; i++)
        {
            int team = i % 2;
            if(used[team] >= c.team_size[team]) team = 1 - team;
            c.human_team[i] = team;
            used[team]++;
        }
        buildItems(); // a lista de "Pn team" muda de tamanho
    }
    else
    {
        int i = item - ITEM_HUMAN_1_TEAM;
        int from = c.human_team[i], to = 1 - from;
        if(c.humansInTeam(to) < c.team_size[to])
            c.human_team[i] = to;
        else if(s_duel_custom && c.team_size[to] < 4)
        {
            // Equipe cheia no modo personalizado: abre mais uma vaga nela
            c.team_size[to]++;
            c.human_team[i] = to;
        }
        else
        {
            // Equipe cheia: troca de lugar com o último humano da outra equipe
            for(int j = c.humans - 1; j >= 0; j--)
                if(c.human_team[j] == to) { c.human_team[j] = from; break; }
            c.human_team[i] = to;
        }
    }
}

void Menu::confirm()
{
    if(m_items.empty()) return;
    Item item = m_items.at(m_menu_index);

    if(isValueItem(item))
    {
        changeValue(+1);
        return;
    }

    switch(item)
    {
    case ITEM_CAMPAIGN_1:
    case ITEM_CAMPAIGN_2:
    case ITEM_CAMPAIGN_3:
    case ITEM_CAMPAIGN_4:
        m_campaign_players = item - ITEM_CAMPAIGN_1 + 1;
        m_result = RESULT_CAMPAIGN;
        m_finished = true;
        break;
    case ITEM_EXTRA_MODES:
        openScreen(SCREEN_EXTRA);
        break;
    case ITEM_EXIT:
        m_result = RESULT_EXIT;
        m_finished = true;
        break;
    case ITEM_DUEL_MODE:
        openScreen(SCREEN_DUEL_FORMAT);
        break;
    case ITEM_FORMAT_1V1:
    case ITEM_FORMAT_2V2:
    case ITEM_FORMAT_3V3:
    case ITEM_FORMAT_4V4:
        s_duel_custom = false;
        applyDuelFormat(item - ITEM_FORMAT_1V1 + 1);
        openScreen(SCREEN_DUEL_SETUP);
        break;
    case ITEM_FORMAT_CUSTOM:
        s_duel_custom = true;
        openScreen(SCREEN_DUEL_SETUP);
        m_menu_index = 0; // no personalizado, começa pelo tamanho das equipes
        break;
    case ITEM_START:
        m_result = RESULT_DUEL;
        m_finished = true;
        break;
    case ITEM_BACK:
        back();
        break;
    default:
        break;
    }
}

void Menu::back()
{
    switch(m_screen)
    {
    case SCREEN_MAIN:
        m_result = RESULT_EXIT;
        m_finished = true;
        break;
    case SCREEN_EXTRA:
        openScreen(SCREEN_MAIN);
        m_menu_index = std::find(m_items.begin(), m_items.end(), ITEM_EXTRA_MODES) - m_items.begin();
        break;
    case SCREEN_DUEL_FORMAT:
        openScreen(SCREEN_EXTRA);
        break;
    case SCREEN_DUEL_SETUP:
        openScreen(SCREEN_DUEL_FORMAT);
        break;
    }
}

// ======================== Laço ========================

// Desenha o menu na tela, incluindo fundo, logo/título, opções e ponteiro do tanque
void Menu::draw()
{
    Renderer* renderer = Engine::getEngine().getRenderer();
    renderer->clear();

    // Desenha as áreas do mapa e status
    renderer->drawRect(&AppConfig::map_rect, {0, 0, 0, 255}, true);
    renderer->drawRect(&AppConfig::status_rect, {0, 0, 0, 255}, true);

    SDL_Point text_start;
    if(m_screen != SCREEN_DUEL_SETUP)
    {
        // Desenha o LOGO do jogo centralizado
        const SpriteData* logo = Engine::getEngine().getSpriteConfig()->getSpriteData(ST_TANKS_LOGO);
        SDL_Rect dst = {(AppConfig::map_rect.w + AppConfig::status_rect.w - logo->rect.w)/2, 50, logo->rect.w, logo->rect.h};
        renderer->drawObject(&logo->rect, &dst);
    }

    // Título da tela: uma linha da grade acima do primeiro item, na mesma coluna
    std::string title;
    if(m_screen == SCREEN_EXTRA) title = "Extra Modes";
    else if(m_screen == SCREEN_DUEL_FORMAT) title = "Duel Mode";
    else if(m_screen == SCREEN_DUEL_SETUP)
    {
        const DuelConfig& c = s_duel_config;
        title = s_duel_custom ? std::string("Duel  Custom")
              : "Duel  " + Engine::intToString(c.team_size[0]) + " vs " + Engine::intToString(c.team_size[1]);
    }
    if(!title.empty())
    {
        text_start = {TEXT_X, itemY(-1)};
        renderer->drawText(&text_start, title, GRAY, 2);
    }

    // Desenha as opções do menu
    for(size_t i = 0; i < m_items.size(); i++)
    {
        text_start = {TEXT_X, itemY(i)};
        renderer->drawText(&text_start, itemText(m_items[i]), WHITE, 2);
    }

    // Bots de cada equipe, na linha seguinte da grade e na coluna dos valores
    // ("Players   < 2 >": o valor começa no 11º caractere). A fonte é monoespaçada,
    // então os espaços à esquerda alinham as partes de cores diferentes.
    if(m_screen == SCREEN_DUEL_SETUP)
    {
        const DuelConfig& c = s_duel_config;
        text_start = {TEXT_X, itemY(m_items.size())};
        renderer->drawText(&text_start, "CPU", GRAY, 2);
        for(int team = 0; team < 2; team++)
        {
            int bots = c.team_size[team] - c.humansInTeam(team);
            std::string value = std::string(team == 0 ? 10 : 13, ' ') + (team == 0 ? "A" : "B") + Engine::intToString(bots);
            renderer->drawText(&text_start, value, AppConfig::duel_team_colors.at(team), 2);
        }
    }

    // Desenha o tanque que indica a opção selecionada
    m_tank_pointer->pos_y = itemY(m_menu_index) - 10;
    m_tank_pointer->draw();

    renderer->flush();
}

// Atualiza o estado do menu (apenas atualiza o tanque ponteiro)
void Menu::update(Uint32 dt)
{
    m_tank_pointer->pos_y = itemY(m_menu_index) - 10;
    m_tank_pointer->speed = m_tank_pointer->default_speed;
    m_tank_pointer->stop = true;
    m_tank_pointer->update(dt);
}

// Processa eventos de teclado e controle para navegação e seleção no menu
void Menu::eventProcess(SDL_Event *ev)
{
    if(ev->type == SDL_KEYDOWN)
    {
        switch(ev->key.keysym.sym)
        {
        case SDLK_UP:     moveSelection(-1); break;
        case SDLK_DOWN:   moveSelection(+1); break;
        case SDLK_LEFT:   changeValue(-1); break;
        case SDLK_RIGHT:  changeValue(+1); break;
        case SDLK_SPACE:
        case SDLK_RETURN: confirm(); break;
        case SDLK_ESCAPE: back(); break;
        default: break;
        }
    }
    else if(ev->type == SDL_CONTROLLERAXISMOTION)
    {
        // Processa movimento do analógico: age só quando cruza a zona morta
        const Sint16 DEADZONE = 8000;
        Sint16 value = ev->caxis.value;

        if(ev->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
        {
            if(value < -DEADZONE && !m_controller_up_pressed)
            {
                m_controller_up_pressed = true;
                moveSelection(-1);
            }
            else if(value > DEADZONE && !m_controller_down_pressed)
            {
                m_controller_down_pressed = true;
                moveSelection(+1);
            }
            else if(value >= -DEADZONE && value <= DEADZONE)
            {
                // Reset dos flags quando o analógico volta para o centro
                m_controller_up_pressed = false;
                m_controller_down_pressed = false;
            }
        }
        else if(ev->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
        {
            if(value < -DEADZONE && !m_controller_left_pressed)
            {
                m_controller_left_pressed = true;
                changeValue(-1);
            }
            else if(value > DEADZONE && !m_controller_right_pressed)
            {
                m_controller_right_pressed = true;
                changeValue(+1);
            }
            else if(value >= -DEADZONE && value <= DEADZONE)
            {
                m_controller_left_pressed = false;
                m_controller_right_pressed = false;
            }
        }
    }
    else if(ev->type == SDL_CONTROLLERBUTTONDOWN)
    {
        switch(ev->cbutton.button)
        {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:    moveSelection(-1); break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  moveSelection(+1); break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  changeValue(-1); break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: changeValue(+1); break;
        // Botão A ou Start para selecionar
        case SDL_CONTROLLER_BUTTON_A:
        case SDL_CONTROLLER_BUTTON_START:      confirm(); break;
        // Botão B ou Back para voltar
        case SDL_CONTROLLER_BUTTON_B:
        case SDL_CONTROLLER_BUTTON_BACK:       back(); break;
        default: break;
        }
    }
}

// Indica se o menu foi finalizado (alguma opção foi selecionada)
bool Menu::finished() const
{
    return m_finished;
}

// Retorna o próximo estado do aplicativo conforme a opção selecionada
AppState* Menu::nextState()
{
    switch(m_result)
    {
    case RESULT_CAMPAIGN:
        return new Game(m_campaign_players);
    case RESULT_DUEL:
        return new Duel(s_duel_config);
    default:
        // "Exit" ou Esc na tela principal: encerra o app
        return nullptr;
    }
}
