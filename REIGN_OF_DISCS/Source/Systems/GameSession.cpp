#include "Systems/GameSession.h"
#include "Systems/AbilityManager.h"
#include "Systems/FacilityManager.h"

void GameSession::Start(const GameConfig& config)
{
    GameRules::Setup(state_, config);
    events_.clear();
    StartTurn();
}

bool GameSession::TryPlace(Pos pos)
{
    if (state_.over)
    {
        return false;
    }
    if (!Board::IsInside(pos))
    {
        Reject("盤面の外には置けません");
        return false;
    }
    FlipList flips;
    const CellOwner actor = state_.turn.Current();
    if (!GameRules::ApplyPlacement(state_, pos, &flips))
    {
        Reject("そこには置けません");
        return false;
    }

    GameEvent placed;
    placed.type = GameEventType::Placed;
    placed.actor = actor;
    placed.pos = pos;
    placed.flips.assign(flips.begin(), flips.end());
    placed.message = state_.Side(actor).name + ": 敵の石を" + std::to_string(flips.count) + "個反転しました";
    Emit(std::move(placed));

    FinishTurn(false);
    return true;
}

bool GameSession::TryPass()
{
    if (state_.over)
    {
        return false;
    }
    if (!GameRules::CanPass(state_))
    {
        Reject("置ける場所がある間はパスできません");
        return false;
    }
    GameEvent passed;
    passed.type = GameEventType::Passed;
    passed.actor = state_.turn.Current();
    passed.message = state_.Current().name + "はパスしました";
    Emit(std::move(passed));
    FinishTurn(true);
    return true;
}

bool GameSession::TryUseAbility(AbilityType type, Pos target)
{
    if (state_.over)
    {
        return false;
    }
    const CellOwner actor = state_.turn.Current();
    AbilityResult result;
    const std::string reason = AbilityManager::Use(state_, type, target, &result);
    if (!reason.empty())
    {
        Reject(reason);
        return false;
    }

    const AbilityInfo& info = AbilityManager::Info(type);
    GameEvent used;
    used.type = GameEventType::AbilityUsed;
    used.actor = actor;
    used.pos = info.needsTarget ? target : Pos{ -1, -1 };
    used.flips = result.changed;
    used.gained = result.gained;
    used.ability = type;
    used.message = state_.Side(actor).name + ": 「" + info.name + "」を使用しました";
    if (info.needsTarget)
    {
        used.message += "（" + std::to_string(target.x) + ", " + std::to_string(target.y) + "）";
    }
    Emit(std::move(used));

    if (state_.board.IsFull())
    {
        FinishTurn(false); // Tactical Placement filled the board: end now and judge
    }
    return true;
}

bool GameSession::TryBuild(FacilityType type, Pos pos)
{
    if (state_.over)
    {
        return false;
    }
    const CellOwner actor = state_.turn.Current();
    const std::string reason = FacilityManager::Build(state_, pos, type);
    if (!reason.empty())
    {
        Reject(reason);
        return false;
    }
    GameEvent built;
    built.type = GameEventType::Built;
    built.actor = actor;
    built.pos = pos;
    built.facility = type;
    built.message = state_.Side(actor).name + ": " + FacilityManager::Info(type).name + "を建設しました（" +
        std::to_string(pos.x) + ", " + std::to_string(pos.y) + "）";
    Emit(std::move(built));
    return true;
}

std::vector<GameEvent> GameSession::TakeEvents()
{
    std::vector<GameEvent> out;
    out.swap(events_);
    return out;
}

void GameSession::StartTurn()
{
    // Loops only through automatic passes; at most two in a row end the game.
    while (!state_.over)
    {
        const CellOwner side = state_.turn.Current();

        GameEvent started;
        started.type = GameEventType::TurnStarted;
        started.actor = side;
        started.message = state_.Side(side).name + "のターンです";
        Emit(std::move(started));

        const Resources gained = GameRules::BeginTurn(state_);
        if (!gained.IsZero())
        {
            GameEvent income;
            income.type = GameEventType::ResourceGained;
            income.actor = side;
            income.gained = gained;
            income.message = state_.Side(side).name + ": 資源獲得";
            if (gained.mana > 0) income.message += " マナ+" + std::to_string(gained.mana);
            if (gained.ore > 0) income.message += " 鉱石+" + std::to_string(gained.ore);
            if (gained.influence > 0) income.message += " 影響力+" + std::to_string(gained.influence);
            Emit(std::move(income));
        }

        if (!GameRules::MustAutoPass(state_))
        {
            return; // waits for the side's input
        }

        GameEvent passed;
        passed.type = GameEventType::Passed;
        passed.actor = side;
        passed.message = state_.Side(side).name + "は置ける場所がないためパスしました";
        Emit(std::move(passed));
        EndTurnAndReport(true);
    }

    GameEvent over;
    over.type = GameEventType::GameOver;
    over.message = "ゲーム終了";
    Emit(std::move(over));
}

void GameSession::FinishTurn(bool passed)
{
    EndTurnAndReport(passed);
    StartTurn(); // emits GameOver if the game ended
}

void GameSession::EndTurnAndReport(bool passed)
{
    const CellOwner actor = state_.turn.Current();
    const GameRules::TurnEndInfo info = GameRules::EndTurn(state_, passed);
    if (info.fortsRemoved > 0)
    {
        GameEvent gone;
        gone.type = GameEventType::FortExpired;
        gone.actor = actor;
        gone.message = "砦の守りが終わり、砦が" + std::to_string(info.fortsRemoved) + "つ消えました";
        Emit(std::move(gone));
    }
    if (info.extraTurn)
    {
        GameEvent extra;
        extra.type = GameEventType::ExtraTurn;
        extra.actor = actor;
        extra.message = state_.Side(actor).name + "が砦を奪った！ もう1ターン行います";
        Emit(std::move(extra));
    }
}

void GameSession::Emit(GameEvent event)
{
    events_.push_back(std::move(event));
}

void GameSession::Reject(const std::string& message)
{
    GameEvent rejected;
    rejected.type = GameEventType::Rejected;
    rejected.actor = state_.turn.Current();
    rejected.message = message;
    Emit(std::move(rejected));
}
