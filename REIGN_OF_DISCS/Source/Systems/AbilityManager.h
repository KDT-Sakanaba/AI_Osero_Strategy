#pragma once
// Special abilities (original 4, F002 R-18..R-24).

#include <array>
#include <string>
#include <vector>
#include "Board/MoveValidator.h"
#include "Systems/GameState.h"

struct AbilityInfo
{
    AbilityType type;
    const char* name;
    Resources cost;
    bool needsTarget;
    int gameLimit;       // uses per game, 0 = unlimited
    const char* effect;
};

struct AbilityResult
{
    std::vector<Pos> changed;   // squares whose stone changed (flip / placement)
    Resources gained;           // Resource Mining
};

class AbilityManager
{
public:
    static const std::array<AbilityInfo, Const::ABILITY_COUNT>& All();
    static const AbilityInfo& Info(AbilityType type);

    static int RemainingUses(const Player& player, AbilityType type); // -1 = unlimited

    // Reason the side to move cannot use 'type' now, ignoring the target (empty = usable).
    static std::string CheckUse(const GameState& state, AbilityType type);

    static bool IsValidTarget(const GameState& state, AbilityType type, Pos pos);
    static std::vector<Pos> ValidTargets(const GameState& state, AbilityType type);

    // Any ability usable right now (used for the automatic pass decision, F002 R-26).
    static bool CanUseAny(const GameState& state);

    // Uses the ability. Returns the reason on failure (state unchanged), empty on success.
    static std::string Use(GameState& state, AbilityType type, Pos target, AbilityResult* result = nullptr);
};
