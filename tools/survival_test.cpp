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
//   espaços    cada espaço guarda uma unidade; a unidade colocada no mapa sai do estoque;
//              o botão usa a escolhida com L2 / R2
//   loja       a caixa de texto da loja, com a borda, fica inteira dentro do mapa; uma loja
//              por jogador, sem uma cobrir a outra, e só o dono compra nela
//   aliados    jogadores, torreta e reforço atravessam os tanques do mesmo lado; o tiro
//              aliado não detona a mina
//   torreta    torreta e mina ficam até serem destruídas, na sobrevivência e no duelo (o
//              duelo ainda as fazia sumir com o tempo); até 3 torretas por jogador
//   cada um    duelo 1v1v1v1: as quatro bases são a da equipe A girada; os mapas da pasta ffa
//   por si     passam; base destruída tira só a equipe dela, e a rodada acaba na última de pé

#include <SDL2/SDL.h>

#include "../src/app_state/survival.h"
#include "../src/app_state/duel.h"
#include "../src/app_state/survival_layout.h"
#include "../src/app_state/powers.h"
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
                // Um bônus por vez no mapa: o sorteio começa sem nenhum
                for(Bonus* old_bonus : s.m_bonuses) delete old_bonus;
                s.m_bonuses.clear();
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

    // ===== Um bônus por vez; tanque vermelho só solta o bônus enquanto pisca =====
    static void bonusRules()
    {
        std::srand(77);
        Survival s(2, 0);
        run(s, 5);
        for(Bonus* old_bonus : s.m_bonuses) delete old_bonus;
        s.m_bonuses.clear();
        s.generateBonus();
        expect(s.m_bonuses.size() == 1, "um bônus no mapa depois do sorteio");
        SDL_Point first = {static_cast<int>(s.m_bonuses.front()->pos_x), static_cast<int>(s.m_bonuses.front()->pos_y)};
        SpriteType first_type = s.m_bonuses.front()->type;
        // Outro tanque vermelho acertado com um bônus já no mapa: não troca o que está lá
        s.generateBonus();
        expect(s.m_bonuses.size() == 1 && static_cast<int>(s.m_bonuses.front()->pos_x) == first.x && static_cast<int>(s.m_bonuses.front()->pos_y) == first.y
               && s.m_bonuses.front()->type == first_type, "tanque vermelho com bônus no mapa não troca o bônus (nem a posição)");
        for(Bonus* old_bonus : s.m_bonuses) delete old_bonus;
        s.m_bonuses.clear();

        // Vermelho inteiro, depois pisca com cada vez menos vermelho, depois a cor do tipo: o
        // bônus sai só enquanto ele é vermelho (flag até o fim do pisca)
        Enemy* e = new Enemy(0, 0, ST_TANK_A);
        e->setFlag(TSF_LIFE);
        e->setFlag(TSF_FROZEN);   // parado, para a cor ser só do tempo
        e->setFlag(TSF_BONUS);
        const Uint32 red = AppConfig::carrier_red_time, blink = AppConfig::carrier_blink_time;
        const Uint32 step = 10;
        std::vector<bool> is_red;   // por passo: vermelho na tela?
        bool still_carrier_before_end = true, carrier_after_end = false;
        for(Uint32 t = 0; t < red + blink + 200; t += step)
        {
            if(t >= red + blink - step && e->testFlag(TSF_BONUS) == false) carrier_after_end = true;
            if(t < red + blink - step && !e->testFlag(TSF_BONUS)) still_carrier_before_end = false;
            e->update(step);
            is_red.push_back(e->testFlag(TSF_BONUS));
        }
        expect(still_carrier_before_end, "tanque vermelho carrega o bônus até o fim do pisca");
        expect(carrier_after_end, "depois do pisca o tanque deixa de carregar o bônus");
        // Cor: vermelho inteiro no começo; no pisca, o vermelho aparece menos no fim do que no começo
        int first_quarter = 0, last_quarter = 0;
        for(size_t k = 0; k < is_red.size(); k++)
        {
            Uint32 t = static_cast<Uint32>(k) * step;
            if(t >= red && t < red + blink / 4 && is_red[k]) first_quarter++;
            if(t >= red + blink * 3 / 4 && t < red + blink && is_red[k]) last_quarter++;
        }
        expect(first_quarter > 0 && first_quarter > last_quarter, "o pisca do tanque vermelho perde vermelho até sumir");
        delete e;
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
        SDL_Point hole = s.baseWallTiles().front();
        delete s.m_level.at(hole.y).at(hole.x);
        s.m_level.at(hole.y).at(hole.x) = nullptr;
        expect(s.buy(p, item(ST_BONUS_REPAIR)) && s.baseWallIntact() && p->storedPowers() == 0, "reparo com a muralha furada: refaz na compra");

        // Trégua: fora da loja e do sorteio por enquanto (o código continua)
        expect(item(ST_BONUS_TRUCE).price < 0, "trégua fora da loja");
        bool truce_drawn = false;
        for(const Powers::Weight& w : Powers::survivalTable()) if(w.type == ST_BONUS_TRUCE) truce_drawn = true;
        expect(!truce_drawn, "trégua fora do sorteio");

        // Escudo: no intervalo correria sem inimigos; fica armado para a onda
        expect(s.buy(p, item(ST_BONUS_TEAM_SHIELD)) && s.m_shield_armed && p->storedPowers() == 0, "escudo de equipe no intervalo: armado");
        s.startWave(s.m_wave + 1);
        run(s, static_cast<int>(AppConfig::survival_wave_intro_time / 16) + 2);
        expect(s.m_phase == Survival::PHASE_PLAY && p->testFlag(TSF_SHIELD), "o escudo começa com a onda");
        // A pá saiu da sobrevivência: nem na loja nem no sorteio
        expect(item(ST_BONUS_SHOVEL).price < 0, "pá fora da loja");
        int shovels = 0;
        for(int k = 0; k < 2000; k++) if(s.randomBonusType() == ST_BONUS_SHOVEL) shovels++;
        expect(shovels == 0, "a pá não cai no mapa");
        expect(Survival::shopName(ST_BONUS_RAPID) == "RAPID FIRE" && item(ST_BONUS_RAPID).price > 0, "cadência na loja, com nome");
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

        // L2 / R2: o botão usa o espaço escolhido, e os outros ficam na ordem
        p->setUnits({ST_BONUS_MINE, ST_BONUS_TURBO, ST_BONUS_MINE});
        p->power_selected = 1;
        expect(s.useSelected(p) && p->boosted(), "usa o espaço escolhido (o turbo, no meio)");
        expect(p->units() == std::vector<SpriteType>({ST_BONUS_MINE, ST_BONUS_MINE}) && p->power_selected == 1,
               "o turbo sai; as minas ficam, e a escolha fica no último");
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

    // ===== Caixas das lojas: nenhuma cobre outra caixa, outra loja nem a águia =====
    static void shopBoxesApart()
    {
        const int t = AppConfig::tile_rect.w;
        // Caixas grandes (texto longo, como TEAM SHIELD com o preço): o pior caso dos 4 jogadores
        const int width = 150, height = 50;
        for(int map = 0; map < static_cast<int>(AppConfig::survival_maps.size()); map++)
            for(bool shared : {true, false})
            {
                std::srand(300 + map);
                Survival s(4, map, shared);
                run(s, 80);
                clearWave(s);
                std::string name = AppConfig::survival_maps[map].first + (shared ? " (equipe)" : " (cada um)");
                std::vector<SDL_Rect> taken;
                if(s.m_eagle != nullptr) taken.push_back(s.m_eagle->collision_rect);
                for(int i = 0; i < 4; i++)
                    if(s.hasShop(i)) taken.push_back({s.m_shop_pads[i].x, s.m_shop_pads[i].y, 2 * t, 2 * t});
                int apart = 0, boxes = 0;
                for(int i = 0; i < 4; i++)
                {
                    if(!s.hasShop(i)) continue;
                    SDL_Rect box = s.shopBoxFree(s.m_shop_pads[i], width, height, taken);
                    SDL_Rect frame = {box.x - 1, box.y - 1, box.w + 2, box.h + 2};
                    boxes++;
                    bool clear = true;
                    for(const SDL_Rect& other : taken) if(SDL_HasIntersection(&frame, &other)) clear = false;
                    if(!clear) apart++;
                    taken.push_back(frame);
                }
                expect(apart == 0, name + ": as " + std::to_string(boxes) + " caixas não se cobrem (" + std::to_string(apart) + " sobrepostas)");
            }
        // Sem lugar livre em volta: fica a de menos sobreposição, sem sair do mapa
        Survival s(1, 0);
        SDL_Rect box = s.shopBoxFree({10 * t, 20 * t}, 80, 40, {{0, 0, AppConfig::map_rect.w, AppConfig::map_rect.h}});
        expect(box.x >= 1 && box.y >= 1 && box.x + box.w < AppConfig::map_rect.w && box.y + box.h < AppConfig::map_rect.h,
               "sem lugar livre, a caixa continua dentro do mapa");
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
                int placed = 0, overlapping = 0, too_close = 0, wrong_side = 0;
                for(int i = 0; i < 4; i++)
                {
                    if(!s.hasShop(i)) continue;
                    placed++;
                    SDL_Rect a = {s.m_shop_pads[i].x, s.m_shop_pads[i].y, 2 * t, 2 * t};
                    // Um bloco livre entre duas lojas: a área com um bloco a mais em volta não toca a outra
                    SDL_Rect near = {a.x - t, a.y - t, a.w + 2 * t, a.h + 2 * t};
                    if((a.x + t < 11 * t) != (i % 2 == 0)) wrong_side++;
                    for(int j = 0; j < i; j++)
                    {
                        SDL_Rect b = {s.m_shop_pads[j].x, s.m_shop_pads[j].y, 2 * t, 2 * t};
                        if(!s.hasShop(j)) continue;
                        if(SDL_HasIntersection(&a, &b)) overlapping++;
                        if(SDL_HasIntersection(&near, &b)) too_close++;
                    }
                }
                expect(placed == 4 && overlapping == 0, name + ": 4 lojas, uma por jogador, sem uma cobrir a outra");
                expect(too_close == 0, name + ": um bloco livre entre as lojas (" + std::to_string(too_close) + " sem)");
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

    // ===== Jogadores e aliados se atravessam; o tiro aliado não detona a mina =====
    static void playersPassThrough()
    {
        std::srand(18);
        Survival s(2, 0);
        if(s.m_players.size() != 2)
        {
            expect(false, "dois jogadores entram no modo sobrevivência");
            return;
        }
        expect(s.passThrough(s.m_players[0], s.m_players[1]), "jogadores se atravessam na sobrevivência");
    }

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

        // Nem o tiro inimigo a detona; o inimigo que passa por cima, sim, e o ponto é do dono
        run(s, 200);
        if(s.m_enemies.empty()) { expect(false, "um inimigo surge"); return; }
        Enemy* e = s.m_enemies.front();
        e->setFlag(TSF_LIFE);
        // Um tiro do inimigo em cima da mina (sem esperar a recarga dele)
        Bullet* eb = new Bullet(mine->collision_rect.x, mine->collision_rect.y);
        eb->collision_rect = mine->collision_rect;
        e->bullets.push_back(eb);
        mines = s.m_mines.size();
        s.updateFriendlyPowers(16);
        expect(eb != nullptr && s.m_mines.size() == mines, "o tiro inimigo passa pela mina sem detonar");
        unsigned before = p->score;
        e->collision_rect = mine->collision_rect;
        s.updateFriendlyPowers(16);
        expect(s.m_mines.size() == mines - 1 && p->score > before, "inimigo em cima: a mina explode e o ponto é do dono");

        // Torreta e reforço pontuam para o dono
        if(turret != nullptr) expect(s.scorer(turret) == p, "a torreta pontua para o dono");
        expect(s.scorer(s.m_allies.back()) == p, "o reforço pontua para quem chamou");
        expect(s.scorer(e) == nullptr, "inimigo não pontua");
    }

    // ===== Bônus de tanque: chama um reforço de quem pegou =====
    static void tankBonus()
    {
        std::srand(23);
        Survival s(1, 0);
        s.m_phase = Survival::PHASE_PLAY;
        Player* p = readyPlayer(s);
        if(p == nullptr) { expect(false, "um jogador entra no mapa"); return; }
        run(s, 5); // o retângulo de colisão sai no update
        Bonus* bonus = new Bonus(p->pos_x, p->pos_y, ST_BONUS_TANK);
        bonus->collision_rect = p->collision_rect;
        s.checkCollisionPlayerWithBonus(p, bonus);
        expect(bonus->to_erase && s.m_allies.size() == 1 && s.m_allies.back()->owner == p->playerIndex(), "tanque cinza: reforço na cor de quem pegou");
        delete bonus;
    }

    // ===== Cadência: soma com a estrela e com o turbo, sem anular =====
    static void rapidFire()
    {
        std::srand(29);
        Survival s(1, 0);
        s.m_phase = Survival::PHASE_PLAY;
        Player* p = readyPlayer(s);
        if(p == nullptr) { expect(false, "um jogador entra no mapa"); return; }
        p->changeStarCountBy(2); // 2 tiros na tela
        p->held_power = ST_BONUS_RAPID;
        expect(s.usePower(p) && p->rapid() && p->extraBullets() == AppConfig::power_rapid_bullets, "cadência: um tiro a mais na tela");
        p->changeStarCountBy(1); // a estrela recalcula o limite: a cadência continua por cima
        expect(p->rapid() && p->extraBullets() == AppConfig::power_rapid_bullets, "pegar estrela não anula a cadência");
        p->held_power = ST_BONUS_TURBO;
        expect(s.usePower(p) && p->boosted() && p->rapid(), "turbo e cadência valem juntos");
        p->held_power = ST_BONUS_RAPID;
        s.usePower(p);
        for(Uint32 t = 0; t < AppConfig::power_rapid_time + 100; t += 16) p->update(16);
        expect(p->rapid(), "usar outra cadência com ela valendo soma o tempo (não recomeça)");
        for(Uint32 t = 0; t < AppConfig::power_rapid_time; t += 16) p->update(16);
        expect(!p->rapid() && p->extraBullets() == 0, "a cadência acaba");
    }

    // ===== Minas: até 5 por jogador =====
    static void mineLimit()
    {
        std::srand(31);
        Survival s(1, 0);
        s.m_phase = Survival::PHASE_PLAY;
        Player* p = readyPlayer(s);
        if(p == nullptr) { expect(false, "um jogador entra no mapa"); return; }
        for(int k = 0; k < 7; k++) { p->held_power = ST_BONUS_MINE; s.usePower(p); }
        int alive = 0;
        for(Mine* m : s.m_mines) if(!m->to_erase) alive++;
        expect(alive == AppConfig::power_mine_max_per_player, "até " + std::to_string(AppConfig::power_mine_max_per_player) + " minas por jogador (" + std::to_string(alive) + ")");
    }

    // ===== Dificuldade lenta a cada 10 ondas; vida a cada 5 =====
    static void ramp()
    {
        std::srand(37);
        Survival s(1, 0);
        int steps[] = {1, 10, 11, 20, 21, 51, 200};
        int expected[] = {0, 0, 1, 1, 2, 5, 5};
        bool ok = true;
        for(int i = 0; i < 7; i++) { s.m_wave = steps[i]; if(s.rampStep() != expected[i]) ok = false; }
        expect(ok, "degrau da dificuldade: 0 até a onda 10, +1 a cada 10, até 5");
        // Inimigos da onda 21 (degrau 2): 10% mais rápidos que os da campanha
        s.m_wave = 21;
        s.m_phase = Survival::PHASE_PLAY;
        for(Enemy* e : s.m_enemies) delete e;
        s.m_enemies.clear();
        s.generateEnemy();
        bool faster = !s.m_enemies.empty();
        for(Enemy* e : s.m_enemies)
        {
            double base = (e->type == ST_TANK_B ? 1.3 : 1.0) * AppConfig::tank_default_speed;
            if(e->default_speed < base * 1.09) faster = false;
        }
        expect(faster, "onda 21: inimigos 10% mais rápidos");
        expect(AppConfig::survival_life_every_waves == 5, "vida a cada 5 ondas");
    }

    // ===== Reforço pega bônus para o dono =====
    static void allyBonus()
    {
        std::srand(41);
        Survival s(1, 0);
        s.m_phase = Survival::PHASE_PLAY;
        Player* p = readyPlayer(s);
        if(p == nullptr) { expect(false, "um jogador entra no mapa"); return; }
        run(s, 5);
        expect(s.callReinforcement(p), "chama um reforço");
        Bot* ally = s.m_allies.back();
        for(int i = 0; i < 100 && !ally->testFlag(TSF_LIFE); i++) run(s, 1);
        run(s, 2); // o retângulo de colisão sai no update
        Bonus* bonus = new Bonus(ally->pos_x, ally->pos_y, ST_BONUS_STAR);
        bonus->collision_rect = ally->collision_rect;
        s.m_bonuses.push_back(bonus);
        int stars = p->stars();
        s.alliesCollectBonuses();
        expect(bonus->to_erase && p->stars() == stars + 1, "o reforço pega a estrela e ela vai para o dono");
        // Perto: vai buscar
        bonus->to_erase = false;
        bonus->pos_x = ally->pos_x + 5 * AppConfig::tile_rect.w;
        bonus->dest_rect.x = static_cast<int>(bonus->pos_x);
        bonus->dest_rect.y = ally->dest_rect.y;
        bonus->collision_rect.x = bonus->dest_rect.x;
        for(Enemy* e : s.m_enemies) delete e;
        s.m_enemies.clear();
        // A direção muda de tempos em tempos (às vezes ao acaso, como os inimigos): na maior
        // parte do tempo, para o lado do bônus
        int right = 0, total = 2000;
        for(int i = 0; i < total; i++)
        {
            s.steerAllies(16);
            if(ally->command.move && ally->command.direction == D_RIGHT) right++;
        }
        expect(right > total / 2, "bônus perto: o reforço vai até ele (" + std::to_string(100 * right / total) + "% para o lado dele)");
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

    // Passa a abertura da rodada (o duelo só joga depois dela)
    static void startPlaying(Duel& d)
    {
        for(int i = 0; i < 200 && d.m_phase != Duel::PHASE_PLAY; i++) d.update(16);
    }

    // Tiro de um jogador da @a shooter na base da @a target, como o jogo o entrega
    static void shootBase(Duel& d, int shooter, int target)
    {
        Bullet bullet;
        bullet.team = shooter;
        bullet.from_player = true;
        d.onBaseHit(d.baseOf(target), &bullet);
    }

    static void freeForAll()
    {
        std::srand(23);
        DuelLayout::loadMapList();

        // Geometria do girassol: as bases perto dos cantos (o canto superior esquerdo da águia)
        using DuelLayout::Tile;
        {
            DuelLayout::FormatScope two(2);
            Tile a = DuelLayout::baseOrigin(0), b = DuelLayout::baseOrigin(1);
            expect(a.row == 24 && a.column == 12 && b.row == 0 && b.column == 12, "duas equipes: as bases no meio de baixo e de cima");
            expect(DuelLayout::zoneTeam(12, 0, 2) == -1, "com duas equipes, a zona da esquerda não existe");
        }
        DuelLayout::useFormat(4);
        Tile a = DuelLayout::baseOrigin(0), b = DuelLayout::baseOrigin(1), c = DuelLayout::baseOrigin(2), d4 = DuelLayout::baseOrigin(3);
        expect(a.row == 24 && a.column == 19 && b.row == 0 && b.column == 5 && c.row == 19 && c.column == 0 && d4.row == 5 && d4.column == 24,
               "girassol: A embaixo à direita, C à esquerda mais acima, B em cima mais à direita, D à direita mais abaixo");
        bool zones = DuelLayout::zoneTeam(24, 19, 4) == 0 && DuelLayout::zoneTeam(0, 5, 4) == 1 &&
                     DuelLayout::zoneTeam(19, 0, 4) == 2 && DuelLayout::zoneTeam(5, 24, 4) == 3 && DuelLayout::zoneTeam(12, 12, 4) == -1;
        expect(zones, "uma zona de base por equipe, e o meio fica de ninguém");

        // Muralha do girassol: só a frente (virada para o centro) é de tijolo; laterais, cantos e
        // o escudo à frente são de pedra. Com duas equipes continua o contrário (frente de pedra)
        bool walls = true;
        for(int team = 0; team < 4; team++)
            for(const Tile& w : DuelLayout::baseWallTiles(team))
            {
                bool front = false;
                for(const Tile& f : DuelLayout::baseFrontTiles(team)) front = front || (f.row == w.row && f.column == w.column);
                walls = walls && DuelLayout::isBaseStone(team, w.row, w.column) == !front;
            }
        std::vector<Tile> shield = DuelLayout::baseShieldTiles(0);
        walls = walls && shield.size() == 2 && shield[0].row == 23 - 1 - AppConfig::duel_ffa_shield_gap && shield[0].column == 19;
        expect(walls, "girassol: frente de tijolo, laterais de pedra e escudo de pedra à frente");
        {
            DuelLayout::FormatScope two(2);
            bool classic = DuelLayout::baseShieldTiles(0).empty() && DuelLayout::isBaseStone(0, 23, 12) && !DuelLayout::isBaseStone(0, 24, 11);
            expect(classic, "duas equipes: frente de pedra, laterais de tijolo, sem escudo");
        }
        // Só se acerta a águia de frente e de perto: entre o escudo e a muralha, na coluna dela
        std::vector<std::string> open(26, std::string(26, '.'));
        expect(DuelLayout::attackSide(open, 0, 21, 19) == DuelLayout::FRONT && DuelLayout::attackSide(open, 0, 24, 16) == 0 &&
                   DuelLayout::attackSide(open, 0, 10, 19) == 0,
               "girassol: tiro de frente vale; de lado ou de longe, não");

        // Três equipes: a apertada (entre as outras duas) muda; o lugar oposto a ela fica vazio
        bool crowded = true;
        for(int k = 0; k < 3; k++)
        {
            DuelLayout::useFormat(3, k);
            int turns[3] = {DuelLayout::quarterTurns(0), DuelLayout::quarterTurns(1), DuelLayout::quarterTurns(2)};
            bool distinct = turns[0] != turns[1] && turns[1] != turns[2] && turns[0] != turns[2];
            int vacant = (turns[k] + 2) % 4;
            bool others = true;
            for(int t = 0; t < 3; t++) others = others && (t == k || (turns[t] != vacant && (turns[t] + 2) % 4 != turns[k]));
            crowded = crowded && distinct && others;
        }
        expect(crowded, "três equipes: cada uma pode ser a apertada, com o lugar oposto vazio");
        DuelLayout::useFormat(4);
        // Os nascimentos de cada equipe são os da A girados: ficam na borda da própria base
        bool spawns = true;
        for(SDL_Point p : DuelLayout::spawnOrder(2)) spawns = spawns && p.x == 0;
        for(SDL_Point p : DuelLayout::spawnOrder(3)) spawns = spawns && p.x == 24 * AppConfig::tile_rect.w;
        expect(spawns, "nascimentos de C na borda esquerda e de D na direita");
        bool facing = Tank::facingOfTeam(0) == D_UP && Tank::facingOfTeam(1) == D_DOWN && Tank::facingOfTeam(2) == D_RIGHT &&
                      Tank::facingOfTeam(3) == D_LEFT && Tank::facingOfTeam(-1) == D_UP;
        expect(facing, "cada equipe nasce olhando para o meio do mapa (a campanha, para cima)");

        // Os mapas da pasta ffa: simétricos girando 90 graus, com caminho até as bases
        int bad = 0;
        for(auto& map : AppConfig::duel_ffa_maps)
            if(!DuelLayout::validate(DuelLayout::readMap(AppConfig::duel_ffa_levels_path + map.first), DuelLayout::FFA_TEAMS).empty()) bad++;
        expect(!AppConfig::duel_ffa_maps.empty() && bad == 0, "os " + std::to_string(AppConfig::duel_ffa_maps.size()) + " mapas cada um por si passam no validador");

        // Rodada: base destruída tira só a equipe dela; a última de pé vence
        DuelConfig config;
        config.ffa = true;
        config.humans = 4;
        for(int i = 0; i < 4; i++) { config.human_team[i] = i; config.cpu[i] = true; }
        config.map = 0;
        Duel d(config);
        startPlaying(d);
        expect(d.m_phase == Duel::PHASE_PLAY && d.bases().size() == 4, "cada um por si: quatro bases e a rodada começa");
        Tile origin = DuelLayout::baseOrigin(0);
        Object* side = d.m_level.at(24).at(18);
        Object* front = d.m_level.at(23).at(19);
        expect(d.baseOf(0)->pos_x == origin.column * AppConfig::tile_rect.w && side != nullptr && side->type == ST_STONE_WALL &&
                   front != nullptr && front->type == ST_BRICK_WALL,
               "cada um por si: a águia da A sai do meio, com lateral de pedra e frente de tijolo");

        shootBase(d, 1, 2);
        expect(d.m_out[2] && !d.teamAlive(2) && d.m_phase == Duel::PHASE_PLAY, "base da C cai: a C sai, a rodada segue");
        bool c_gone = true;
        for(Player* p : d.m_players) if(p->team == 2 && p->lives_count > 0) c_gone = false;
        expect(c_gone, "os jogadores da C não renascem");
        shootBase(d, 0, 3);
        expect(d.m_phase == Duel::PHASE_PLAY && !d.teamAlive(3), "base da D cai: restam A e B, a rodada segue");

        // O tiro de um jogador na própria base também a derruba (fogo amigo, como no original)
        shootBase(d, 1, 1);
        expect(d.m_phase == Duel::PHASE_ROUND_END && d.m_round_winner == 0 && d.m_wins[0] == 1,
               "base da B cai: a A é a última de pé e vence a rodada");

        // Menos de 3 jogadores não é cada um por si: duas equipes, como sempre
        DuelConfig two;
        two.ffa = true;
        two.humans = 2;
        Duel e(two);
        expect(!e.m_config.ffa && e.m_teams == 2, "com 2 jogadores o duelo volta a ser de duas equipes");

        // Três jogadores: três bases e a zona da quarta não existe
        DuelConfig three;
        three.ffa = true;
        three.humans = 3;
        for(int i = 0; i < 3; i++) three.cpu[i] = true;
        Duel f(three);
        startPlaying(f);
        expect(f.bases().size() == 3 && f.baseOf(3) == nullptr, "três jogadores: três bases");
        // A equipe apertada é sorteada a cada rodada: em algumas rodadas, todas passam por ela
        bool seen[3] = {false, false, false};
        for(int round = 0; round < 30; round++) { f.startRound(); startPlaying(f); seen[f.m_crowded] = true; }
        expect(seen[0] && seen[1] && seen[2], "três jogadores: a equipe apertada muda de rodada em rodada");
        expect(f.bases().size() == 3, "três jogadores: três bases depois de várias rodadas");
        shootBase(f, 0, 1);
        shootBase(f, 0, 2);
        expect(f.m_phase == Duel::PHASE_ROUND_END && f.m_round_winner == 0, "três jogadores: a última base de pé vence");
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
    SurvivalTest::shopBoxesApart();
    SurvivalTest::bonusRules();
    SurvivalTest::ownShops();
    SurvivalTest::playersPassThrough();
    SurvivalTest::friendlyFire();
    SurvivalTest::tankBonus();
    SurvivalTest::rapidFire();
    SurvivalTest::mineLimit();
    SurvivalTest::ramp();
    SurvivalTest::allyBonus();
    SurvivalTest::permanentPowers();
    SurvivalTest::freeForAll();

    std::printf("\n%s: %d falha(s)\n", g_failures == 0 ? "PASSOU" : "FALHOU", g_failures);
    return g_failures == 0 ? 0 : 1;
}
