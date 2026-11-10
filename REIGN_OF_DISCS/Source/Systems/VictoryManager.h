#pragma once
// End-of-game detection and the winner (original 2.5).
// Conditions are separate functions so special victory conditions can be added later (F002 R-27).

#include "Board/Board.h"
#include "Systems/TurnManager.h"

struct GameResult
{
    CellOwner winner = CellOwner::None; // None = draw
    int playerStones = 0;
    int enemyStones = 0;

    bool IsDraw() const { return winner == CellOwner::None; }
};

class VictoryManager
{
public:
    static bool IsBoardFull(const Board& board) { return board.IsFull(); }
    static bool IsBothPassed(const TurnManager& turn) { return turn.IsBothPassed(); }
    static bool IsGameOver(const Board& board, const TurnManager& turn);

    // Stone count comparison.
    static GameResult Judge(const Board& board);
};
