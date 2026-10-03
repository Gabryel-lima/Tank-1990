#include "menu.h"
#include "../engine/engine.h"
#include "../engine/renderer.h"
#include "../appconfig.h"
#include "../type.h"
#include "../app_state/game.h"
#include "../app_state/duel.h"
#include "../app_state/duel_layout.h"
#include "../soundmanager.h"
#include "../controllers.h"

#include <algorithm>
#include <iostream>

DuelConfig Menu::s_duel_config;
bool Menu::s_duel_custom = false;

namespace
{
    const SDL_Color WHITE = {255, 255, 255, 255};
    const SDL_Color GRAY = {120, 120, 120, 255};
    const SDL_Color RED = {230, 40, 40, 255};

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
    m_tank_pointer = new Player(0);
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

    // Grades dos mapas do duelo para as miniaturas (um mapa ausente fica com a grade vazia)
    for(auto& map : AppConfig::duel_maps)
        m_map_grids.push_back(DuelLayout::readMap(AppConfig::duel_levels_path + map.first));

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
        // Só jogadores humanos (até 4): 3 vs 3 e 4 vs 4 não cabem
        m_items = {ITEM_FORMAT_1V1, ITEM_FORMAT_2V2, ITEM_FORMAT_CUSTOM, ITEM_BACK};
        break;
    case SCREEN_DUEL_SETUP:
        // Nos formatos fixos a quantidade de jogadores já está definida
        if(s_duel_custom) m_items.push_back(ITEM_HUMANS);
        for(int i = 0; i < s_duel_config.humans; i++)
            m_items.push_back(static_cast<Item>(ITEM_HUMAN_1_TEAM + i));
        m_items.push_back(ITEM_NEXT);
        m_items.push_back(ITEM_BACK);
        break;
    case SCREEN_DUEL_MAP:
        for(size_t i = 0; i < AppConfig::duel_maps.size(); i++)
            m_items.push_back(static_cast<Item>(ITEM_MAP_FIRST + i));
        m_items.push_back(ITEM_MAP_RANDOM);
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
    // Na configuração do duelo, começa no "Next"; na escolha de mapa, no último escolhido
    // (assim a revanche é um botão só)
    if(screen == SCREEN_DUEL_SETUP)
        m_menu_index = std::find(m_items.begin(), m_items.end(), ITEM_NEXT) - m_items.begin();
    if(screen == SCREEN_DUEL_MAP)
    {
        Item last = s_duel_config.map < 0 ? ITEM_MAP_RANDOM : static_cast<Item>(ITEM_MAP_FIRST + s_duel_config.map);
        auto it = std::find(m_items.begin(), m_items.end(), last);
        m_menu_index = (it != m_items.end()) ? it - m_items.begin() : 0;
    }
}

int Menu::itemY(int i) const
{
    // Tela principal como no original (152, 184, 216...); nas demais, a linha
    // 152 é o título da tela e os itens começam na seguinte
    if(m_screen == SCREEN_MAIN) return rowY(i + 1);
    return rowY(i + 2);
}

bool Menu::isValueItem(Item item) const
{
    return item == ITEM_HUMANS || (item >= ITEM_HUMAN_1_TEAM && item <= ITEM_HUMAN_4_TEAM);
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
    case ITEM_FORMAT_CUSTOM: return "Custom Teams";
    case ITEM_HUMANS: return "Players   < " + Engine::intToString(c.humans) + " >";
    case ITEM_HUMAN_1_TEAM:
    case ITEM_HUMAN_2_TEAM:
    case ITEM_HUMAN_3_TEAM:
    case ITEM_HUMAN_4_TEAM:
    {
        // "P1 PAD 1  < A >": dispositivo do jogador (decidido automaticamente) e a equipe,
        // com o valor na mesma coluna do "Players   < 2 >"
        int i = item - ITEM_HUMAN_1_TEAM;
        std::string input = Controllers::inputName(c.humans, i);
        input.resize(6, ' ');
        return "P" + Engine::intToString(i + 1) + " " + input + " < " + (c.human_team[i] == 0 ? "A" : "B") + " >";
    }
    case ITEM_NEXT: return "Next";
    case ITEM_MAP_RANDOM: return "Random";
    case ITEM_BACK: return "Back";
    default: break;
    }
    int map = item - ITEM_MAP_FIRST;
    if(map >= 0 && map < static_cast<int>(AppConfig::duel_maps.size()))
        return AppConfig::duel_maps[map].second;
    return "";
}

void Menu::applyDuelFormat(int team_size)
{
    DuelConfig& c = s_duel_config;
    c.humans = 2 * team_size;
    // Jogadores alternam entre as equipes: P1 em A, P2 em B, P3 em A, P4 em B
    for(int i = 0; i < 4; i++) c.human_team[i] = i % 2;
    syncTeamSizes();
}

void Menu::syncTeamSizes()
{
    // Sem bots ocupando vagas, cada equipe tem exatamente os jogadores escolhidos para ela
    for(int team = 0; team < 2; team++)
        s_duel_config.team_size[team] = s_duel_config.humansInTeam(team);
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
    if(item == ITEM_HUMANS)
    {
        // 2 a 4 jogadores; as equipes recomeçam alternando (P1 em A, P2 em B...)
        int humans = c.humans + delta;
        if(humans > 4) humans = 2;
        else if(humans < 2) humans = 4;
        c.humans = humans;
        for(int i = 0; i < 4; i++) c.human_team[i] = i % 2;
        buildItems(); // a lista de "Pn team" muda de tamanho
    }
    else
    {
        int i = item - ITEM_HUMAN_1_TEAM;
        int from = c.human_team[i], to = 1 - from;
        // No personalizado o jogador muda de equipe, desde que a dele não fique vazia.
        // Nos formatos fixos (ou se ficaria vazia), troca de lugar com o último jogador da outra equipe.
        if(!(s_duel_custom && c.humansInTeam(from) > 1))
            for(int j = c.humans - 1; j >= 0; j--)
                if(j != i && c.human_team[j] == to) { c.human_team[j] = from; break; }
        c.human_team[i] = to;
    }
    syncTeamSizes();
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
        s_duel_custom = false;
        applyDuelFormat(item - ITEM_FORMAT_1V1 + 1);
        openScreen(SCREEN_DUEL_SETUP);
        break;
    case ITEM_FORMAT_CUSTOM:
        s_duel_custom = true;
        syncTeamSizes();
        openScreen(SCREEN_DUEL_SETUP);
        m_menu_index = 0; // no personalizado, começa pela quantidade de jogadores
        break;
    case ITEM_NEXT:
        // Só avança se todo jogador tiver controle ou teclado (o título explica o que falta)
        if(Controllers::playersWithoutInput(s_duel_config.humans) > 0) break;
        openScreen(SCREEN_DUEL_MAP);
        break;
    case ITEM_MAP_RANDOM:
        s_duel_config.map = -1;
        m_result = RESULT_DUEL;
        m_finished = true;
        break;
    case ITEM_BACK:
        back();
        break;
    default:
    {
        // Um dos mapas: começa o duelo nele
        int map = item - ITEM_MAP_FIRST;
        if(map >= 0 && map < static_cast<int>(AppConfig::duel_maps.size()))
        {
            s_duel_config.map = map;
            m_result = RESULT_DUEL;
            m_finished = true;
        }
        break;
    }
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
    case SCREEN_DUEL_MAP:
        openScreen(SCREEN_DUEL_SETUP);
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

    // Desenha o LOGO do jogo centralizado
    const SpriteData* logo = Engine::getEngine().getSpriteConfig()->getSpriteData(ST_TANKS_LOGO);
    SDL_Rect dst = {(AppConfig::map_rect.w + AppConfig::status_rect.w - logo->rect.w)/2, 50, logo->rect.w, logo->rect.h};
    renderer->drawObject(&logo->rect, &dst);

    SDL_Point text_start;

    // Título da tela: uma linha da grade acima do primeiro item, na mesma coluna
    std::string title;
    if(m_screen == SCREEN_EXTRA) title = "Extra Modes";
    else if(m_screen == SCREEN_DUEL_FORMAT) title = "Duel Mode";
    else if(m_screen == SCREEN_DUEL_MAP) title = "Select Map";
    else if(m_screen == SCREEN_DUEL_SETUP)
    {
        // Mostra a divisão atual das equipes, que muda ao trocar jogadores de lado
        const DuelConfig& c = s_duel_config;
        title = "Duel  " + Engine::intToString(c.humansInTeam(0)) + " vs " + Engine::intToString(c.humansInTeam(1));
    }
    // Na configuração do duelo, jogadores sem controle nem teclado bloqueiam o início:
    // o título vira o aviso (atualiza sozinho ao conectar um controle)
    int missing = (m_screen == SCREEN_DUEL_SETUP ? Controllers::playersWithoutInput(s_duel_config.humans) : 0);
    SDL_Color title_color = GRAY;
    if(missing > 0)
    {
        title = "Connect " + Engine::intToString(missing) + (missing == 1 ? " pad" : " pads");
        title_color = RED;
    }
    if(!title.empty())
    {
        text_start = {TEXT_X, itemY(-1)};
        renderer->drawText(&text_start, title, title_color, 2);
    }

    // Desenha as opções do menu
    for(size_t i = 0; i < m_items.size(); i++)
    {
        Item item = m_items[i];
        SDL_Color color = WHITE;
        if(item == ITEM_NEXT && missing > 0) color = GRAY;
        if(item >= ITEM_HUMAN_1_TEAM && item <= ITEM_HUMAN_4_TEAM &&
           Controllers::inputName(s_duel_config.humans, item - ITEM_HUMAN_1_TEAM) == "NO PAD")
            color = RED;
        text_start = {TEXT_X, itemY(i)};
        renderer->drawText(&text_start, itemText(item), color, 2);
        // "P1" na cor da equipe escolhida (muda junto ao trocar de equipe)
        if(item >= ITEM_HUMAN_1_TEAM && item <= ITEM_HUMAN_4_TEAM && color.r == WHITE.r && color.g == WHITE.g)
        {
            int idx = item - ITEM_HUMAN_1_TEAM;
            renderer->drawText(&text_start, "P" + Engine::intToString(idx + 1), Duel::teamColor(s_duel_config.human_team[idx]), 2);
        }
    }

    // Miniatura do mapa selecionado, à esquerda da lista
    if(m_screen == SCREEN_DUEL_MAP && !m_items.empty())
    {
        int map = m_items[m_menu_index] - ITEM_MAP_FIRST;
        drawMapPreview(map >= 0 && map < static_cast<int>(m_map_grids.size()) ? map : -1);
    }

    // Desenha o tanque que indica a opção selecionada
    m_tank_pointer->pos_y = itemY(m_menu_index) - 10;
    m_tank_pointer->dest_rect.y = m_tank_pointer->pos_y;
    m_tank_pointer->draw();

    renderer->flush();
}

void Menu::drawMapPreview(int map_index)
{
    // 26 x 26 tiles de 5 px = 130 px, na área livre à esquerda do ponteiro (x < 144),
    // com o topo alinhado ao primeiro item da lista
    const int TILE = 5;
    SDL_Rect frame = {6, itemY(0) - 2, 26 * TILE + 4, 26 * TILE + 4};
    Renderer* renderer = Engine::getEngine().getRenderer();
    renderer->drawRect(&frame, GRAY, false);
    SDL_Rect inside = {frame.x + 2, frame.y + 2, 26 * TILE, 26 * TILE};
    renderer->drawRect(&inside, {16, 16, 16, 255}, true);

    if(map_index < 0 || m_map_grids[map_index].empty())
    {
        // Aleatório (ou mapa não encontrado)
        SDL_Point size = renderer->textSize("?", 1);
        SDL_Point p = {inside.x + (inside.w - size.x) / 2, inside.y + (inside.h - size.y) / 2};
        renderer->drawText(&p, "?", GRAY, 1);
        return;
    }

    const std::vector<std::string>& grid = m_map_grids[map_index];
    for(size_t r = 0; r < grid.size() && r < 26; r++)
        for(size_t c = 0; c < grid[r].size() && c < 26; c++)
        {
            SDL_Color color;
            switch(grid[r][c])
            {
            case '#': color = {170, 70, 20, 255}; break;   // tijolo
            case '@': color = {170, 170, 170, 255}; break; // pedra
            case '~': color = {40, 80, 220, 255}; break;   // água
            case '%': color = {40, 150, 40, 255}; break;   // arbusto
            case '-': color = {190, 210, 230, 255}; break; // gelo
            default: continue;
            }
            SDL_Rect tile = {inside.x + static_cast<int>(c) * TILE, inside.y + static_cast<int>(r) * TILE, TILE, TILE};
            renderer->drawRect(&tile, color, true);
        }

    // Bases nas cores das equipes: A embaixo, B em cima
    for(int team = 0; team < 2; team++)
    {
        SDL_Rect base = {inside.x + 12 * TILE, inside.y + (team == 0 ? 24 : 0) * TILE, 2 * TILE, 2 * TILE};
        renderer->drawRect(&base, Duel::teamColor(team), true);
    }
}

// Atualiza o estado do menu (apenas atualiza o tanque ponteiro)
void Menu::update(Uint32 dt)
{
    // Posiciona também o retângulo de desenho: durante a animação de criação
    // o Tank::update ainda não o atualiza, e o ponteiro apareceria fora do lugar
    m_tank_pointer->pos_y = itemY(m_menu_index) - 10;
    m_tank_pointer->dest_rect.y = m_tank_pointer->pos_y;
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
