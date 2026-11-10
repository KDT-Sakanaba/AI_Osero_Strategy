#pragma once
// Complete state of one game. A plain value type: the AI copies it to search.

#include <array>
#include "Board/Board.h"
#include "Player/Player.h"
#include "Systems/TurnManager.h"

// What the side to move has already done this turn (original 7.2).
struct TurnActions
{
    bool abilityUsed = false;
    bool built = false;
    bool capturedFort = false;   // took an enemy Fort this turn -> one extra turn (user rule 2026-11-10)
};

struct GameState
{
    GameConfig config;
    Board board;
    TurnManager turn;
    std::array<Player, 2> players;
    TurnActions actions;
    bool over = false;

    bool IsStrategy() const { return config.rule == RuleMode::Strategy; }
    Player& Side(CellOwner owner) { return players[SideIndex(owner)]; }
    const Player& Side(CellOwner owner) const { return players[SideIndex(owner)]; }
    Player& Current() { return Side(turn.Current()); }
    const Player& Current() const { return Side(turn.Current()); }
};
