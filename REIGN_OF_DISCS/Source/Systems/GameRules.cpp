#include "Systems/GameRules.h"
#include "Systems/AbilityManager.h"
#include "Systems/FacilityManager.h"
#include "Systems/MapGenerator.h"
#include "Systems/ResourceManager.h"
#include "Systems/VictoryManager.h"

namespace GameRules
{
    std::string DefaultName(CellOwner owner, const GameConfig& config)
    {
        if (IsAiOpponent(config.opponent))
        {
            if (owner == CellOwner::Player)
            {
                return "プレイヤー";
            }
            switch (config.opponent)
            {
            case OpponentType::AiEasy:   return "敵AI（EASY）";
            case OpponentType::AiNormal: return "敵AI（NORMAL）";
            default:                     return "敵AI（HARD）";
            }
        }
        return owner == CellOwner::Player ? "プレイヤー1（黒）" : "プレイヤー2（白）";
    }

    void Setup(GameState& state, const GameConfig& config)
    {
        state = GameState{};
        state.config = config;
        state.board.Initialize();
        const bool strategy = state.IsStrategy();
        if (strategy)
        {
            state.board.ApplyStrategyLayout(MapGenerator::Generate(config.layout, config.mapSeed));
        }
        state.players[0] = MakePlayer(CellOwner::Player, DefaultName(CellOwner::Player, config), strategy);
        state.players[1] = MakePlayer(CellOwner::Enemy, DefaultName(CellOwner::Enemy, config), strategy);
        state.turn.Reset();
    }

    FlipRule FlipRuleFor(const GameState& state)
    {
        FlipRule rule;
        rule.respectProtection = state.IsStrategy();
        return rule;
    }

    FlipList FindFlips(const GameState& state, Pos pos, CellOwner owner)
    {
        return MoveValidator::FindFlips(state.board, pos, owner, FlipRuleFor(state));
    }

    bool HasLegalMove(const GameState& state, CellOwner owner)
    {
        return MoveValidator::HasLegalMove(state.board, owner, FlipRuleFor(state));
    }

    std::vector<Pos> LegalMoves(const GameState& state)
    {
        return MoveValidator::GetLegalMoves(state.board, state.turn.Current(), FlipRuleFor(state));
    }

    bool ApplyPlacement(GameState& state, Pos pos, FlipList* outFlips)
    {
        if (state.over)
        {
            return false;
        }
        const CellOwner owner = state.turn.Current();
        const FlipList flips = FindFlips(state, pos, owner);
        if (flips.Empty())
        {
            return false; // nothing changes on an illegal move
        }
        state.board.SetOwner(pos, owner);
        for (const Pos& p : flips)
        {
            if (state.board.GetCell(p).facility == FacilityType::Fort)
            {
                state.actions.capturedFort = true; // enemy Fort taken by a normal flip
            }
            state.board.SetOwner(p, owner);
        }
        if (state.IsStrategy())
        {
            FacilityManager::RecomputeDefense(state.board); // new stones may enter / leave Fort range
        }
        if (outFlips != nullptr)
        {
            *outFlips = flips;
        }
        return true;
    }

    Resources BeginTurn(GameState& state)
    {
        if (!state.IsStrategy() || state.over)
        {
            return Resources{};
        }
        const CellOwner side = state.turn.Current();
        return ResourceManager::AddCapped(state.Side(side), ResourceManager::TurnIncome(state, side));
    }

    bool CanPass(const GameState& state)
    {
        return !state.over && !HasLegalMove(state, state.turn.Current());
    }

    bool MustAutoPass(const GameState& state)
    {
        if (!CanPass(state))
        {
            return false;
        }
        if (!state.IsStrategy())
        {
            return true;
        }
        // Strategy: stay in the turn while a build or an ability is still possible (F002 R-26).
        return !FacilityManager::CanBuildAnything(state) && !AbilityManager::CanUseAny(state);
    }

    namespace
    {
        // Timed effects are counted in turns of a side, so extra turns never shorten them:
        //  - Magic Reinforce: ends when the stone owner's opponent finishes a turn (F002 R-19).
        //  - Siege Break: lasts until the facility owner has finished 2 turns (F002 R-22).
        //  - Fort: protects until the Fort owner's opponent finishes a turn, then disappears
        //    (user rule 2026-11-10: "1ターンだけ守れる").
        int ExpireEffects(GameState& state, CellOwner ender)
        {
            int fortsRemoved = 0;
            for (int y = 0; y < Board::SIZE; ++y)
            {
                for (int x = 0; x < Board::SIZE; ++x)
                {
                    const Pos p{ x, y };
                    const Cell& cell = state.board.GetCell(p);
                    if (cell.IsEnhanced() && ender == Opponent(cell.owner))
                    {
                        state.board.SetEnhancedUntil(p, 0);
                    }
                    if (cell.IsFacilityDisabled() && ender == cell.owner)
                    {
                        state.board.SetDisabledUntil(p, cell.disabledUntil - 1);
                    }
                    if (cell.facility == FacilityType::Fort && ender == Opponent(cell.owner))
                    {
                        state.board.SetFacility(p, FacilityType::None);
                        state.board.SetDisabledUntil(p, 0);
                        ++fortsRemoved;
                    }
                }
            }
            FacilityManager::RecomputeDefense(state.board);
            return fortsRemoved;
        }
    }

    TurnEndInfo EndTurn(GameState& state, bool passed)
    {
        TurnEndInfo info;
        if (state.IsStrategy())
        {
            info.fortsRemoved = ExpireEffects(state, state.turn.Current());
            info.extraTurn = state.actions.capturedFort;
        }
        state.turn.EndTurn(passed, info.extraTurn);
        state.actions = TurnActions{};
        state.over = IsGameOver(state);
        if (state.over)
        {
            info.extraTurn = false;
        }
        return info;
    }

    bool IsGameOver(const GameState& state)
    {
        return VictoryManager::IsGameOver(state.board, state.turn);
    }
}
