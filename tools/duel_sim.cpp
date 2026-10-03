// Simulação do modo duelo sem janela: todos os jogadores controlados pelo computador.
//
// Serve para medir o equilíbrio dos bônus e o comportamento da IA em cada mapa:
// quantas rodadas cada lado vence, quanto cada bônus ajuda quem o pega e quanto
// tempo os bots passam presos. Rode a partir de build/bin (onde estão os mapas):
//
//   make duel-sim
//   cd build/bin && ./duel_sim --matches 200 --teams ABAB --map 0
//
// Opções:
//   --matches N   partidas (padrão 100)
//   --teams XYZ   equipe de cada jogador, ex.: AB (1v1), ABAB (2v2), AAB (2v1)
//   --map M       índice do mapa (padrão: todos, um de cada vez)
//   --seed S      semente do sorteio (padrão 1)
//   --heat        imprime o mapa de calor dos bots de reforço
//   ajustes (AppConfig): --helmet MS, --star-armor 0|1, --stone-ratio N,
//                        --neutral CHANCE, --enemy-side CHANCE, --lives N

#include <SDL2/SDL.h>

#include "../src/app_state/duel_layout.h"
#include "../src/app_state/duel.h"
#include "../src/appconfig.h"
#include "../src/engine/engine.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace
{
    const Uint32 STEP = 16;                       // ms por quadro (~60 FPS)
    const Uint32 MATCH_LIMIT = 20 * 60 * 1000;    // partida travada: desiste após 20 min

    const char* bonusName(SpriteType type)
    {
        switch(type)
        {
        case ST_BONUS_GRENADE: return "grenade";
        case ST_BONUS_HELMET:  return "helmet";
        case ST_BONUS_CLOCK:   return "clock";
        case ST_BONUS_SHOVEL:  return "shovel";
        case ST_BONUS_TANK:    return "tank";
        case ST_BONUS_STAR:    return "star";
        case ST_BONUS_GUN:     return "gun";
        case ST_BONUS_BOAT:    return "boat";
        default:               return "?";
        }
    }

    // Vitórias de quem pegou o bônus, separadas pela situação no momento da coleta
    struct PickupTally
    {
        int count[3] = {0, 0, 0}; // [0] atrás, [1] parelho, [2] na frente
        int wins[3] = {0, 0, 0};
    };

    int leadBucket(double lead)
    {
        if(lead <= -0.2) return 0;
        if(lead >= 0.2) return 2;
        return 1;
    }

    std::string percent(int part, int total)
    {
        if(total == 0) return "   -";
        char buf[16];
        std::snprintf(buf, sizeof buf, "%3d%%", (100 * part + total / 2) / total);
        return buf;
    }

    struct MapReport
    {
        int rounds = 0, wins[2] = {0, 0}, draws = 0, by_base = 0, stalled = 0;
        double time = 0;
        double alive[2] = {0, 0}, stuck[2] = {0, 0};
        int unstick[2] = {0, 0};
        std::vector<double> heat;
        int heat_columns = 0;
    };
}

int main(int argc, char* argv[])
{
    int matches = 100, only_map = -1;
    unsigned seed = 1;
    bool heat = false;
    std::string teams = "AB";
    for(int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if(arg == "--matches" && i + 1 < argc) matches = std::atoi(argv[++i]);
        else if(arg == "--teams" && i + 1 < argc) teams = argv[++i];
        else if(arg == "--map" && i + 1 < argc) only_map = std::atoi(argv[++i]);
        else if(arg == "--seed" && i + 1 < argc) seed = static_cast<unsigned>(std::atoi(argv[++i]));
        else if(arg == "--heat") heat = true;
        // Ajustes de equilíbrio, para comparar variações sem recompilar
        else if(arg == "--helmet" && i + 1 < argc) AppConfig::duel_helmet_time = std::atoi(argv[++i]);
        else if(arg == "--star-armor" && i + 1 < argc) AppConfig::duel_star_armor = std::atoi(argv[++i]) != 0;
        else if(arg == "--stone-ratio" && i + 1 < argc) AppConfig::duel_stone_wall_ratio = std::atoi(argv[++i]);
        else if(arg == "--neutral" && i + 1 < argc) AppConfig::duel_neutral_bonus_chance = std::atof(argv[++i]);
        else if(arg == "--enemy-side" && i + 1 < argc) AppConfig::duel_team_bonus_enemy_side_chance = std::atof(argv[++i]);
        else if(arg == "--lives" && i + 1 < argc) AppConfig::duel_tank_lives = std::atoi(argv[++i]);
        else
        {
            std::fprintf(stderr, "uso: %s [--matches N] [--teams ABAB] [--map M] [--seed S] [--heat]\n"
                                 "       [--helmet MS] [--star-armor 0|1] [--stone-ratio N] [--neutral P] [--enemy-side P] [--lives N]\n", argv[0]);
            return 1;
        }
    }

    DuelConfig config;
    config.humans = static_cast<int>(teams.size());
    if(config.humans < 2 || config.humans > 4)
    {
        std::fprintf(stderr, "--teams precisa de 2 a 4 jogadores (ex.: AB, ABAB, AAB)\n");
        return 1;
    }
    for(int i = 0; i < config.humans; i++)
    {
        config.human_team[i] = (teams[i] == 'B' || teams[i] == 'b') ? 1 : 0;
        config.cpu[i] = true;
    }

    // Só a configuração de sprites (tamanhos e quadros): nada é desenhado
    Engine::getEngine().initModules();
    DuelLayout::loadMapList();
    std::srand(seed);

    // (tipo, cinza, lado: -1 meio, 0 próprio, 1 adversário)
    std::map<std::tuple<SpriteType, bool, int>, PickupTally> tally;
    std::vector<MapReport> maps(AppConfig::duel_maps.size());

    for(int map = 0; map < static_cast<int>(AppConfig::duel_maps.size()); map++)
    {
        if(only_map >= 0 && map != only_map) continue;
        config.map = map;
        MapReport& report = maps[map];

        for(int m = 0; m < matches; m++)
        {
            Duel duel(config);
            Uint32 time = 0;
            while(!duel.finished() && time < MATCH_LIMIT)
            {
                duel.update(STEP);
                time += STEP;
                // A partida acabou (alguém venceu 2 rodadas): não espera a tela final
                const DuelStats& s = duel.stats();
                int wins[2] = {0, 0};
                for(int w : s.round_winner) if(w >= 0) wins[w]++;
                if(wins[0] >= AppConfig::duel_rounds_to_win || wins[1] >= AppConfig::duel_rounds_to_win) break;
            }
            if(time >= MATCH_LIMIT) report.stalled++;

            const DuelStats& s = duel.stats();
            for(size_t r = 0; r < s.round_winner.size(); r++)
            {
                report.rounds++;
                if(s.round_winner[r] < 0) report.draws++;
                else report.wins[s.round_winner[r]]++;
                if(s.round_by_base[r]) report.by_base++;
                report.time += s.round_time[r];
            }
            for(const DuelStats::Pickup& p : s.pickups)
            {
                if(p.round - 1 >= static_cast<int>(s.round_winner.size())) continue; // rodada não terminou
                PickupTally& t = tally[std::make_tuple(p.type, p.neutral, p.side)];
                int bucket = leadBucket(p.lead);
                t.count[bucket]++;
                if(s.round_winner[p.round - 1] == p.team) t.wins[bucket]++;
            }
            for(int k = 0; k < 2; k++)
            {
                report.alive[k] += s.ai_alive_time[k];
                report.stuck[k] += s.ai_stuck_time[k];
                report.unstick[k] += s.ai_unstick[k];
            }
            if(!s.bot_heat.empty())
            {
                report.heat.resize(s.bot_heat.size(), 0.0);
                report.heat_columns = s.heat_columns;
                for(size_t i = 0; i < s.bot_heat.size(); i++) report.heat[i] += s.bot_heat[i];
            }
        }
    }

    std::printf("Equipes %s, %d partidas por mapa, semente %u\n\n", teams.c_str(), matches, seed);

    std::printf("%-12s %6s %6s %6s %6s %7s %7s | %-24s | %-24s\n", "mapa", "rodadas", "A", "B", "empate", "base", "tempo",
                "bots: preso  destrava/min", "cpu: preso  destrava/min");
    for(size_t map = 0; map < maps.size(); map++)
    {
        const MapReport& r = maps[map];
        if(r.rounds == 0 && r.stalled == 0) continue;
        char bots[64], cpus[64];
        for(int k = 0; k < 2; k++)
        {
            char* out = (k == 0 ? bots : cpus);
            if(r.alive[k] <= 0) std::snprintf(out, 64, "%s", "-");
            else std::snprintf(out, 64, "%5.1f%%  %6.2f", 100.0 * r.stuck[k] / r.alive[k], r.unstick[k] / (r.alive[k] / 60000.0));
        }
        std::printf("%-12s %6d %6s %6s %6s %7s %6.0fs | %-24s | %-24s%s\n", AppConfig::duel_maps[map].second.c_str(), r.rounds,
                    percent(r.wins[0], r.rounds).c_str(), percent(r.wins[1], r.rounds).c_str(),
                    percent(r.draws, r.rounds).c_str(), percent(r.by_base, r.rounds).c_str(),
                    r.rounds ? r.time / r.rounds / 1000.0 : 0.0, bots, cpus,
                    r.stalled ? (" (" + std::to_string(r.stalled) + " travadas)").c_str() : "");
    }

    std::map<std::pair<SpriteType, bool>, PickupTally> by_color;
    for(auto& entry : tally)
    {
        PickupTally& t = by_color[{std::get<0>(entry.first), std::get<1>(entry.first)}];
        for(int b = 0; b < 3; b++)
        {
            t.count[b] += entry.second.count[b];
            t.wins[b] += entry.second.wins[b];
        }
    }
    std::printf("\nVitória na rodada de quem pegou o bônus (situação em vidas no momento da coleta)\n");
    std::printf("%-8s %-7s | %14s | %14s | %14s | %6s\n", "bonus", "cor", "atrás", "parelho", "na frente", "total");
    for(auto& entry : by_color)
    {
        const PickupTally& t = entry.second;
        int count = t.count[0] + t.count[1] + t.count[2];
        int wins = t.wins[0] + t.wins[1] + t.wins[2];
        std::printf("%-8s %-7s |", bonusName(entry.first.first), entry.first.second ? "cinza" : "equipe");
        for(int b = 0; b < 3; b++) std::printf(" %s de %5d |", percent(t.wins[b], t.count[b]).c_str(), t.count[b]);
        std::printf(" %s\n", percent(wins, count).c_str());
    }

    // Linhas cruas, para somar várias execuções (tools/duel_sim_report.py)
    for(size_t map = 0; map < maps.size(); map++)
    {
        const MapReport& r = maps[map];
        if(r.rounds == 0) continue;
        std::printf("#map %zu %d %d %d %d %d %.0f %.0f %.0f %.0f %.0f %d %d %d\n", map, r.rounds, r.wins[0], r.wins[1], r.draws,
                    r.by_base, r.time, r.alive[0], r.stuck[0], r.alive[1], r.stuck[1], r.unstick[0], r.unstick[1], r.stalled);
    }
    for(auto& entry : tally)
    {
        const PickupTally& t = entry.second;
        std::printf("#pickup %s %s %d %d %d %d %d %d %d\n", bonusName(std::get<0>(entry.first)), std::get<1>(entry.first) ? "cinza" : "equipe",
                    std::get<2>(entry.first),
                    t.count[0], t.wins[0], t.count[1], t.wins[1], t.count[2], t.wins[2]);
    }

    if(heat)
        for(size_t map = 0; map < maps.size(); map++)
        {
            const MapReport& r = maps[map];
            if(r.heat.empty()) continue;
            double total = 0;
            for(double v : r.heat) total += v;
            std::printf("\nOnde os bots passam o tempo: %s (. <0,2%%  : <0,5%%  o <1%%  O <2%%  @ >=2%%)\n",
                        AppConfig::duel_maps[map].second.c_str());
            for(size_t i = 0; i < r.heat.size(); i++)
            {
                double share = r.heat[i] / total;
                char c = share < 0.002 ? '.' : share < 0.005 ? ':' : share < 0.01 ? 'o' : share < 0.02 ? 'O' : '@';
                std::putchar(c);
                if(static_cast<int>(i % r.heat_columns) == r.heat_columns - 1) std::putchar('\n');
            }
        }

    Engine::getEngine().destroyModules();
    return 0;
}
