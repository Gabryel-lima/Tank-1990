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

private:
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
    void generateEnemy();

    /**
     * Gera um bônus aleatório no mapa e o posiciona em local que não colida com a águia.
     */
    void generateBonus();

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
    void checkCollisionPlayerBulletsWithEnemy(Player* player, Enemy* enemy);

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
    void checkCollisionPlayerWithBonus(Player* player, Bonus* bonus);

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
     * Indica se o estado atual do jogo deve ser finalizado e transitar para a tela de resultados ou menu.
     */
    bool m_finished;

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
