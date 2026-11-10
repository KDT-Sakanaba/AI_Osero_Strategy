#pragma once
// Per-side data (original 11.5, 11.6). Plain value types.

#include <array>
#include <string>
#include "Utils/Constants.h"

struct Resources
{
    int mana = 0;
    int ore = 0;
    int influence = 0;

    bool IsZero() const { return mana == 0 && ore == 0 && influence == 0; }
    bool Covers(const Resources& cost) const
    {
        return mana >= cost.mana && ore >= cost.ore && influence >= cost.influence;
    }
};

struct Player
{
    CellOwner owner = CellOwner::None;
    std::string name;            // UTF-8 display name
    Resources resources;
    int facilityCount = 0;       // facilities built by this side (statistics)
    int abilityUsedCount = 0;    // abilities used in total
    std::array<int, Const::ABILITY_COUNT> abilityUses{}; // per-ability uses this game
};

Player MakePlayer(CellOwner owner, const std::string& name, bool strategy);
