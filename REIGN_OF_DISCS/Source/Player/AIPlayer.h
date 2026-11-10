#pragma once
// Computer opponent (original 13, F003). Logic layer only: decides, never touches the UI.
// The decision is a list of actions for one turn: optional ability / build, then Place or Pass.

#include <cstdint>
#include <vector>
#include "Systems/GameState.h"

struct AiAction
{
    enum class Kind
    {
        UseAbility,
        Build,
        Place,
        Pass
    };

    Kind kind = Kind::Pass;
    AbilityType ability = AbilityType::MagicReinforce;
    FacilityType facility = FacilityType::None;
    Pos pos{ -1, -1 };
};

struct AiSettings
{
    OpponentType level = OpponentType::AiEasy;
    int maxDepth = 4;            // HARD search depth in plies (original 13.3: 4)
    double timeLimitSec = 2.0;   // HARD thinking time limit (original 13.3: 2 s)
    std::uint32_t seed = 1;
};

struct AiStats
{
    int completedDepth = 0;
    long long nodes = 0;
    double elapsedSec = 0.0;
};

class AIPlayer
{
public:
    // Decides the whole turn of the side to move in 'state'.
    static std::vector<AiAction> DecideTurn(const GameState& state, const AiSettings& settings, AiStats* stats = nullptr);

    // Applies one action to a state with the same rules as the game (used by search and tests).
    // Place / Pass also end the turn and start the next one. Returns false if refused.
    static bool Apply(GameState& state, const AiAction& action);
};
