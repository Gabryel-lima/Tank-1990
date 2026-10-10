// Testes do modo sobrevivência sem janela: regras que já quebraram uma vez e não podem voltar.
//
//   make survival-test        (compila e roda a partir de build/bin, onde estão os mapas)
//
// Cada teste monta o cenário direto no estado do jogo (SurvivalTest é friend da Survival) e
// confere o resultado. Sai com código 1 se algum falhar.
//
//   bônus      o bônus do tanque vermelho só surge onde os tanques andam: nunca dentro de
//              tijolo, pedra ou água, nem na águia, e num lugar aonde se chega andando
//   contagem   o número na tela ("NEXT WAVE IN" e "START IN") é o tempo real até o primeiro
//              inimigo surgir, e chega a 0 exatamente quando a onda começa
//   compra     o colocável comprado vai para o espaço de poder; o resto vale na compra
//              (reviver, reparo), a trégua e o escudo armados para a próxima onda
//   espaços    cada espaço guarda uma unidade; a unidade colocada no mapa sai do estoque
//   loja       a caixa de texto da loja, com a borda, fica inteira dentro do mapa; uma loja
//              por jogador, sem uma cobrir a outra, e só o dono compra nela
//   aliados    torreta e reforço atravessam os tanques do mesmo lado; o tiro aliado não
//              detona a mina
//   torreta    torreta e mina ficam até serem destruídas, na sobrevivência e no duelo (o
//              duelo ainda as fazia sumir com o tempo); até 3 torretas por jogador

#include <SDL2/SDL.h>

#include "../src/app_state/survival.h"
#include "../src/app_state/duel.h"
#include "../src/app_state/survival_layout.h"
#include "../src/app_state/duel_layout.h"
#include "../src/appconfig.h"
#include "../src/engine/engine.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{
    int g_failures = 0;

    void expect(bool ok, const std::string& what)
    {
        if(!ok) g_failures++;
        std::printf("%s  %s\n", ok ? "ok   " : "FALHA", what.c_str());
    }
}

struct SurvivalTest
{
    static void run(Survival& s, int frames)
    {
        for(int i = 0; i < frames; i++) s.update(16);
    }

    // Os jogadores ficam fora do caminho e não morrem (o teste não depende da sorte deles)
    static void protectPlayers(Survival& s)
    {
        for(Player* p : s.m_players) p->setFlag(TSF_SHIELD);
    }

    // Fim da onda: tira os inimigos e abre o intervalo, como o jogo faz
    static void clearWave(Survival& s)
    {
        for(Enemy* e : s.m_enemies) delete e;
        s.m_enemies.clear();
        s.m_enemy_to_kill = 0;
        s.m_phase = Survival::PHASE_PLAY;
        s.startBreak();
    }

    static Survival::ShopItem item(SpriteType type)
    {
        for(const Survival::ShopItem& i : Survival::shopItems())
            if(i.type == type) return i;
        return {ST_NONE, -1};
    }

    // ===== Bônus =====
    static void bonusSpots()
    {
        const int t = AppConfig::tile_rect.w;
        for(int map = 0; map < static_cast<int>(AppConfig::survival_maps.size()); map++)
        {
            std::srand(1000 + map);
            Survival s(2, map);
            if(s.m_level.empty()) { expect(false, "mapa " + AppConfig::survival_maps[map].second + " carrega"); continue; }
            run(s, 5);
            std::vector<SDL_Point> spots = s.bonusSpots();
            int inside_block = 0, on_eagle = 0, unreachable = 0;
            for(int k = 0; k < 300; k++)
            {
                s.generateBonus();
                const Bonus* b = s.m_bonuses.back();
                SDL_Rect r = b->collision_rect;
                // Fora do mapa ou sobre um bloco que pare tanque (gelo não para)
                bool bad = r.x < 0 || r.y < 0 || r.x + r.w > s.m_level_columns_count * t || r.y + r.h > s.m_level_rows_count * t;
                for(int row = r.y / t; !bad && row <= (r.y + r.h - 1) / t; row++)
                    for(int column = r.x / t; !bad && column <= (r.x + r.w - 1) / t; column++)
                    {
                        const Object* o = s.m_level.at(row).at(column);
                        if(o != nullptr && o->type != ST_ICE) bad = true;
                    }
                if(bad) inside_block++;
                if(SDL_HasIntersection(&r, &s.m_eagle->collision_rect)) on_eagle++;
                bool listed = std::any_of(spots.begin(), spots.end(), [&](SDL_Point p) { return p.x == r.x && p.y == r.y; });
                if(!listed) unreachable++;
            }
            std::string name = AppConfig::survival_maps[map].second;
            expect(!spots.empty(), name + ": há lugar para bônus");
            expect(inside_block == 0, name + ": nenhum bônus dentro de bloco (" + std::to_string(inside_block) + " de 300)");
            expect(on_eagle == 0, name + ": nenhum bônus na águia");
            expect(unreachable == 0, name + ": todo bônus num lugar aonde os tanques chegam");
        }
    }

    // ===== Contagem até a próxima onda =====
    static void countdown()
    {
        std::srand(7);
        Survival s(1, 0);
        run(s, 5);
        protectPlayers(s);
        clearWave(s);

        // Quadro a quadro: o número mostrado e o momento em que o primeiro inimigo surge
        std::vector<std::pair<Uint32, int>> shown; // (tempo desde o início do intervalo, número)
        Uint32 now = 0, spawn = 0;
        bool continuous = true;
        int last = s.countdown();
        expect(last * 1000 >= static_cast<int>(AppConfig::survival_break_time) - 999,
               "o intervalo começa mostrando o tempo inteiro (" + std::to_string(last) + " s)");
        while(now < 60000)
        {
            shown.push_back({now, s.countdown()});
            protectPlayers(s);
            s.update(16);
            now += 16;
            if(s.countdown() > last) continuous = false; // a contagem nunca volta para cima
            last = s.countdown();
            if(!s.m_enemies.empty()) { spawn = now; break; }
        }
        expect(spawn > 0, "a onda começa (primeiro inimigo em " + std::to_string(spawn) + " ms)");
        expect(continuous, "a contagem só desce, do intervalo até o aviso da onda");

        // O número na tela é o tempo real até a onda, arredondado para cima, com até 2 quadros
        // de folga (a virada de fase cai no fim de um quadro e o inimigo nasce no seguinte).
        // Antes desta correção, a contagem sumia aos 3 s e a onda vinha de 3 a 4,5 s depois
        int wrong = 0;
        for(auto& f : shown)
        {
            Uint32 left = spawn - f.first;
            bool matches = false;
            for(Uint32 slack = 0; slack <= 32 && slack <= left; slack += 16)
                if(f.second == static_cast<int>((left - slack + 999) / 1000)) matches = true;
            if(!matches)
            {
                if(wrong < 5) std::printf("        em %u ms: mostra %d, faltam %u ms\n", f.first, f.second, left);
                wrong++;
            }
        }
        expect(wrong == 0, "o número mostrado é o tempo real até a onda (" + std::to_string(wrong) + " quadros errados)");
        expect(shown.back().second <= 1, "a contagem chega ao fim quando o inimigo surge");
    }

    // ===== Compra: o colocável fica guardado, o resto vale na hora =====
    static void purchases()
    {
        std::srand(11);
        Survival s(1, 0);
        run(s, 80); // o tanque termina de surgir
        Player* p = s.m_players.front();
        p->score = 1000000;
        p->power_slots = AppConfig::survival_max_slots;
        clearWave(s);

        // Reviver: vale na compra (ninguém caído: uma vida a mais), sem ocupar espaço
        int lives = p->lives_count;
        expect(s.buy(p, item(ST_BONUS_REVIVE)), "reviver: compra");
        expect(p->lives_count == lives + 1 && p->storedPowers() == 0, "reviver: a vida vale na compra, sem ocupar espaço");

        // Reparo: no intervalo a muralha já foi refeita; furada, a compra a refaz na hora
        expect(s.cannotBuy(item(ST_BONUS_REPAIR), p) == "WALL OK", "reparo com a muralha inteira: não vende (WALL OK)");
        SDL_Point wall = s.baseWallTiles().front();
        delete s.m_level.at(wall.y).at(wall.x);
        s.m_level.at(wall.y).at(wall.x) = nullptr;
        expect(s.buy(p, item(ST_BONUS_REPAIR)) && s.baseWallIntact() && p->storedPowers() == 0, "reparo com a muralha furada: refaz na compra");

        // Trégua e escudo: no intervalo correriam sem inimigos; ficam armados para a onda
        expect(s.buy(p, item(ST_BONUS_TRUCE)) && s.m_truce_time == 0 && s.m_truce_armed, "trégua no intervalo: armada para a próxima onda");
        expect(s.cannotBuy(item(ST_BONUS_TRUCE), p) == "READY", "trégua já armada: não compra outra");
        expect(s.buy(p, item(ST_BONUS_TEAM_SHIELD)) && s.m_shield_armed && p->storedPowers() == 0, "escudo de equipe no intervalo: armado");
        s.startWave(s.m_wave + 1);
        run(s, static_cast<int>(AppConfig::survival_wave_intro_time / 16) + 2);
        expect(s.m_phase == Survival::PHASE_PLAY && s.m_truce_time > 0 && p->testFlag(TSF_SHIELD), "trégua e escudo começam com a onda");

        // Colocáveis e os que dependem de onde e quando: guardados
        for(SpriteType type : {ST_BONUS_MINE, ST_BONUS_BARRICADE, ST_BONUS_TURRET, ST_BONUS_RECALL, ST_BONUS_TURBO})
            expect(Survival::storesPower(type), Survival::shopName(type) + ": fica guardado");
    }

    // ===== Espaços de poder =====
    static void slots()
    {
        std::srand(13);
        Survival s(1, 0);
        run(s, 80);
        Player* p = s.m_players.front();
        p->score = 1000000;
        clearWave(s);
        expect(p->power_slots == 1, "começa com 1 espaço");
        expect(s.buy(p, item(ST_BONUS_MINE)), "compra uma mina");
        expect(s.cannotBuy(item(ST_BONUS_MINE), p) == "SLOTS FULL", "com o espaço cheio, não compra outra (SLOTS FULL)");
        expect(s.buy(p, item(ST_NONE)) && s.buy(p, item(ST_NONE)), "compra 2 espaços");
        expect(s.cannotBuy(item(ST_NONE), p) == "MAX SLOTS", "máximo de espaços");
        expect(s.buy(p, item(ST_BONUS_MINE)) && s.buy(p, item(ST_BONUS_MINE)), "3 minas, uma por espaço");
        expect(p->storedPowers() == 3, "3 guardadas");
        expect(s.usePower(p), "coloca uma mina");
        p->consumeHeldPower();
        expect(p->storedPowers() == 2 && s.m_mines.size() == 1, "2 guardadas, 1 no mapa (a do mapa não ocupa espaço)");
    }
    // ===== Caixa da loja =====
    static void shopBox()
    {
        Survival s(1, 0);
        const int t = AppConfig::tile_rect.w, map_w = AppConfig::map_rect.w, map_h = AppConfig::map_rect.h;
        int outside = 0, checked = 0;
        // A loja em qualquer tile do mapa e caixas de vários tamanhos (o texto muda com o item)
        for(int row = 0; row + 2 <= map_h / t; row++)
            for(int column = 0; column + 2 <= map_w / t; column++)
                for(int w = 30; w <= 220; w += 10)
                    for(int h = 20; h <= 120; h += 10)
                    {
                        SDL_Rect box = s.shopBoxRect({column * t, row * t}, w, h);
                        SDL_Rect border = {box.x - 1, box.y - 1, box.w + 2, box.h + 2};
                        checked++;
                        if(border.x < 0 || border.y < 0 || border.x + border.w > map_w || border.y + border.h > map_h)
                        {
                            if(outside < 3) std::printf("        loja em (%d,%d), caixa %dx%d: borda em (%d,%d) %dx%d\n", column, row, w, h, border.x, border.y, border.w, border.h);
                            outside++;
                        }
                    }
        expect(outside == 0, "a borda da caixa da loja fica dentro do mapa (" + std::to_string(outside) + " de " + std::to_string(checked) + " fora)");
        // A caixa continua ao lado da loja, do lado de fora, quando cabe (sem tapar a loja)
        SDL_Rect box = s.shopBoxRect({10 * t, 20 * t}, 80, 40);
        expect(box.x + box.w + 3 == 10 * t, "loja à esquerda do meio: caixa à esquerda dela");
        box = s.shopBoxRect({14 * t, 20 * t}, 80, 40);
        expect(box.x == 16 * t + 3, "loja à direita do meio: caixa à direita dela");
    }

    // ===== Uma loja por jogador =====
    static void ownShops()
    {
        const int t = AppConfig::tile_rect.w;
        for(int map = 0; map < static_cast<int>(AppConfig::survival_maps.size()); map++)
            for(bool shared : {true, false})
            {
                std::srand(300 + map);
                Survival s(4, map, shared);
                run(s, 80);
                clearWave(s);
                std::string name = AppConfig::survival_maps[map].first + (shared ? " (equipe)" : " (cada um)");
                int placed = 0, overlapping = 0, wrong_side = 0;
                for(int i = 0; i < 4; i++)
                {
                    if(!s.hasShop(i)) continue;
                    placed++;
                    SDL_Rect a = {s.m_shop_pads[i].x, s.m_shop_pads[i].y, 2 * t, 2 * t};
                    if((a.x + t < 11 * t) != (i % 2 == 0)) wrong_side++;
                    for(int j = 0; j < i; j++)
                    {
                        SDL_Rect b = {s.m_shop_pads[j].x, s.m_shop_pads[j].y, 2 * t, 2 * t};
                        if(s.hasShop(j) && SDL_HasIntersection(&a, &b)) overlapping++;
                    }
                }
                expect(placed == 4 && overlapping == 0, name + ": 4 lojas, uma por jogador, sem uma cobrir a outra");
                if(wrong_side > 0) std::printf("        %s: %d loja(s) do outro lado (sem lugar no dela)\n", name.c_str(), wrong_side);
                if(map > 0) continue;

                // Só o dono compra na loja dele; o outro, em cima dela, anda e atira normalmente
                Player* p1 = s.m_players[0];
                Player* p2 = s.m_players[1];
                p1->pos_x = s.m_shop_pads[1].x;
                p1->pos_y = s.m_shop_pads[1].y;
                p2->pos_x = s.m_shop_pads[1].x;
                p2->pos_y = s.m_shop_pads[1].y;
                expect(!s.onShop(p1) && s.onShop(p2), name + ": a loja do P2 é só do P2");
                p1->pos_x = s.m_shop_pads[0].x;
                p1->pos_y = s.m_shop_pads[0].y;
                expect(s.onShop(p1), name + ": a loja do P1 é do P1");
            }
    }

    // ===== Aliados se atravessam; o tiro aliado não detona a mina =====
    static void friendlyFire()
    {
        std::srand(19);
        Survival s(1, 0);
        s.m_phase = Survival::PHASE_PLAY;
        Player* p = readyPlayer(s);
        if(p == nullptr) { expect(false, "um jogador entra no mapa"); return; }
        Turret* turret = nullptr;
        for(int d = 0; d < 4 && turret == nullptr; d++)
        {
            p->direction = static_cast<Direction>(d);
            p->held_power = ST_BONUS_TURRET;
            if(s.usePower(p)) turret = s.m_turrets.back();
        }
        expect(turret != nullptr && s.passThrough(p, turret), "jogador e a própria torreta se atravessam");
        expect(s.callReinforcement(p) && s.passThrough(p, s.m_allies.back()), "jogador e o reforço se atravessam");
        if(turret != nullptr) expect(s.passThrough(s.m_allies.back(), turret), "reforço e torreta se atravessam");
        if(!s.m_enemies.empty() && turret != nullptr) expect(!s.passThrough(s.m_enemies.front(), turret), "inimigo bate na torreta");

        // Torreta fora do caminho; mina debaixo do jogador, que atira para baixo dela
        p->held_power = ST_BONUS_MINE;
        s.usePower(p);
        Mine* mine = s.m_mines.back();
        p->clearFlag(TSF_SHIELD);
        Bullet* b = p->fire();
        if(b != nullptr)
        {
            b->pos_x = mine->collision_rect.x;
            b->pos_y = mine->collision_rect.y;
            b->collision_rect.x = mine->collision_rect.x;
            b->collision_rect.y = mine->collision_rect.y;
        }
        size_t mines = s.m_mines.size();
        s.updateFriendlyPowers(16); // a mina detonada sairia daqui
        expect(b != nullptr && s.m_mines.size() == mines, "o tiro do jogador passa pela mina sem detonar");
    }

    // ===== Torreta e mina permanentes, nos dois modos =====
    template<class Mode>
    static Player* readyPlayer(Mode& mode)
    {
        for(int i = 0; i < 200; i++)
        {
            for(Player* p : mode.m_players)
                if(!p->to_erase && p->testFlag(TSF_LIFE)) return p;
            mode.update(16);
        }
        return nullptr;
    }

    template<class Mode>
    static void permanentPowers(Mode& mode, const std::string& name)
    {
        Player* p = readyPlayer(mode);
        if(p == nullptr) { expect(false, name + ": um jogador entra no mapa"); return; }
        p->held_power = ST_BONUS_MINE;
        expect(mode.usePower(p) && !mode.m_mines.empty(), name + ": coloca a mina");
        Mine* mine = mode.m_mines.back();

        // A torreta na direção em que couber
        Turret* turret = nullptr;
        for(int d = 0; d < 4 && turret == nullptr; d++)
        {
            p->direction = static_cast<Direction>(d);
            p->held_power = ST_BONUS_TURRET;
            if(mode.usePower(p)) turret = mode.m_turrets.back();
        }
        expect(turret != nullptr, name + ": coloca a torreta");
        if(turret == nullptr) return;

        // 3 minutos de jogo só para elas (sem inimigos nem tiros por perto): antes, a torreta
        // sumia em 20 s (ou 10 tiros) e a mina em 30 s fora da sobrevivência
        for(Uint32 time = 0; time < 180000; time += 16)
        {
            turret->update(16);
            mine->update(16);
            turret->think([](Direction) { return true; }); // atira o tempo todo: sem munição contada
            for(Bullet* b : turret->bullets) b->destroy();
        }
        expect(!turret->to_erase && turret->testFlag(TSF_LIFE) && !turret->testFlag(TSF_DESTROYED), name + ": a torreta fica 3 min e atirando");
        expect(!mine->to_erase, name + ": a mina fica 3 min");

        // Limite por jogador: a quarta desmonta a mais antiga dele. Cada torreta colocada sai da
        // frente (fora do mapa), para a próxima caber no mesmo lugar
        std::vector<Turret*> placed = {turret};
        for(int k = 0; k < 3; k++)
        {
            // Surgindo, o update não move os retângulos: muda direto
            Turret* last = placed.back();
            last->pos_x = -1000 - 100 * k;
            last->dest_rect.x = static_cast<int>(last->pos_x);
            last->collision_rect.x = last->dest_rect.x + 2;
            p->held_power = ST_BONUS_TURRET;
            if(mode.usePower(p)) placed.push_back(mode.m_turrets.back());
        }
        expect(placed.size() == 4, name + ": coloca mais 3 torretas");
        int alive = 0;
        for(Turret* t : placed) if(!t->to_erase && !t->testFlag(TSF_DESTROYED)) alive++;
        expect(alive == AppConfig::power_turret_max_per_player && placed.front()->testFlag(TSF_DESTROYED),
               name + ": até " + std::to_string(AppConfig::power_turret_max_per_player) + " por jogador, a mais antiga sai (" + std::to_string(alive) + " no mapa)");
    }

    static void permanentPowers()
    {
        std::srand(17);
        Survival s(1, 0);
        s.m_phase = Survival::PHASE_PLAY;
        permanentPowers(s, "sobrevivência");

        std::srand(17);
        DuelLayout::loadMapList();
        DuelConfig config;
        Duel d(config);
        permanentPowers(d, "duelo");
    }
};

// Com os argumentos: no Windows o SDL troca main por SDL_main (extern "C", com argc e argv);
// sem eles, virava outra função e o link falhava
int main(int, char*[])
{
    // Só a configuração de sprites (tamanhos e quadros): nada é desenhado
    Engine::getEngine().initModules();
    SurvivalLayout::loadMapList();

    SurvivalTest::bonusSpots();
    SurvivalTest::countdown();
    SurvivalTest::purchases();
    SurvivalTest::slots();
    SurvivalTest::shopBox();
    SurvivalTest::ownShops();
    SurvivalTest::friendlyFire();
    SurvivalTest::permanentPowers();

    std::printf("\n%s: %d falha(s)\n", g_failures == 0 ? "PASSOU" : "FALHOU", g_failures);
    return g_failures == 0 ? 0 : 1;
}
