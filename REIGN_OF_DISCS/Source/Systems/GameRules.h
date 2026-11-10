#pragma once
// Turn-level rules shared by the game, the AI and the tests.
// Classic mode never touches strategy data (F002 R-28).

#include <string>
#include <vector>
#include "Board/MoveValidator.h"
#include "Systems/GameState.h"

namespace GameRules
{
    std::string DefaultName(CellOwner owner, const GameConfig& config);

    // New game: board, layout (strategy), players, turn order.
    void Setup(GameState& state, const GameConfig& config);

    FlipRule FlipRuleFor(const GameState& state);
    FlipList FindFlips(const GameState& state, Pos pos, CellOwner owner);
    bool HasLegalMove(const GameState& state, CellOwner owner);
    std::vector<Pos> LegalMoves(const GameState& state);

    // Places a stone for the side to move and flips. Does not end the turn.
    // Returns false and changes nothing if the move is illegal (original 12.4).
    bool ApplyPlacement(GameState& state, Pos pos, FlipList* outFlips = nullptr);

    // Turn start processing for the side to move: resource and facility income (strategy only).
    Resources BeginTurn(GameState& state);

    // Pass button: allowed only while the side to move has no legal placement (F001 R-06, F002 R-26).
    bool CanPass(const GameState& state);

    // No legal placement and nothing else the side could do -> pass automatically.
    bool MustAutoPass(const GameState& state);

    struct TurnEndInfo
    {
        bool extraTurn = false;   // the side that just moved moves again (Fort captured)
        int fortsRemoved = 0;     // Forts that finished their one turn of protection
    };

    // Ends the turn: expiry of timed effects (strategy), hand-over, game-over check.
    TurnEndInfo EndTurn(GameState& state, bool passed);

    bool IsGameOver(const GameState& state);
}
