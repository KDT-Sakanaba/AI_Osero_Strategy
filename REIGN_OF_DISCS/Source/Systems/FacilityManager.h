#pragma once
// Facility construction, effects and Fort defence (original 3.4, 3.5, F002 R-10..R-17).

#include <array>
#include <string>
#include <vector>
#include "Systems/GameState.h"

struct FacilityInfo
{
    FacilityType type;
    const char* name;
    Resources cost;
    const char* effect;
};

class FacilityManager
{
public:
    static const std::array<FacilityInfo, Const::FACILITY_TYPE_COUNT>& All();
    static const FacilityInfo& Info(FacilityType type);

    // Reason the side to move may not build anything this turn (empty = may build).
    static std::string CheckTurn(const GameState& state);

    // Reason 'type' cannot be built at 'pos' by the side to move (empty = OK). No state change.
    static std::string CheckBuild(const GameState& state, Pos pos, FacilityType type);

    static std::vector<Pos> BuildableCells(const GameState& state, FacilityType type);
    static bool CanBuildAnything(const GameState& state);

    // Builds if CheckBuild passes. Returns the CheckBuild reason (empty on success).
    static std::string Build(GameState& state, Pos pos, FacilityType type);

    // Recomputes every square's isDefended from Forts (position, owner, disabled).
    static void RecomputeDefense(Board& board);
};
