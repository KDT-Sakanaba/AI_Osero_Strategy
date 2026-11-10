#pragma once
// One game in progress: GameState + the events produced for the UI (messages, animations).
// Scenes and the AI controller operate the game only through this class.

#include <string>
#include <vector>
#include "Systems/GameRules.h"
#include "Systems/VictoryManager.h"

enum class GameEventType
{
    TurnStarted,
    Placed,
    Passed,
    ResourceGained,
    Built,
    AbilityUsed,
    FortExpired, // Forts disappeared after their one turn of protection
    ExtraTurn,  // the side took a Fort and moves again
    Rejected,   // an operation was refused; message says why
    GameOver
};

struct GameEvent
{
    GameEventType type = GameEventType::TurnStarted;
    CellOwner actor = CellOwner::None;
    Pos pos{ -1, -1 };
    std::vector<Pos> flips;   // stones flipped by this event, in flip order
    Resources gained;         // ResourceGained / Resource Mining
    AbilityType ability = AbilityType::MagicReinforce; // AbilityUsed
    FacilityType facility = FacilityType::None;        // Built
    std::string message;      // UTF-8 text for the message log
};

class GameSession
{
public:
    void Start(const GameConfig& config);

    const GameState& State() const { return state_; }
    bool IsOver() const { return state_.over; }
    GameResult Result() const { return VictoryManager::Judge(state_.board); }

    // Places a stone for the side to move. Illegal -> false, nothing changes, a Rejected event.
    bool TryPlace(Pos pos);

    // Pass button. Allowed only when there is no legal placement.
    bool TryPass();

    // Strategy mode: special ability (target ignored when not needed) and construction.
    // Refused -> false, nothing changes, a Rejected event with the reason.
    bool TryUseAbility(AbilityType type, Pos target);
    bool TryBuild(FacilityType type, Pos pos);

    // Events since the last call (oldest first).
    std::vector<GameEvent> TakeEvents();

protected:
    void StartTurn();               // income + automatic passes
    void FinishTurn(bool passed);   // hand-over and next turn start
    void EndTurnAndReport(bool passed);
    void Emit(GameEvent event);
    void Reject(const std::string& message);

    GameState state_;
    std::vector<GameEvent> events_;
};
