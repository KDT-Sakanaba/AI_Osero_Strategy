#pragma once
// Game rule constants. Must not depend on DxLib.
// Screen layout / colours / timing live in UI/UiConstants.h.

#include <array>
#include "Utils/Types.h"

namespace Const
{
    constexpr int BOARD_SIZE = 8;
    constexpr int CELL_COUNT = BOARD_SIZE * BOARD_SIZE;

    // Search order of the 8 directions (original 12.1).
    constexpr int DIRECTIONS[8][2] =
    {
        {  0, -1 },
        {  0,  1 },
        { -1,  0 },
        {  1,  0 },
        { -1, -1 },
        {  1, -1 },
        { -1,  1 },
        {  1,  1 }
    };

    // Up, down, left, right: the squares a Fort protects (F002 R-14).
    constexpr int ORTHOGONAL[4][2] = { { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } };

    struct InitialStone { Pos pos; CellOwner owner; };
    constexpr std::array<InitialStone, 4> INITIAL_STONES =
    { {
        { { 3, 3 }, CellOwner::Enemy },
        { { 4, 4 }, CellOwner::Enemy },
        { { 4, 3 }, CellOwner::Player },
        { { 3, 4 }, CellOwner::Player },
    } };

    // Fixed point-symmetric special-square layout (F002 R-02).
    struct SpecialCell { Pos pos; CellType type; };
    constexpr std::array<SpecialCell, 14> SPECIAL_CELLS =
    { {
        { { 2, 0 }, CellType::ManaMine },
        { { 7, 2 }, CellType::ManaMine },
        { { 0, 5 }, CellType::ManaMine },
        { { 5, 7 }, CellType::ManaMine },
        { { 5, 0 }, CellType::OreMine },
        { { 0, 2 }, CellType::OreMine },
        { { 7, 5 }, CellType::OreMine },
        { { 2, 7 }, CellType::OreMine },
        { { 5, 2 }, CellType::Altar },
        { { 2, 5 }, CellType::Altar },
        { { 3, 1 }, CellType::NeutralBase },
        { { 6, 3 }, CellType::NeutralBase },
        { { 1, 4 }, CellType::NeutralBase },
        { { 4, 6 }, CellType::NeutralBase },
    } };

    // ---- Resources (original 3.2, 3.3) ----
    constexpr int MANA_INITIAL = 3;
    constexpr int ORE_INITIAL = 2;
    constexpr int INFLUENCE_INITIAL = 0;
    constexpr int MANA_MAX = 15;
    constexpr int ORE_MAX = 15;
    constexpr int INFLUENCE_MAX = 10;

    // ---- Facilities (original 3.4) ----
    constexpr int FACILITY_TYPE_COUNT = 4;

    // ---- Abilities (original 4.2) ----
    constexpr int ABILITY_COUNT = 5;
    constexpr int MINING_ABILITY_LIMIT = 3;    // Resource Mining: 3 uses per game
    constexpr int TACTICAL_PLACEMENT_LIMIT = 1; // Tactical Placement: 1 use per game
    constexpr int MINING_ABILITY_ORE = 2;
    // Siege Break lasts until the facility owner has finished this many turns.
    constexpr int SIEGE_BREAK_OWNER_TURNS = 2;
}
