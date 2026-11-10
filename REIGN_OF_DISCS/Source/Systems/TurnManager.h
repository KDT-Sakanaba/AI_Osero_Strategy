#pragma once
// Whose turn it is, turn numbers and consecutive passes (original 2.4, 7).

#include <array>
#include "Utils/Types.h"

class TurnManager
{
public:
    void Reset();

    CellOwner Current() const { return current_; }

    // Global turn number shown as "ターン N": counts every turn including passes, starts at 1 (F001 R-08).
    int TurnNumber() const { return turnNumber_; }

    int TurnCountOf(CellOwner owner) const { return turnCounts_[SideIndex(owner)]; }
    int ConsecutivePasses() const { return consecutivePasses_; }
    bool IsBothPassed() const { return consecutivePasses_ >= 2; }

    // Ends the current turn (by placement or pass) and hands over to the opponent.
    // extraTurn: the same side moves again (Fort capture); this also resets the pass count.
    void EndTurn(bool passed, bool extraTurn = false);

private:
    CellOwner current_ = CellOwner::Player;
    int turnNumber_ = 1;
    std::array<int, 2> turnCounts_{};
    int consecutivePasses_ = 0;
};
