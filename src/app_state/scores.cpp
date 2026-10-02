#include "scores.h"
#include "../engine/engine.h"
#include "../appconfig.h"
#include "game.h"
#include "menu.h"

// Construtor padrão: inicializa variáveis do estado de pontuação
Scores::Scores()
{
    m_show_time = 0;           // Tempo de exibição da tela de pontuação
    m_level = 0;               // Nível atual
    m_game_over = true;        // Indica se o jogo terminou
    m_score_counter_run = true;// Controla a animação do contador de pontos
    m_score_counter = 0;       // Valor atual do contador de pontos (animação)
    m_max_score = 0;           // Maior pontuação entre os jogadores
}

// Construtor com jogadores, nível e status de game over
Scores::Scores(std::vector<Player *> players, int level, bool game_over)
{
    m_players = players;
    m_level = level;
    m_game_over = game_over;
    m_show_time = 0;
    m_score_counter_run = true;
    m_score_counter = 0;
    m_max_score = 0;
    // Inicializa cada jogador para a tela de pontuação
    for(auto player : m_players)
    {
        player->to_erase = false; // Garante que o jogador não será removido
        // Se o jogador perdeu todas as vidas mas o jogo não acabou, dá 2 vidas para continuar
        if(player->lives_count == 0 && !game_over) player->lives_count = 2;
        else player->lives_count++; // Caso contrário, incrementa uma vida
        player->respawn();          // Reposiciona o jogador
        player->setFlag(TSF_MENU);  // Marca o jogador como presente no menu

        // Atualiza a maior pontuação
        if(player->score > m_max_score) m_max_score = player->score;
    }
}

// Desenha a tela de pontuação
void Scores::draw()
{
    Renderer* renderer = Engine::getEngine().getRenderer();
    renderer->clear();

    // Desenha fundo das áreas do mapa e status
    renderer->drawRect(&AppConfig::map_rect, {0, 0, 0, 255}, true);
    renderer->drawRect(&AppConfig::status_rect, {0, 0, 0, 255}, true);

    SDL_Point p_dst;
    SDL_Rect dst;

    // Tudo alinhado ao centro da janela (a tela inteira é preta, mapa + painel),
    // inclusive a tabela, que antes ficava 7 px fora do eixo do título
    const int center_x = AppConfig::windows_rect.x + (AppConfig::map_rect.w + AppConfig::status_rect.w) / 2;
    const int left = center_x - 125, right = center_x + 125;
    const SDL_Color WHITE = {255, 255, 255, 255};

    // Desenha o nome do estágio/nível
    renderer->drawTextCentered(center_x, 10, std::string("STAGE ") + Engine::intToString(m_level), {255, 255, 220, 255}, 1);
    // Cabeçalhos das colunas: "PLAYER" alinhado à esquerda, "SCORE" à direita (como os números)
    p_dst = {left, 50};
    renderer->drawText(&p_dst, std::string("PLAYER"), WHITE, 2);
    p_dst = {right - renderer->textSize("SCORE", 2).x, 50};
    renderer->drawText(&p_dst, std::string("SCORE"), WHITE, 2);
    // Linha divisória
    dst = {left - 25, 75, (right - left) + 50, 2};
    renderer->drawRect(&dst, {250, 250, 200, 255}, true);

    int i = 0;
    // Para cada jogador, desenha sprite (na cor dele), vidas e pontuação
    for(auto player : m_players)
    {
        int row_y = 90 + i * player->src_rect.h;
        // Sprite do jogador
        dst = {left, row_y, player->src_rect.w, player->src_rect.h};
        renderer->drawObjectWithColor(&player->src_rect, &dst, player->color);
        // Quantidade de vidas
        p_dst = {left + 40, row_y + 8};
        renderer->drawText(&p_dst, std::string("x") + Engine::intToString(player->lives_count), WHITE, 2);
        // Pontuação (animação do contador), alinhada à direita
        std::string score = (m_score_counter < player->score ? Engine::intToString(m_score_counter) : Engine::intToString(player->score));
        p_dst = {right - renderer->textSize(score, 2).x, row_y + 8};
        renderer->drawText(&p_dst, score, WHITE, 2);
        i++;
    }

    renderer->flush();
}

// Atualiza a animação do contador de pontos e o estado dos jogadores
void Scores::update(Uint32 dt)
{
    // Se o contador de pontos já passou do máximo, começa a contar o tempo de exibição
    if(m_score_counter > (1 << 30) || m_score_counter > m_max_score)
    {
        m_show_time += dt;
        m_score_counter_run = false;
    }
    // Animação do contador de pontos (incrementa mais rápido conforme o valor)
    if(m_score_counter_run)
    {
        if(m_score_counter < 20) m_score_counter++;
        else if(m_score_counter < 200) m_score_counter += 10;
        else if(m_score_counter < 2000) m_score_counter += 100;
        else if(m_score_counter < 20000) m_score_counter += 1000;
        else if(m_score_counter < 200000) m_score_counter += 10000;
        else m_score_counter += 100000;
    }
    // Atualiza cada jogador para garantir que fiquem parados e com sprite correto
    for(auto player : m_players)
    {
        player->speed = player->default_speed; // Restaura velocidade padrão
        player->stop = true;                   // Mantém parado
        player->setDirection(D_RIGHT);         // Direção padrão para sprite
        player->update(dt);                    // Atualiza animação do sprite
    }
}

// Processa eventos de teclado na tela de pontuação
void Scores::eventProcess(SDL_Event *ev)
{
    // Se pressionar ENTER (ou A/Start no controle), pula animação do contador
    // ou avança para o próximo estado
    bool skip = (ev->type == SDL_KEYDOWN && ev->key.keysym.sym == SDLK_RETURN) ||
                (ev->type == SDL_CONTROLLERBUTTONDOWN &&
                 (ev->cbutton.button == SDL_CONTROLLER_BUTTON_A ||
                  ev->cbutton.button == SDL_CONTROLLER_BUTTON_START));
    if(skip)
    {
        // Se já terminou a contagem, força o fim da tela de pontuação
        if(m_score_counter > (1 << 30)) m_show_time = AppConfig::score_show_time + 1;
        // Caso contrário, termina imediatamente a animação do contador
        else m_score_counter = (1 << 30) + 1;
    }
}

// Indica se a tela de pontuação terminou (tempo de exibição esgotado)
bool Scores::finished() const
{
    return m_show_time > AppConfig::score_show_time;
}

// Retorna o próximo estado do jogo após a tela de pontuação
AppState *Scores::nextState()
{
    // Se o jogo acabou, volta para o menu principal
    if(m_game_over)
    {
        Menu* m = new Menu;
        return m;
    }
    // Caso contrário, inicia o próximo nível com os mesmos jogadores
    Game* g = new Game(m_players, m_level);
    return g;
}
