// F003: AI (original 17: T014, F003 AC-01..AC-06).
#include "TestFramework.h"
#include <algorithm>
#include <random>
#include "Player/AIPlayer.h"
#include "Systems/AbilityManager.h"
#include "Systems/FacilityManager.h"

namespace
{
    // Executes an AI plan through GameSession exactly like the game screen does.
    // Returns the number of refused actions.
    int Execute(GameSession& session, const std::vector<AiAction>& plan)
    {
        int refused = 0;
        const CellOwner side = session.State().turn.Current();
        for (const AiAction& a : plan)
        {
            if (session.IsOver() || session.State().turn.Current() != side) break;
            bool ok = false;
            switch (a.kind)
            {
            case AiAction::Kind::UseAbility: ok = session.TryUseAbility(a.ability, a.pos); break;
            case AiAction::Kind::Build:      ok = session.TryBuild(a.facility, a.pos); break;
            case AiAction::Kind::Place:      ok = session.TryPlace(a.pos); break;
            case AiAction::Kind::Pass:       ok = session.TryPass(); break;
            }
            if (!ok) ++refused;
        }
        return refused;
    }

    AiSettings Settings(OpponentType level, std::uint32_t seed, int depth = 4, double limit = 2.0)
    {
        AiSettings s;
        s.level = level;
        s.seed = seed;
        s.maxDepth = depth;
        s.timeLimitSec = limit;
        return s;
    }

    // Plays one game; 'black' and 'white' settings. Returns the result and counts refusals.
    GameResult PlayGame(RuleMode rule, const AiSettings& black, const AiSettings& white, int& refused, int& turns)
    {
        GameConfig cfg;
        cfg.rule = rule;
        GameSession session;
        session.Start(cfg);
        int guard = 0;
        std::uint32_t seedBump = 0;
        while (!session.IsOver() && guard++ < 300)
        {
            AiSettings s = session.State().turn.Current() == CellOwner::Player ? black : white;
            s.seed += ++seedBump;
            const auto before = session.State().turn.TurnNumber();
            refused += Execute(session, AIPlayer::DecideTurn(session.State(), s));
            if (!session.IsOver() && session.State().turn.TurnNumber() == before)
            {
                ++refused;          // the AI failed to finish its turn
                session.TryPass();  // keep the test moving
                if (session.State().turn.TurnNumber() == before)
                {
                    const auto moves = GameRules::LegalMoves(session.State());
                    if (!moves.empty()) session.TryPlace(moves.front());
                }
            }
        }
        turns = session.State().turn.TurnNumber();
        return session.Result();
    }
}

// T014 / AC-01 / AC-06
TEST(T014_AllLevelsOnlyLegalActions)
{
    const OpponentType levels[3] = { OpponentType::AiEasy, OpponentType::AiNormal, OpponentType::AiHard };
    for (RuleMode rule : { RuleMode::Classic, RuleMode::Strategy })
    {
        for (OpponentType level : levels)
        {
            const int games = level == OpponentType::AiHard ? 6 : 30;
            int refused = 0;
            for (int g = 0; g < games; ++g)
            {
                int turns = 0;
                // Fast HARD settings for the legality check (time limit is checked separately).
                const AiSettings me = Settings(level, 100 + g, 2, 0.05);
                const AiSettings rnd = Settings(OpponentType::AiEasy, 900 + g);
                if (g % 2 == 0) PlayGame(rule, me, rnd, refused, turns); else PlayGame(rule, rnd, me, refused, turns);
            }
            CHECK_EQ(refused, 0);
        }
    }
}

// AC-02
TEST(F003_PassWhenNoMove)
{
    const char* rows[8] = {
        "WWWWWWWW", "WWWWWWWW", "WWWWWWWW", "WWWWWWWW",
        "WWWWWWWW", "WWWWWWWW", "WWWWWWB.", "WWWWWWW.",
    };
    for (OpponentType level : { OpponentType::AiEasy, OpponentType::AiNormal, OpponentType::AiHard })
    {
        const GameState s = tf::MakeState(RuleMode::Classic, rows, CellOwner::Player);
        const auto plan = AIPlayer::DecideTurn(s, Settings(level, 1));
        CHECK(!plan.empty());
        CHECK(plan.back().kind == AiAction::Kind::Pass);
    }
    // Over -> no action
    const char* full[8] = {
        "BBBBBBBB", "BBBBBBBB", "BBBBBBBB", "BBBBBBBB",
        "WWWWWWWW", "WWWWWWWW", "WWWWWWWW", "WWWWWWWW",
    };
    GameState over = tf::MakeState(RuleMode::Classic, full, CellOwner::Player);
    over.over = true;
    CHECK(AIPlayer::DecideTurn(over, Settings(OpponentType::AiHard, 1)).empty());
}

// AC-03
TEST(F003_NormalTakesCorner)
{
    // Black can take (0,0) via (1,1) W -> (2,2) B, or play elsewhere.
    const char* rows[8] = {
        "........",
        ".W......",
        "..B.....",
        "...WB...",
        "...BW...",
        "........",
        "........",
        "........",
    };
    for (std::uint32_t seed = 1; seed <= 10; ++seed)
    {
        const GameState s = tf::MakeState(RuleMode::Classic, rows, CellOwner::Player);
        const auto plan = AIPlayer::DecideTurn(s, Settings(OpponentType::AiNormal, seed));
        CHECK(plan.back().kind == AiAction::Kind::Place);
        CHECK(plan.back().pos == (Pos{ 0, 0 }));
    }
    const GameState s = tf::MakeState(RuleMode::Classic, rows, CellOwner::Player);
    const auto hard = AIPlayer::DecideTurn(s, Settings(OpponentType::AiHard, 3));
    CHECK(hard.back().pos == (Pos{ 0, 0 }));
}

TEST(F003_EasyNeverUsesStrategyActions)
{
    for (std::uint32_t seed = 1; seed <= 20; ++seed)
    {
        GameState s;
        GameConfig cfg;
        cfg.rule = RuleMode::Strategy;
        GameRules::Setup(s, cfg);
        s.players[0].resources = Resources{ 15, 15, 10 };
        const auto plan = AIPlayer::DecideTurn(s, Settings(OpponentType::AiEasy, seed));
        CHECK_EQ(static_cast<int>(plan.size()), 1);
        CHECK(plan[0].kind == AiAction::Kind::Place);
    }
}

TEST(F003_NormalUsesStrategyWhenRich)
{
    const char* rows[8] = {
        "........", "........", "........", "...WB...",
        "...BW...", "........", "........", "........",
    };
    GameState s = tf::MakeState(RuleMode::Strategy, rows, CellOwner::Player);
    s.players[0].resources = Resources{ 3, 4, 0 };
    const auto plan = AIPlayer::DecideTurn(s, Settings(OpponentType::AiNormal, 5));
    bool mined = false;
    for (const auto& a : plan) if (a.kind == AiAction::Kind::UseAbility && a.ability == AbilityType::ResourceMining) mined = true;
    CHECK(mined);
    CHECK(plan.back().kind == AiAction::Kind::Place);
}

// AC-04: thinking time (meaningful in Release; Debug only reports)
TEST(F003_HardThinkingTimeLimit)
{
    std::mt19937 rng(42);
    double worst = 0.0;
    int deepest = 0;
    for (int i = 0; i < 20; ++i)
    {
        GameConfig cfg;
        cfg.rule = (i % 2 == 0) ? RuleMode::Classic : RuleMode::Strategy;
        GameSession session;
        session.Start(cfg);
        // Random opening of 10-24 plies to reach a midgame position.
        const int opening = 10 + static_cast<int>(rng() % 15);
        for (int k = 0; k < opening && !session.IsOver(); ++k)
        {
            const auto moves = GameRules::LegalMoves(session.State());
            if (moves.empty()) { session.TryPass(); continue; }
            session.TryPlace(moves[rng() % moves.size()]);
        }
        if (session.IsOver()) continue;
        AiStats stats;
        AIPlayer::DecideTurn(session.State(), Settings(OpponentType::AiHard, 7 + i), &stats);
        worst = std::max(worst, stats.elapsedSec);
        deepest = std::max(deepest, stats.completedDepth);
    }
    std::printf("    HARD worst thinking time %.3f s, deepest completed depth %d\n", worst, deepest);
#ifdef NDEBUG
    CHECK(worst <= 2.0);
    CHECK(deepest >= 4);
#endif
}

// AC-05: strength order (slow; run with: ReignTests.exe SLOW_)
TEST(SLOW_AC05_StrengthOrder)
{
    struct Match { OpponentType strong; OpponentType weak; RuleMode rule; const char* name; };
    const Match matches[] = {
        { OpponentType::AiNormal, OpponentType::AiEasy,   RuleMode::Classic,  "NORMAL vs EASY  (classic)" },
        { OpponentType::AiNormal, OpponentType::AiEasy,   RuleMode::Strategy, "NORMAL vs EASY  (strategy)" },
        { OpponentType::AiHard,   OpponentType::AiNormal, RuleMode::Classic,  "HARD vs NORMAL  (classic)" },
        { OpponentType::AiHard,   OpponentType::AiNormal, RuleMode::Strategy, "HARD vs NORMAL  (strategy)" },
    };
    for (const Match& m : matches)
    {
        int wins = 0, draws = 0, refused = 0;
        const int games = 50;
        for (int g = 0; g < games; ++g)
        {
            int turns = 0;
            const AiSettings strong = Settings(m.strong, 1000 + g);
            const AiSettings weak = Settings(m.weak, 5000 + g);
            const bool strongIsBlack = (g % 2 == 0);
            const GameResult r = strongIsBlack ? PlayGame(m.rule, strong, weak, refused, turns)
                                               : PlayGame(m.rule, weak, strong, refused, turns);
            const CellOwner strongSide = strongIsBlack ? CellOwner::Player : CellOwner::Enemy;
            if (r.IsDraw()) ++draws; else if (r.winner == strongSide) ++wins;
        }
        const double rate = static_cast<double>(wins) / games;
        std::printf("    %s: %d wins, %d draws / %d games (%.0f%%)\n", m.name, wins, draws, games, rate * 100.0);
        CHECK(rate >= 0.6);
        CHECK_EQ(refused, 0);
    }
}
