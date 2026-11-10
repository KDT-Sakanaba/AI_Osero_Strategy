#pragma once
// Resource amounts, caps, costs and turn income (original 3.1-3.3, F002 R-06..R-09).

#include <string>
#include "Systems/GameState.h"

class ResourceManager
{
public:
    static Resources Max() { return Resources{ Const::MANA_MAX, Const::ORE_MAX, Const::INFLUENCE_MAX }; }

    // Adds with caps; returns what was actually added (overflow is lost, original 3.2).
    static Resources AddCapped(Player& player, const Resources& delta);

    static bool CanAfford(const Player& player, const Resources& cost) { return player.resources.Covers(cost); }

    // Pays the cost. Returns false and changes nothing when short.
    static bool Pay(Player& player, const Resources& cost);

    // "マナが不足しています" etc., or empty when affordable.
    static std::string Shortage(const Player& player, const Resources& cost);

    // Income from a single square for its owner (resource square + active facility).
    static Resources CellIncome(const Cell& cell);

    // Turn-start income of 'side': every owned square counted once (original 3.3).
    static Resources TurnIncome(const GameState& state, CellOwner side);
};
