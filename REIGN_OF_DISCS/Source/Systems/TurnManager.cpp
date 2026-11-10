#include "Systems/TurnManager.h"

void TurnManager::Reset()
{
    current_ = CellOwner::Player; // player (black) moves first (original 7.3)
    turnNumber_ = 1;
    turnCounts_ = {};
    consecutivePasses_ = 0;
}

void TurnManager::EndTurn(bool passed, bool extraTurn)
{
    consecutivePasses_ = (passed && !extraTurn) ? consecutivePasses_ + 1 : 0;
    ++turnCounts_[SideIndex(current_)];
    ++turnNumber_;
    if (!extraTurn)
    {
        current_ = Opponent(current_);
    }
}
