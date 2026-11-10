#include "Player/Player.h"

Player MakePlayer(CellOwner owner, const std::string& name, bool strategy)
{
    Player player;
    player.owner = owner;
    player.name = name;
    if (strategy)
    {
        player.resources = Resources{ Const::MANA_INITIAL, Const::ORE_INITIAL, Const::INFLUENCE_INITIAL };
    }
    return player;
}
