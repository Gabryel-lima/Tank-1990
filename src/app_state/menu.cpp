#include "menu.h"
#include "../engine/engine.h"
#include "../engine/renderer.h"
#include "../appconfig.h"
#include "../type.h"
#include "../app_state/game.h"

#include <iostream>

// ----------------------------------------------------- //
#include <SDL2/SDL_mixer.h>
// Ponteiro global para o efeito sonoro do disparo
extern Mix_Chunk* fxPlayerExp;
// ----------------------------------------------------- //

// Construtor do Menu: inicializa os textos do menu, índice e tanque indicador
Menu::Menu()
{
    // Adiciona as opções do menu
    m_menu_texts.push_back("1 Player");
    m_menu_texts.push_back("2 Players");
    m_menu_texts.push_back("3 Players");
    m_menu_texts.push_back("4 Players");
    m_menu_texts.push_back("Exit");
    m_menu_index = 0;

    // Cria o tanque que serve como ponteiro visual no menu
    // Usa controle configurado para o primeiro jogador
    m_tank_pointer = new Player(AppConfig::player_keys.at(0), 0);
    m_tank_pointer->direction = D_RIGHT;
    m_tank_pointer->pos_x = 144;
    m_tank_pointer->pos_y = (m_menu_index + 1) * 32 + 112;
    m_tank_pointer->setFlag(TSF_LIFE);
    m_tank_pointer->update(0);
    m_tank_pointer->clearFlag(TSF_LIFE);
    m_tank_pointer->clearFlag(TSF_SHIELD);
    m_tank_pointer->setFlag(TSF_MENU);
    m_finished = false;
    
    // Inicializa flags de controle
    m_controller_up_pressed = false;
    m_controller_down_pressed = false;
}

// Destrutor do Menu: libera o ponteiro do tanque
Menu::~Menu()
{
    delete m_tank_pointer;
}

// Desenha o menu na tela, incluindo fundo, logo, opções e ponteiro do tanque
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

    // Desenha as opções do menu
    int i = 0;
    SDL_Point text_start;
    for(auto text : m_menu_texts)
    {
        text_start = { 180, (i + 1) * 32 + 120};
        i++;
        renderer->drawText(&text_start, text, {255, 255, 255, 255}, 2);
    }

    // Desenha o tanque que indica a opção selecionada
    m_tank_pointer->draw();

    renderer->flush();
}

// Atualiza o estado do menu (apenas atualiza o tanque ponteiro)
void Menu::update(Uint32 dt)
{
    m_tank_pointer->speed = m_tank_pointer->default_speed;
    m_tank_pointer->stop = true;
    m_tank_pointer->update(dt);
}

// Processa eventos de teclado e controle para navegação e seleção no menu
void Menu::eventProcess(SDL_Event *ev)
{
    if(ev->type == SDL_KEYDOWN)
    {
        // Seta para cima: move seleção para cima
        if(ev->key.keysym.sym == SDLK_UP)
        {
            m_menu_index--;
            if(m_menu_index < 0)
                m_menu_index = m_menu_texts.size() - 1;

            // Atualiza a posição do tanque ponteiro
            m_tank_pointer->pos_y = (m_menu_index + 1) * 32 + 110;
        }
        // Seta para baixo: move seleção para baixo
        else if(ev->key.keysym.sym == SDLK_DOWN)
        {
            m_menu_index++;
            if(m_menu_index >= static_cast<int>(m_menu_texts.size()))
                m_menu_index = 0;

            // Atualiza a posição do tanque ponteiro
            m_tank_pointer->pos_y = (m_menu_index + 1) * 32 + 110;
        }
        // Espaço ou Enter: seleciona a opção atual
        else if(ev->key.keysym.sym == SDLK_SPACE || ev->key.keysym.sym == SDLK_RETURN)
        {
            // sound, vem do extern de app.cpp
            Mix_PlayChannel(1, fxPlayerExp, 0);
            m_finished = true;
        }
        // ESC: sai do menu
        else if(ev->key.keysym.sym == SDLK_ESCAPE)
        {
            m_menu_index = -1;
            m_finished = true;
        }
    }
    else if(ev->type == SDL_CONTROLLERAXISMOTION)
    {
        // Processa movimento do analógico do controle
        const Sint16 DEADZONE = 8000;
        
        if(ev->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
        {
            // Eixo vertical - apenas quando cruza a deadzone
            if(ev->caxis.value < -DEADZONE && !m_controller_up_pressed)
            {
                m_controller_up_pressed = true;
                m_menu_index--;
                if(m_menu_index < 0)
                    m_menu_index = m_menu_texts.size() - 1;
                m_tank_pointer->pos_y = (m_menu_index + 1) * 32 + 110;
            }
            else if(ev->caxis.value > DEADZONE && !m_controller_down_pressed)
            {
                m_controller_down_pressed = true;
                m_menu_index++;
                if(m_menu_index >= static_cast<int>(m_menu_texts.size()))
                    m_menu_index = 0;
                m_tank_pointer->pos_y = (m_menu_index + 1) * 32 + 110;
            }
            else if(ev->caxis.value >= -DEADZONE && ev->caxis.value <= DEADZONE)
            {
                // Reset dos flags quando o analógico volta para o centro
                m_controller_up_pressed = false;
                m_controller_down_pressed = false;
            }
        }
    }
    else if(ev->type == SDL_CONTROLLERBUTTONDOWN)
    {
        // sound, vem do extern de app.cpp
        Mix_PlayChannel(1, fxPlayerExp, 0);

        // Botão A ou Start para selecionar
        if(ev->cbutton.button == SDL_CONTROLLER_BUTTON_A || 
           ev->cbutton.button == SDL_CONTROLLER_BUTTON_START)
        {
            m_finished = true;
        }
        // Botão B ou Back para sair
        else if(ev->cbutton.button == SDL_CONTROLLER_BUTTON_B || 
                ev->cbutton.button == SDL_CONTROLLER_BUTTON_BACK)
        {
            m_menu_index = -1;
            m_finished = true;
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
    // Se a opção for "Exit", retorna nullptr (encerra o app)
    if(m_menu_index == static_cast<int>(m_menu_texts.size()) - 1)
        return nullptr;
    // Se a opção for "1 Player", inicia o jogo para 1 jogador
    else if(m_menu_index == 0)
    {
        Game* g = new Game(1);
        return g;
    }
    // Se a opção for "2 Players", inicia o jogo para 2 jogadores
    else if(m_menu_index == 1)
    {
        Game* g = new Game(2);
        return g;
    }
    // Se a opção for "2 Players", inicia o jogo para 3 jogadores
    else if (m_menu_index == 2)
    {
        Game* g = new Game(3);
        return g;
    }
    // Se a opção for "4 Players", inicia o jogo para 4 jogadores
    else if (m_menu_index == 3)
    {
        Game* g = new Game(4);
        return g;
    }
    // Caso padrão: retorna nullptr
    return nullptr;
}
