#include "Player/AIPlayer.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <random>
#include "Systems/AbilityManager.h"
#include "Systems/FacilityManager.h"
#include "Systems/GameRules.h"
#include "Systems/ResourceManager.h"

namespace
{
    // ---- Weights (F003 R-07). NORMAL: original 13.2 ----
    namespace W
    {
        constexpr int CORNER = 100;
        constexpr int EDGE = 20;
        constexpr int RESOURCE_CELL = 30;
        constexpr int BASE_CELL = 40;
        constexpr int GIVES_CORNER = -80;
        constexpr int FLIP_PENALTY = 2;        // per flipped stone while many squares are empty
        constexpr int EARLY_EMPTY = 20;
        constexpr int FACILITY_TARGET = 35;
        constexpr int FORT_CAPTURE = 60;       // taking a Fort gives an extra turn (user rule 2026-11-10)
        constexpr int WORTHWHILE = 30;         // minimum square value for targeted abilities
        constexpr int MINING_ORE_BELOW = 12;   // use Resource Mining while ore <= 11
        constexpr int GENERATOR_MANA_BELOW = 10;

        // HARD evaluation
        constexpr int E_CORNER = 25;
        constexpr int E_EDGE = 5;
        constexpr int E_MOBILITY = 4;
        constexpr int E_RESOURCE = 1;
        constexpr int E_FACILITY = 10;
        constexpr int E_BASE = 15;
        constexpr int E_RESOURCE_CELL = 8;
        constexpr int E_FLIP_READY = 6;
        constexpr int E_WIN = 10000;
        constexpr int HARD_ABILITY_CANDIDATES = 2;
    }

    using Clock = std::chrono::steady_clock;

    bool IsCorner(Pos p) { return (p.x == 0 || p.x == 7) && (p.y == 0 || p.y == 7); }
    bool IsEdge(Pos p) { return !IsCorner(p) && (p.x == 0 || p.x == 7 || p.y == 0 || p.y == 7); }
    bool IsResourceCell(CellType t) { return t == CellType::ManaMine || t == CellType::OreMine || t == CellType::Altar; }

    // Positional value of a square (NORMAL, original 13.2).
    int SquareValue(const GameState& s, Pos p)
    {
        int v = IsCorner(p) ? W::CORNER : IsEdge(p) ? W::EDGE : 0;
        if (s.IsStrategy())
        {
            const CellType t = s.board.GetCell(p).type;
            if (IsResourceCell(t)) v += W::RESOURCE_CELL;
            if (t == CellType::NeutralBase) v += W::BASE_CELL;
        }
        return v;
    }

    int CornersAvailable(const GameState& s, CellOwner side)
    {
        int n = 0;
        for (Pos c : { Pos{ 0, 0 }, Pos{ 7, 0 }, Pos{ 0, 7 }, Pos{ 7, 7 } })
        {
            if (!GameRules::FindFlips(s, c, side).Empty()) ++n;
        }
        return n;
    }

    // Squares the opponent could flip with its next placement.
    std::array<bool, Const::CELL_COUNT> ThreatenedSquares(const GameState& s, CellOwner side)
    {
        std::array<bool, Const::CELL_COUNT> threatened{};
        const CellOwner opp = Opponent(side);
        for (int y = 0; y < Board::SIZE; ++y)
        {
            for (int x = 0; x < Board::SIZE; ++x)
            {
                for (const Pos& f : GameRules::FindFlips(s, Pos{ x, y }, opp))
                {
                    threatened[f.y * Board::SIZE + f.x] = true;
                }
            }
        }
        return threatened;
    }

    AiAction PlaceAction(Pos p) { AiAction a; a.kind = AiAction::Kind::Place; a.pos = p; return a; }
    AiAction PassAction() { return AiAction{}; }

    // ---------------- NORMAL rule-based strategy ----------------

    std::vector<AiAction> AbilityCandidates(const GameState& s)
    {
        std::vector<AiAction> out;
        if (!s.IsStrategy() || s.actions.abilityUsed)
        {
            return out;
        }
        const CellOwner me = s.turn.Current();
        auto bestTarget = [&](AbilityType type, auto valueOf) {
            if (!AbilityManager::CheckUse(s, type).empty()) return;
            int best = -1;
            Pos bestPos{ -1, -1 };
            for (const Pos& p : AbilityManager::ValidTargets(s, type))
            {
                const int v = valueOf(p);
                if (v > best) { best = v; bestPos = p; }
            }
            if (best >= W::WORTHWHILE)
            {
                AiAction a;
                a.kind = AiAction::Kind::UseAbility;
                a.ability = type;
                a.pos = bestPos;
                out.push_back(a);
            }
        };

        // 1. Forced Flip on a valuable enemy stone.
        bestTarget(AbilityType::ForcedFlip, [&](Pos p) {
            const Cell& c = s.board.GetCell(p);
            return SquareValue(s, p) + (c.HasFacility() ? W::FACILITY_TARGET : 0) +
                (c.facility == FacilityType::Fort ? W::FORT_CAPTURE : 0);
        });
        // 2. Siege Break: enemy Fort protecting stones, else an income facility.
        bestTarget(AbilityType::SiegeBreak, [&](Pos p) {
            const Cell& c = s.board.GetCell(p);
            if (c.facility != FacilityType::Fort) return W::WORTHWHILE;
            int defended = 0;
            for (const auto& d : Const::ORTHOGONAL)
            {
                const Cell* n = s.board.TryGetCell(p.x + d[0], p.y + d[1]);
                if (n != nullptr && n->owner == c.owner) ++defended;
            }
            return W::WORTHWHILE + defended * 10;
        });
        // 3. Tactical Placement onto a corner / resource square / base.
        bestTarget(AbilityType::TacticalPlacement, [&](Pos p) { return SquareValue(s, p); });
        // 4. Magic Reinforce on a valuable own stone the opponent could flip next.
        const auto threatened = ThreatenedSquares(s, me);
        bestTarget(AbilityType::MagicReinforce, [&](Pos p) {
            if (!threatened[p.y * Board::SIZE + p.x]) return 0;
            return SquareValue(s, p) + (s.board.GetCell(p).HasFacility() ? W::FACILITY_TARGET : 0);
        });
        // 5. Resource Mining while ore is low.
        if (AbilityManager::CheckUse(s, AbilityType::ResourceMining).empty() &&
            s.Current().resources.ore < W::MINING_ORE_BELOW)
        {
            AiAction a;
            a.kind = AiAction::Kind::UseAbility;
            a.ability = AbilityType::ResourceMining;
            out.push_back(a);
        }
        return out;
    }

    bool BuildCandidate(const GameState& s, AiAction& out)
    {
        if (!s.IsStrategy() || !FacilityManager::CheckTurn(s).empty())
        {
            return false;
        }
        const CellOwner me = s.turn.Current();
        const auto threatened = ThreatenedSquares(s, me);
        const Player& p = s.Current();

        std::vector<FacilityType> order;
        if (p.resources.mana < W::GENERATOR_MANA_BELOW) order.push_back(FacilityType::ManaGenerator);
        order.push_back(FacilityType::MiningFacility);
        order.push_back(FacilityType::Altar);
        order.push_back(FacilityType::Fort);

        for (FacilityType type : order)
        {
            int best = -1;
            Pos bestPos{ -1, -1 };
            for (const Pos& c : FacilityManager::BuildableCells(s, type))
            {
                int safety = IsCorner(c) ? 3 : IsEdge(c) ? 2 : (!threatened[c.y * Board::SIZE + c.x] ? 1 : 0);
                if (type == FacilityType::Fort)
                {
                    int own = 0;
                    for (const auto& d : Const::ORTHOGONAL)
                    {
                        const Cell* n = s.board.TryGetCell(c.x + d[0], c.y + d[1]);
                        if (n != nullptr && n->owner == me) ++own;
                    }
                    if (own < 2) continue;
                    safety += own * 2;
                }
                if (safety < 1) continue; // do not build where it would be taken at once
                if (safety > best) { best = safety; bestPos = c; }
            }
            if (best >= 1)
            {
                out = AiAction{};
                out.kind = AiAction::Kind::Build;
                out.facility = type;
                out.pos = bestPos;
                return true;
            }
        }
        return false;
    }

    int ScorePlacement(const GameState& s, Pos p)
    {
        const CellOwner me = s.turn.Current();
        const CellOwner opp = Opponent(me);
        int score = SquareValue(s, p);
        GameState after = s;
        FlipList flips;
        if (!GameRules::ApplyPlacement(after, p, &flips))
        {
            return std::numeric_limits<int>::min();
        }
        for (const Pos& f : flips)
        {
            if (s.board.GetCell(f).facility == FacilityType::Fort)
            {
                score += W::FORT_CAPTURE;
                break;
            }
        }
        if (CornersAvailable(after, opp) > CornersAvailable(s, opp))
        {
            score += W::GIVES_CORNER;
        }
        if (s.board.CountEmpty() > W::EARLY_EMPTY)
        {
            score -= W::FLIP_PENALTY * flips.count;
        }
        return score;
    }

    bool ChooseNormalPlacement(const GameState& s, std::mt19937& rng, Pos& out)
    {
        const auto moves = GameRules::LegalMoves(s);
        if (moves.empty())
        {
            return false;
        }
        int best = std::numeric_limits<int>::min();
        std::vector<Pos> bestMoves;
        for (const Pos& m : moves)
        {
            const int v = ScorePlacement(s, m);
            if (v > best) { best = v; bestMoves.clear(); }
            if (v == best) bestMoves.push_back(m);
        }
        out = bestMoves[std::uniform_int_distribution<size_t>(0, bestMoves.size() - 1)(rng)];
        return true;
    }

    // ---------------- HARD search ----------------

    struct SearchContext
    {
        CellOwner me = CellOwner::Player;
        Clock::time_point deadline;
        long long nodes = 0;
        bool aborted = false;
    };

    int CountWhere(const Board& b, CellOwner side, bool (*pred)(Pos, const Cell&))
    {
        int n = 0;
        for (int y = 0; y < Board::SIZE; ++y)
            for (int x = 0; x < Board::SIZE; ++x)
            {
                const Cell& c = b.GetCell(x, y);
                if (c.owner == side && pred(Pos{ x, y }, c)) ++n;
            }
        return n;
    }

    int Evaluate(const GameState& s, CellOwner me)
    {
        const CellOwner opp = Opponent(me);
        const int myStones = s.board.CountStones(me);
        const int oppStones = s.board.CountStones(opp);
        if (s.over)
        {
            const int diff = myStones - oppStones;
            return diff > 0 ? W::E_WIN + diff : diff < 0 ? -W::E_WIN + diff : 0;
        }
        const int empty = s.board.CountEmpty();
        const int stoneWeight = empty > 40 ? 1 : empty > 20 ? 2 : 6;
        auto corner = [](Pos p, const Cell&) { return IsCorner(p); };
        auto edge = [](Pos p, const Cell&) { return IsEdge(p); };

        int score = stoneWeight * (myStones - oppStones);
        score += W::E_CORNER * (CountWhere(s.board, me, corner) - CountWhere(s.board, opp, corner));
        score += W::E_EDGE * (CountWhere(s.board, me, edge) - CountWhere(s.board, opp, edge));
        const FlipRule rule = GameRules::FlipRuleFor(s);
        score += W::E_MOBILITY * (MoveValidator::CountLegalMoves(s.board, me, rule) - MoveValidator::CountLegalMoves(s.board, opp, rule));

        if (s.IsStrategy())
        {
            auto facility = [](Pos, const Cell& c) { return c.HasFacility(); };
            auto base = [](Pos, const Cell& c) { return c.type == CellType::NeutralBase; };
            auto resource = [](Pos, const Cell& c) { return IsResourceCell(c.type); };
            const Resources& mr = s.Side(me).resources;
            const Resources& or_ = s.Side(opp).resources;
            score += W::E_RESOURCE * ((mr.mana + mr.ore + mr.influence) - (or_.mana + or_.ore + or_.influence));
            score += W::E_FACILITY * (CountWhere(s.board, me, facility) - CountWhere(s.board, opp, facility));
            score += W::E_BASE * (CountWhere(s.board, me, base) - CountWhere(s.board, opp, base));
            score += W::E_RESOURCE_CELL * (CountWhere(s.board, me, resource) - CountWhere(s.board, opp, resource));
            score += W::E_FLIP_READY * ((mr.mana >= 5 ? 1 : 0) - (or_.mana >= 5 ? 1 : 0));
        }
        return score;
    }

    std::vector<Pos> OrderedMoves(const GameState& s)
    {
        std::vector<Pos> moves = GameRules::LegalMoves(s);
        std::stable_sort(moves.begin(), moves.end(), [](Pos a, Pos b) {
            const int ra = IsCorner(a) ? 0 : IsEdge(a) ? 1 : 2;
            const int rb = IsCorner(b) ? 0 : IsEdge(b) ? 1 : 2;
            return ra < rb;
        });
        return moves;
    }

    int Search(const GameState& s, int depth, int alpha, int beta, SearchContext& ctx)
    {
        ++ctx.nodes;
        if ((ctx.nodes & 255) == 0 && Clock::now() > ctx.deadline)
        {
            ctx.aborted = true;
        }
        if (ctx.aborted)
        {
            return 0;
        }
        if (s.over || depth == 0)
        {
            return Evaluate(s, ctx.me);
        }
        const bool maximizing = (s.turn.Current() == ctx.me);
        const std::vector<Pos> moves = OrderedMoves(s);
        if (moves.empty())
        {
            GameState child = s;
            AIPlayer::Apply(child, PassAction());
            return Search(child, depth - 1, alpha, beta, ctx);
        }
        int best = maximizing ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
        for (const Pos& m : moves)
        {
            GameState child = s;
            AIPlayer::Apply(child, PlaceAction(m));
            const int v = Search(child, depth - 1, alpha, beta, ctx);
            if (maximizing)
            {
                best = std::max(best, v);
                alpha = std::max(alpha, v);
            }
            else
            {
                best = std::min(best, v);
                beta = std::min(beta, v);
            }
            if (beta <= alpha || ctx.aborted)
            {
                break;
            }
        }
        return best;
    }

    struct RootOption
    {
        std::vector<AiAction> pre;   // ability / build before placing
        GameState state;             // state after 'pre'
    };

    std::vector<RootOption> RootOptions(const GameState& s)
    {
        std::vector<std::vector<AiAction>> abilityChoices{ {} };
        std::vector<std::vector<AiAction>> buildChoices{ {} };
        if (s.IsStrategy())
        {
            const auto abilities = AbilityCandidates(s);
            for (size_t i = 0; i < abilities.size() && static_cast<int>(i) < W::HARD_ABILITY_CANDIDATES; ++i)
            {
                abilityChoices.push_back({ abilities[i] });
            }
            AiAction build;
            if (BuildCandidate(s, build))
            {
                buildChoices.push_back({ build });
            }
        }
        std::vector<RootOption> options;
        for (const auto& a : abilityChoices)
        {
            for (const auto& b : buildChoices)
            {
                RootOption opt;
                opt.state = s;
                bool ok = true;
                for (const auto& act : a) ok = ok && AIPlayer::Apply(opt.state, act);
                for (const auto& act : b) ok = ok && AIPlayer::Apply(opt.state, act);
                if (!ok) continue;
                opt.pre = a;
                opt.pre.insert(opt.pre.end(), b.begin(), b.end());
                options.push_back(std::move(opt));
            }
        }
        return options;
    }

    std::vector<AiAction> DecideHard(const GameState& s, const AiSettings& settings, std::mt19937& rng, AiStats& stats)
    {
        const auto start = Clock::now();
        SearchContext ctx;
        ctx.me = s.turn.Current();
        ctx.deadline = start + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(settings.timeLimitSec * 0.9));

        std::vector<RootOption> options = RootOptions(s);
        std::vector<AiAction> best;

        // Fallback before any search: NORMAL's choice.
        {
            Pos p;
            best.clear();
            if (ChooseNormalPlacement(s, rng, p)) best.push_back(PlaceAction(p)); else best.push_back(PassAction());
        }

        for (int depth = 1; depth <= std::max(1, settings.maxDepth); ++depth)
        {
            int bestValue = std::numeric_limits<int>::min();
            std::vector<AiAction> depthBest;
            for (const RootOption& opt : options)
            {
                if (opt.state.over || opt.state.turn.Current() != ctx.me)
                {
                    const int v = Evaluate(opt.state, ctx.me);
                    if (v > bestValue) { bestValue = v; depthBest = opt.pre; }
                    continue;
                }
                std::vector<Pos> moves = OrderedMoves(opt.state);
                if (moves.empty())
                {
                    GameState child = opt.state;
                    AIPlayer::Apply(child, PassAction());
                    const int v = Search(child, depth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), ctx);
                    if (!ctx.aborted && v > bestValue)
                    {
                        bestValue = v;
                        depthBest = opt.pre;
                        depthBest.push_back(PassAction());
                    }
                    continue;
                }
                std::shuffle(moves.begin(), moves.end(), rng);
                std::stable_sort(moves.begin(), moves.end(), [](Pos a, Pos b) {
                    return (IsCorner(a) ? 0 : IsEdge(a) ? 1 : 2) < (IsCorner(b) ? 0 : IsEdge(b) ? 1 : 2);
                });
                for (const Pos& m : moves)
                {
                    GameState child = opt.state;
                    AIPlayer::Apply(child, PlaceAction(m));
                    const int v = Search(child, depth - 1, bestValue, std::numeric_limits<int>::max(), ctx);
                    if (ctx.aborted) break;
                    if (v > bestValue)
                    {
                        bestValue = v;
                        depthBest = opt.pre;
                        depthBest.push_back(PlaceAction(m));
                    }
                }
                if (ctx.aborted) break;
            }
            if (ctx.aborted)
            {
                break;
            }
            if (!depthBest.empty())
            {
                best = depthBest;
                stats.completedDepth = depth;
            }
            if (Clock::now() > ctx.deadline)
            {
                break;
            }
        }
        stats.nodes = ctx.nodes;
        return best;
    }
}

bool AIPlayer::Apply(GameState& state, const AiAction& action)
{
    switch (action.kind)
    {
    case AiAction::Kind::UseAbility:
    {
        const bool ok = AbilityManager::Use(state, action.ability, action.pos).empty();
        if (ok && state.board.IsFull())
        {
            GameRules::EndTurn(state, false);
            if (!state.over) GameRules::BeginTurn(state);
        }
        return ok;
    }
    case AiAction::Kind::Build:
        return FacilityManager::Build(state, action.pos, action.facility).empty();
    case AiAction::Kind::Place:
        if (!GameRules::ApplyPlacement(state, action.pos))
        {
            return false;
        }
        GameRules::EndTurn(state, false);
        break;
    case AiAction::Kind::Pass:
        if (!GameRules::CanPass(state))
        {
            return false;
        }
        GameRules::EndTurn(state, true);
        break;
    }
    if (!state.over)
    {
        GameRules::BeginTurn(state);
    }
    return true;
}

std::vector<AiAction> AIPlayer::DecideTurn(const GameState& state, const AiSettings& settings, AiStats* stats)
{
    const auto start = Clock::now();
    std::mt19937 rng(settings.seed);
    AiStats local;
    std::vector<AiAction> plan;

    if (state.over)
    {
        return plan;
    }

    switch (settings.level)
    {
    case OpponentType::AiNormal:
    {
        GameState work = state;
        const auto abilities = AbilityCandidates(work);
        if (!abilities.empty() && Apply(work, abilities.front()))
        {
            plan.push_back(abilities.front());
        }
        AiAction build;
        if (!work.over && work.turn.Current() == state.turn.Current() && BuildCandidate(work, build) && Apply(work, build))
        {
            plan.push_back(build);
        }
        if (!work.over && work.turn.Current() == state.turn.Current())
        {
            Pos p;
            plan.push_back(ChooseNormalPlacement(work, rng, p) ? PlaceAction(p) : PassAction());
        }
        break;
    }
    case OpponentType::AiHard:
        plan = DecideHard(state, settings, rng, local);
        break;
    default: // EASY: random legal placement, no builds / abilities (F003 R-03)
    {
        const auto moves = GameRules::LegalMoves(state);
        if (moves.empty())
        {
            plan.push_back(PassAction());
        }
        else
        {
            plan.push_back(PlaceAction(moves[std::uniform_int_distribution<size_t>(0, moves.size() - 1)(rng)]));
        }
        break;
    }
    }

    local.elapsedSec = std::chrono::duration<double>(Clock::now() - start).count();
    if (stats != nullptr)
    {
        *stats = local;
    }
    return plan;
}
