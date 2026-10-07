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
//   compra     poder comprado vai para o espaço de poder: nada vale na hora da compra (a
//              trégua e o escudo não correm no intervalo) e só vale quando faz efeito
//   espaços    cada espaço guarda uma unidade; a unidade colocada no mapa sai do estoque

#include <SDL2/SDL.h>

#include "../src/app_state/survival.h"
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

    // ===== Compra: guardado, vale quando usado =====
    static void purchases()
    {
        std::srand(11);
        Survival s(1, 0);
        run(s, 80); // o tanque termina de surgir
        Player* p = s.m_players.front();
        p->score = 1000000;
        p->power_slots = AppConfig::survival_max_slots;
        clearWave(s);

        for(SpriteType type : {ST_BONUS_TRUCE, ST_BONUS_TEAM_SHIELD, ST_BONUS_REPAIR, ST_BONUS_REVIVE})
        {
            p->held_power = ST_NONE;
            p->power_stock.clear();
            p->clearFlag(TSF_SHIELD);
            int lives = p->lives_count;
            Survival::ShopItem i = item(type);
            if(i.price < 0) continue; // fora da loja nesta configuração
            std::string name = Survival::shopName(type);
            expect(s.buy(p, i), name + ": compra");
            expect(p->held_power == type, name + ": vai para o espaço de poder");
            expect(s.m_truce_time == 0 && !p->testFlag(TSF_SHIELD) && p->lives_count == lives, name + ": nada vale na hora da compra");
        }

        // No intervalo, a trégua e o escudo continuam guardados (sem inimigos, seriam perdidos)
        p->held_power = ST_BONUS_TRUCE;
        expect(!s.usePower(p) && s.m_truce_time == 0, "trégua no intervalo: continua guardada");
        p->held_power = ST_BONUS_TEAM_SHIELD;
        expect(!s.usePower(p), "escudo de equipe no intervalo: continua guardado");
        p->held_power = ST_BONUS_REPAIR;
        expect(!s.usePower(p), "reparo com a muralha inteira: continua guardado");

        // Durante a onda, valem
        s.m_phase = Survival::PHASE_PLAY;
        p->held_power = ST_BONUS_TRUCE;
        expect(s.usePower(p) && s.m_truce_time > 0, "trégua na onda: vale");
        p->held_power = ST_BONUS_TEAM_SHIELD;
        expect(s.usePower(p) && p->testFlag(TSF_SHIELD), "escudo de equipe na onda: vale");
        SDL_Point wall = s.baseWallTiles().front();
        delete s.m_level.at(wall.y).at(wall.x);
        s.m_level.at(wall.y).at(wall.x) = nullptr;
        p->held_power = ST_BONUS_REPAIR;
        expect(s.usePower(p) && s.baseWallIntact(), "reparo com a muralha furada: refaz");
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

    std::printf("\n%s: %d falha(s)\n", g_failures == 0 ? "PASSOU" : "FALHOU", g_failures);
    return g_failures == 0 ? 0 : 1;
}
