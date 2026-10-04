#include "powers.h"

#include <cstdlib>

namespace Powers
{

bool storable(SpriteType type)
{
    switch(type)
    {
    case ST_BONUS_MINE:
    case ST_BONUS_BARRICADE:
    case ST_BONUS_TURRET:
    case ST_BONUS_RECALL:
    case ST_BONUS_TURBO:
        return true;
    default:
        return false;
    }
}

bool isExtra(SpriteType type)
{
    return type >= ST_BONUS_MINE && type <= ST_BONUS_TEAM_SHIELD;
}

const char* name(SpriteType type)
{
    switch(type)
    {
    case ST_BONUS_GRENADE: return "grenade";
    case ST_BONUS_HELMET: return "helmet";
    case ST_BONUS_CLOCK: return "clock";
    case ST_BONUS_SHOVEL: return "shovel";
    case ST_BONUS_TANK: return "tank";
    case ST_BONUS_STAR: return "star";
    case ST_BONUS_GUN: return "gun";
    case ST_BONUS_BOAT: return "boat";
    case ST_BONUS_MINE: return "mine";
    case ST_BONUS_BARRICADE: return "barricade";
    case ST_BONUS_TURRET: return "turret";
    case ST_BONUS_RECALL: return "recall";
    case ST_BONUS_TURBO: return "turbo";
    case ST_BONUS_REVIVE: return "revive";
    case ST_BONUS_REPAIR: return "repair";
    case ST_BONUS_TRUCE: return "truce";
    case ST_BONUS_TEAM_SHIELD: return "teamshield";
    default: return "?";
    }
}

const std::vector<Weight>& duelTable()
{
    // Os que decidem a rodada sozinhos (granada, canhão, escudo de equipe) são raros
    static const std::vector<Weight> table = {
        {ST_BONUS_STAR, 20}, {ST_BONUS_HELMET, 14}, {ST_BONUS_SHOVEL, 12}, {ST_BONUS_CLOCK, 10},
        {ST_BONUS_TANK, 10}, {ST_BONUS_BOAT, 8}, {ST_BONUS_GRENADE, 6}, {ST_BONUS_GUN, 6},
        {ST_BONUS_MINE, 10}, {ST_BONUS_BARRICADE, 10}, {ST_BONUS_TURBO, 10}, {ST_BONUS_RECALL, 8},
        {ST_BONUS_REPAIR, 8}, {ST_BONUS_TURRET, 6}, {ST_BONUS_REVIVE, 6}, {ST_BONUS_TEAM_SHIELD, 5},
    };
    return table;
}

const std::vector<Weight>& survivalTable()
{
    // Os originais com o mesmo peso entre si (como na campanha) e os novos um pouco mais
    // raros, para a cara da campanha continuar
    static const std::vector<Weight> table = {
        {ST_BONUS_GRENADE, 10}, {ST_BONUS_HELMET, 10}, {ST_BONUS_CLOCK, 10}, {ST_BONUS_SHOVEL, 10},
        {ST_BONUS_TANK, 10}, {ST_BONUS_STAR, 10}, {ST_BONUS_GUN, 10}, {ST_BONUS_BOAT, 10},
        {ST_BONUS_MINE, 8}, {ST_BONUS_BARRICADE, 8}, {ST_BONUS_TURRET, 7}, {ST_BONUS_RECALL, 6},
        {ST_BONUS_TURBO, 7}, {ST_BONUS_REVIVE, 7}, {ST_BONUS_REPAIR, 8}, {ST_BONUS_TRUCE, 6},
        {ST_BONUS_TEAM_SHIELD, 6},
    };
    return table;
}

SpriteType draw(const std::vector<Weight>& table)
{
    int total = 0;
    for(const Weight& w : table) total += w.weight;
    int roll = rand() % total;
    for(const Weight& w : table)
    {
        if(roll < w.weight) return w.type;
        roll -= w.weight;
    }
    return table.front().type;
}

} // namespace Powers
