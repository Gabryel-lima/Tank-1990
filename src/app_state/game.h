#ifndef GAME_H
#define GAME_H

#include "appstate.h"
#include "../objects/object.h"
#include "../objects/player.h"
#include "../objects/enemy.h"
#include "../objects/bullet.h"
#include "../objects/brick.h"
#include "../objects/eagle.h"
#include "../objects/bonus.h"
#include "../objects/mine.h"
#include "../objects/turret.h"
#include "../objects/bot.h"
#include <vector>
#include <string>

/**
 * @brief Classe responsável pelo movimento de todos os tanques e pelas interações entre tanques e entre tanques e outros objetos no mapa.
 */
class Game : public AppState
{
public:
    /**
     * Construtor padrão - permite o jogo para um jogador.
     */
    Game();

    /**
     * Construtor que permite definir o número inicial de jogadores. O número de jogadores pode ser 1 ou 2; qualquer outro valor inicia o jogo para um jogador.
     * O construtor é chamado em @a Menu::nextState.
     * @param players_count - número de jogadores (1 ou 2)
     */
    Game(int players_count);

    /**
     * Construtor que recebe jogadores já existentes.
     * Chamado em @a Score::nextState
     * @param players - container com os jogadores
     * @param previous_level - variável que armazena o número do nível anterior
     */
    Game(std::vector<Player*> players, int previous_level);

    ~Game();

    /**
     * Retorna @a true se o jogador destruiu todos os inimigos ou se a águia foi atingida ou o jogador perdeu todas as vidas, ou seja, ocorreu derrota.
     * @return @a true ou @a false
     */
    bool finished() const;

    /**
     * Exibe o número da rodada no início. Durante o jogo, desenha o nível (paredes, pedras, água, gelo, arbustos),
     * jogadores, inimigos, bônus, águia, status do jogo no painel à direita (inimigos restantes, vidas dos jogadores, número da rodada).
     * Após derrota ou durante pausa, exibe a informação correspondente no centro da tela.
     */
    void draw();

    /**
     * Atualiza o estado de todos os objetos no tabuleiro (tanques, bônus, obstáculos). Também verifica colisões entre tanques, entre tanques e elementos do nível, e entre projéteis e tanques/elementos do mapa.
     * Remove objetos destruídos, adiciona novos tanques inimigos e verifica condições de término da rodada.
     * @param dt - tempo desde a última chamada da função em milissegundos
     */
    void update(Uint32 dt);

    /**
     * Processa eventos de teclado:
     * @li Enter - pausa o jogo
     * @li Esc - retorna ao menu
     * @li N - avança para a próxima rodada, se o jogo não estiver perdido
     * @li B - retorna para a rodada anterior, se o jogo não estiver perdido
     * @li T - mostra os caminhos dos tanques inimigos até seus objetivos
     * @param ev - ponteiro para a união SDL_Event contendo tipo e parâmetros de vários eventos, incluindo eventos de teclado
     */
    void eventProcess(SDL_Event* ev);

    /**
     * Transição para o próximo estado.
     * @return ponteiro para objeto da classe @a Scores se o jogador passou a rodada ou perdeu. Se o jogador pressionar Esc, retorna ponteiro para objeto @a Menu.
     */
    AppState* nextState();

protected:
    /**
     * Construtor para modos derivados (ex.: duelo): inicializa o estado sem carregar
     * nenhuma fase da campanha nem criar jogadores.
     */
    struct NoCampaign {};
    explicit Game(NoCampaign);

    /**
     * Bases presentes no mapa. Tanques não atravessam nenhuma delas e projéteis que as atingem
     * chamam onBaseHit. Na campanha há só a águia do jogador.
     */
    virtual std::vector<Eagle*> bases();

    /**
     * Chamada quando um projétil atinge uma base ainda de pé.
     * Na campanha, destrói a águia e encerra o jogo.
     * @param base - base atingida
     * @param bullet - projétil que atingiu a base
     */
    virtual void onBaseHit(Eagle* base, Bullet* bullet);

    /**
     * Indica se o projétil pode danificar o bloco do mapa na linha/coluna informada.
     * Se não puder, o projétil some sem causar dano. Na campanha, sempre pode.
     */
    virtual bool bulletCanDamage(Bullet* bullet, int row, int column);

    /**
     * Indica se o poder do projétil reforçado (3 estrelas: destrói o bloco inteiro, até pedra)
     * vale no bloco da linha/coluna informada. Se não valer, o projétil age como um comum:
     * desgasta tijolo e para na pedra. Na campanha, sempre vale.
     */
    virtual bool powerAppliesAt(Bullet* bullet, int row, int column);

    /**
     * O projétil destrói o bloco inteiro (em vez de desgastar tijolo ou parar na pedra).
     * Na campanha: o projétil reforçado, onde o poder vale (powerAppliesAt).
     */
    virtual bool breaksBlock(Bullet* bullet, int row, int column);

    /**
     * Teclas comuns a todos os modos extras (ver MODOS_EXTRAS.md):
     * @li Esc / Back - sai para o menu (o modo decide a tela em nextState)
     * @li na tela final (@a results_ready): tiro, Enter, A ou Start também saem
     * @li Enter / Start - pausa, quando @a can_pause
     */
    void extraModeInput(SDL_Event* ev, bool results_ready, bool can_pause);

    /**
     * Espaço do poder guardado no painel de um modo extra: o ícone do poder, ou uma
     * moldura preta vazia (também quando @a player é nulo, por exemplo quem já caiu).
     */
    void drawPowerSlot(const Player* player, const SDL_Rect& slot);

    /**
     * Carrega o mapa do nível a partir de um arquivo.
     * @param path - caminho para o arquivo do mapa
     */
    void loadLevel(std::string path);

    /**
     * Remove inimigos restantes, jogadores, objetos do mapa e bônus.
     */
    void clearLevel();

    /**
     * Carrega um novo nível e cria novos jogadores se ainda não existirem.
     * @see Game::loadLevel(std::string path)
     */
    void nextLevel();

    /**
     * Cria um novo inimigo se o número de inimigos no mapa for menor que 4, assumindo que ainda não foram criados todos os 20 inimigos do mapa.
     * Gera diferentes níveis de armadura para os inimigos dependendo do nível; quanto maior o número da rodada, maior a chance do inimigo ter armadura nível 4.
     * O nível de armadura indica quantos tiros são necessários para destruir o inimigo (de 1 a 4, cada um com cor diferente).
     * O inimigo gerado pode, ao ser destruído, gerar um bônus no mapa.
     */
    virtual void generateEnemy();

    /**
     * Cria um inimigo no ponto indicado, com tipo, blindagem e chance de carregar bônus
     * sorteados pela dificuldade @a level (1 a 35, a escala das fases da campanha).
     */
    Enemy* createEnemy(SDL_Point point, int level);

    /** Quantos inimigos podem estar no mapa ao mesmo tempo (campanha: 4). */
    virtual int enemyLimit() const;

    /** Intervalo mínimo (ms) entre o surgimento de dois inimigos. */
    virtual Uint32 enemySpawnDelay() const;

    /** Painel lateral: inimigos restantes, vidas dos jogadores e número da fase. */
    virtual void drawStatus();

    /** Desenhado por cima de tudo, antes de apresentar o quadro (mensagens dos modos extras). */
    virtual void drawOverlay();

    /** Desenhado no chão, depois do cenário e antes de minas e tanques (marcas dos modos extras). */
    virtual void drawFloor() {}

    /**
     * Aviso de pausa. Campanha: "PAUSE" piscando, como no original. Os modos extras usam a
     * caixa padrão (drawPauseBox).
     */
    virtual void drawPause();

    /** Caixa de pausa dos modos extras: "PAUSE" e como continuar (Enter / Start). */
    void drawPauseBox();

    /**
     * Gera um bônus aleatório no mapa e o posiciona em local que não colida com a águia.
     */
    void generateBonus();

    /** Tipo do próximo bônus (campanha: um dos 8 originais, com a mesma chance). */
    virtual SpriteType randomBonusType();

    /**
     * Onde o próximo bônus surge (canto do sprite de 32x32, em pixels). @return false para o
     * sorteio da campanha: qualquer lugar fora da águia, como no original (até sobre paredes)
     */
    virtual bool bonusSpot(SDL_Point* spot);

    /**
     * Verifica se o tanque pode se mover livremente para frente; caso contrário, o tanque é parado. Não permite sair do tabuleiro.
     * Se o tanque entrar no gelo, escorrega. Se possuir o bônus "Barco", pode atravessar água. Tanques não podem passar pela águia.
     * @param tank - tanque a ser verificado
     * @param dt - última alteração de tempo; com pequenas mudanças, é possível prever a próxima posição do tanque e reagir adequadamente.
     */
    void checkCollisionTankWithLevel(Tank* tank, Uint32 dt);

    /**
     * Verifica se há colisão entre dois tanques; se sim, ambos são parados.
     * @param tank1
     * @param tank2
     * @param dt
     */
    void checkCollisionTwoTanks(Tank* tank1, Tank* tank2, Uint32 dt);

    /**
     * Verifica se uma área está livre para um tanque: dentro do mapa e sem paredes, água (exceto com barco),
     * águia ou outros tanques. Gelo e arbustos não bloqueiam.
     * @param area - área a ser verificada
     * @param tank - tanque que ocuparia a área (ignorado na verificação contra tanques)
     * @param dt - última alteração de tempo, usada para prever a posição dos outros tanques
     * @return true se a área estiver livre
     */
    bool isAreaFreeForTank(SDL_Rect area, Tank* tank, Uint32 dt);

    /**
     * Assistência de curva: se o jogador foi parado pela quina de um obstáculo, mas estaria livre
     * se estivesse alinhado à grade (a até AppConfig::tank_corner_slide_max pixels de distância),
     * desliza o tanque de lado em direção ao alinhamento em vez de deixá-lo travado.
     * @param tank - tanque a ser verificado (jogador ou bot do duelo)
     * @param dt - última alteração de tempo
     */
    void tryCornerSlide(Tank* tank, Uint32 dt);

    /**
     * Verifica se o projétil colide com algum elemento do mapa (água e gelo são ignorados). Se sim, projétil e objeto são destruídos.
     * Se atingir a águia, ocorre derrota.
     * @param bullet - projétil
     */
    void checkCollisionBulletWithLevel(Bullet* bullet);

    /**
     * Verifica colisão do projétil com arbustos no mapa. Arbustos e projétil são destruídos se o projétil tiver dano aumentado.
     * @param bullet - projétil
     * @see Bullet::increased_damage
     */
    void checkCollisionBulletWithBush(Bullet* bullet);

    /**
     * Verifica se o jogador acertou o inimigo. Se sim, o jogador ganha pontos e o inimigo perde um nível de armadura.
     * @param player - jogador
     * @param enemy - inimigo
     */
    void checkCollisionPlayerBulletsWithEnemy(Tank* shooter, Enemy* enemy);

    /**
     * Verifica se o inimigo acertou o jogador com um projétil. Se sim, o jogador perde uma vida, a menos que tenha escudo.
     * @param enemy - inimigo
     * @param player - jogador
     */
    void checkCollisionEnemyBulletsWithPlayer(Enemy* enemy, Player* player);

    /**
     * Se dois projéteis colidirem, ambos são destruídos.
     * @param bullet1
     * @param bullet2
     */
    void checkCollisionTwoBullets(Bullet* bullet1, Bullet* bullet2);

    /**
     * Verifica se o jogador pegou um bônus. Se sim, ocorre a reação apropriada:
     * @li Granada - todos os inimigos visíveis são destruídos
     * @li Capacete - jogador recebe escudo temporário contra projéteis
     * @li Relógio - inimigos visíveis são paralisados temporariamente
     * @li Pá - cria temporariamente uma parede de pedra ao redor da águia
     * @li Tanque - aumenta em 1 o número de vidas do jogador
     * @li Estrela - melhora o tanque do jogador (aumenta velocidade, número de projéteis)
     * @li Arma - melhora o jogador ao máximo
     * @li Barco - permite atravessar água
     * O jogador recebe pontos extras ao pegar um bônus.
     * @param player
     * @param bonus
     */
    virtual void checkCollisionPlayerWithBonus(Player* player, Bonus* bonus);

    // ======================== Poderes dos modos extras ========================
    // Comuns ao duelo e à sobrevivência (a campanha nunca cria minas nem torretas)

    /**
     * A área (em tiles) está livre para um objeto novo: dentro do mapa, sem bloco do cenário,
     * sem arbusto (nada de parede escondida no mato), sem base e sem tanque, e fora dos
     * blocos reservados pelo modo (reservedTile).
     */
    bool areaFree(int row, int column, int rows, int columns);

    /** Como areaFree, para uma área em pixels fora da grade (cada tile que ela toca conta). */
    bool rectFree(SDL_Rect area);

    /** Bloco reservado pelo modo (pontos de nascimento...): nada é colocado em cima. */
    virtual bool reservedTile(int row, int column);

    /**
     * Célula (canto de uma área 2x2, em tiles) à frente do tanque, na direção dele: a primeira
     * fileira de blocos que não toca nele (o vão é sempre menor que um bloco).
     */
    void frontCell(Tank* tank, int* row, int* column) const;

    /**
     * Onde vai um poder colocado à frente do tanque (2x2 tiles), regra W9 do MODOS_EXTRAS.md:
     * logo à frente, nunca com um bloco vazio inteiro no meio. Fora da grade (@a on_grid
     * false, a torreta): encostada no tanque e alinhada com ele; rente a uma parede, encostada
     * com o lado na grade; por último, a área da grade (frontCell). Na grade (a barricada,
     * que vira tijolos): só a área de frontCell.
     * @return false se nenhuma das áreas está livre (rectFree)
     */
    bool frontArea(Tank* tank, bool on_grid, SDL_Rect* area);

    /** Barricada: 4 tijolos (2x2) logo à frente do tanque. @return false se não há espaço */
    bool placeBarricade(Tank* tank);

    /**
     * Regenera o terreno do mapa @a grid (as linhas do arquivo, ver DuelLayout::readMap): o
     * tijolo destruído ou rachado volta inteiro, e a pedra e o arbusto destruídos voltam.
     * Só devolve: não tira o que os jogadores puseram (barricadas) e não põe nada em cima de
     * tanque, torreta, mina ou bônus (ninguém fica preso dentro da parede). Os tiles de
     * @a skip ({coluna, linha}) ficam como estão: o modo cuida deles (a águia, a muralha).
     */
    void restoreTerrain(const std::vector<std::string>& grid, const std::vector<SDL_Point>& skip);

    /**
     * Torreta logo à frente do jogador, da cor e da equipe dele. Fica até ser destruída; cada
     * jogador tem até AppConfig::power_turret_max_per_player (a mais antiga dele sai).
     * @return false se não há espaço
     */
    bool placeTurret(Player* player);

    /** Mina embaixo do jogador, da cor e da equipe dele. Fica até explodir. */
    void placeMine(Player* player);

    /**
     * Retorno: leva o jogador ao primeiro dos pontos que estiver livre (sem tanque em cima).
     * @return false se todos estão ocupados
     */
    bool recall(Player* player, const std::vector<SDL_Point>& points);

    /**
     * O tiro de @a shooter na direção @a d acerta um inimigo (m_enemies): inimigo alinhado, a
     * até AppConfig::power_turret_range tiles, sem pedra no caminho e sem passar pela base nem
     * pelos blocos em volta dela (a muralha: o tiro destruiria a própria base). Usado pela
     * torreta e pelos tanques do computador do lado dos jogadores.
     */
    bool clearShot(Tank* shooter, Direction d);

    /**
     * IA simples de um tanque do lado dos jogadores contra os inimigos (o aliado da
     * sobrevivência, os jogadores da demonstração do menu): inimigo na mira (clearShot), vira
     * e atira parado; senão, anda atrás do inimigo mais perto, como os inimigos da campanha
     * atrás do alvo, trocando de direção de tempos em tempos e quando bate em algo.
     */
    void hunt(Tank* tank, TankCommand& command, Uint32 dt);

    /** Destrói o inimigo de vez (mina, granada), dando o bônus que ele carregava e os pontos. */
    void killEnemy(Enemy* enemy, Player* by);

    /**
     * Minas, torretas e aliados (m_allies) do lado dos jogadores contra os inimigos da
     * campanha (m_enemies): usado pela sobrevivência dentro de Game::update. O duelo trata
     * os dele por equipe.
     */
    void updateFriendlyPowers(Uint32 dt);

    // --- Variáveis de estado do jogo ---

    /**
     * Número de colunas da grade do mapa.
     */
    int m_level_columns_count;

    /**
     * Número de linhas da grade do mapa.
     */
    int m_level_rows_count;

    /**
     * Obstáculos no mapa (matriz de ponteiros para objetos).
     */
    std::vector< std::vector <Object*> > m_level;

    /**
     * Arbustos no mapa.
     */
    std::vector<Object*> m_bushes;

    /**
     * Vetor de inimigos ativos.
     */
    std::vector<Enemy*> m_enemies;

    /**
     * Vetor de jogadores ativos.
     */
    std::vector<Player*> m_players;

    /**
     * Vetor de jogadores mortos.
     */
    std::vector<Player*> m_killed_players;

    /**
     * Vetor de bônus presentes no mapa.
     */
    std::vector<Bonus*> m_bonuses;

    /** Minas e torretas dos modos extras. */
    std::vector<Mine*> m_mines;
    std::vector<Turret*> m_turrets;

    /**
     * Tanques aliados do computador do lado dos jogadores (a tropa de reforço da
     * sobrevivência). Quem decide o que fazem (Bot::command) é o modo; o Game cuida das
     * colisões em updateFriendlyPowers, como nas torretas. O duelo guarda os bots dele
     * entre os inimigos, por equipe, e não usa esta lista.
     */
    std::vector<Bot*> m_allies;

    /**
     * Ponteiro para o objeto águia.
     */
    Eagle* m_eagle;

    /**
     * Número atual do nível.
     */
    int m_current_level;

    /**
     * Número de jogadores no modo selecionado (1 ou 2).
     */
    int m_player_count;

    /**
     * Número de inimigos restantes a serem destruídos neste nível.
     */
    int m_enemy_to_kill;

    /**
     * Indica se a tela inicial do nível está sendo exibida.
     */
    bool m_level_start_screen;

    /**
     * Indica se a águia está protegida por uma parede de pedra.
     */
    bool m_protect_eagle;

    /**
     * Tempo (em ms) que a tela inicial do nível está sendo exibida.
     */
    Uint32 m_level_start_time;

    /**
     * Tempo desde a última criação de inimigo.
     */
    Uint32 m_enemy_redy_time;

    /**
     * Tempo desde que o nível foi vencido.
     */
    Uint32 m_level_end_time;

    /**
     * Tempo que a águia está protegida pela parede de pedra.
     */
    Uint32 m_protect_eagle_time;

    /**
     * Indica se o jogo está em estado de derrota.
     */
    bool m_game_over;

    /**
     * Posição do texto "GAME OVER" se @a m_game_over for true.
     */
    double m_game_over_position;

    /**
     * Tempo (ms) que o "GAME OVER" já ficou parado no centro do mapa.
     */
    Uint32 m_game_over_hold_time;

    /**
     * Indica se o estado atual do jogo deve ser finalizado e transitar para a tela de resultados ou menu.
     */
    bool m_finished;
    /**
     * Modos extras: o jogador saiu pela pausa (Esc / Select). Separado de m_finished, que a
     * sobrevivência usa como "a onda acabou": antes, o Select avançava a onda.
     */
    bool m_quit = false;

    /**
     * Partida de demonstração (fundo do menu, ver Demo): não desenha as mensagens nem as
     * caixas do modo (pausa, avisos, loja), que brigariam com as opções do menu por cima.
     */
    bool m_demo = false;

    /**
     * Indica se o jogo está pausado.
     */
    bool m_pause;

    /**
     * Índice da posição do novo inimigo criado. Alterado a cada criação de inimigo.
     */
    int m_enemy_respown_position;
};

#endif // GAME_H
