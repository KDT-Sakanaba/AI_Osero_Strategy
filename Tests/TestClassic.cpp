// F001: classic Othello logic (original 17: T001-T008).
#include "TestFramework.h"
#include <algorithm>
#include <random>

namespace
{
    bool Contains(const std::vector<Pos>& v, Pos p)
    {
        return std::find(v.begin(), v.end(), p) != v.end();
    }

    bool ContainsFlip(const FlipList& list, Pos p)
    {
        return std::find(list.begin(), list.end(), p) != list.end();
    }

    bool SameBoard(const Board& a, const Board& b)
    {
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 8; ++x)
                if (a.GetCell(x, y).owner != b.GetCell(x, y).owner) return false;
        return true;
    }

    const FlipRule kClassic{};
}

// T001
TEST(T001_InitialBoard)
{
    Board board;
    board.Initialize();
    CHECK(board.GetCell(3, 3).owner == CellOwner::Enemy);
    CHECK(board.GetCell(4, 4).owner == CellOwner::Enemy);
    CHECK(board.GetCell(4, 3).owner == CellOwner::Player);
    CHECK(board.GetCell(3, 4).owner == CellOwner::Player);
    CHECK_EQ(board.CountStones(CellOwner::Player), 2);
    CHECK_EQ(board.CountStones(CellOwner::Enemy), 2);
    CHECK_EQ(board.CountEmpty(), 60);
}

TEST(Board_InsideAndSafeAccess)
{
    Board board;
    board.Initialize();
    CHECK(Board::IsInside(0, 0));
    CHECK(Board::IsInside(7, 7));
    CHECK(!Board::IsInside(-1, 0));
    CHECK(!Board::IsInside(8, 3));
    CHECK(!Board::IsInside(3, 8));
    CHECK(board.TryGetCell(8, 0) == nullptr);
    CHECK(board.TryGetCell(0, -1) == nullptr);
    CHECK(!board.SetOwner(Pos{ 9, 9 }, CellOwner::Player));
    CHECK_EQ(board.CountEmpty(), 60);
}

// T002
TEST(T002_InitialLegalMoves)
{
    Board board;
    board.Initialize();
    const auto moves = MoveValidator::GetLegalMoves(board, CellOwner::Player, kClassic);
    CHECK_EQ(static_cast<int>(moves.size()), 4);
    CHECK(Contains(moves, Pos{ 3, 2 }));
    CHECK(Contains(moves, Pos{ 2, 3 }));
    CHECK(Contains(moves, Pos{ 5, 4 }));
    CHECK(Contains(moves, Pos{ 4, 5 }));
    const auto white = MoveValidator::GetLegalMoves(board, CellOwner::Enemy, kClassic);
    CHECK_EQ(static_cast<int>(white.size()), 4);
}

// T003
TEST(T003_IllegalPlacementChangesNothing)
{
    GameSession session;
    session.Start(GameConfig{});
    session.TakeEvents();
    const GameState before = session.State();

    CHECK(!session.TryPlace(Pos{ 3, 3 }));  // occupied
    CHECK(!session.TryPlace(Pos{ 0, 0 }));  // flips nothing
    CHECK(!session.TryPlace(Pos{ 2, 2 }));  // diagonal neighbour of own stone: nothing flips
    CHECK(!session.TryPlace(Pos{ -1, 4 })); // outside
    CHECK(!session.TryPlace(Pos{ 8, 8 }));  // outside

    CHECK(SameBoard(before.board, session.State().board));
    CHECK(session.State().turn.Current() == CellOwner::Player);
    CHECK_EQ(session.State().turn.TurnNumber(), 1);
    const auto events = session.TakeEvents();
    CHECK_EQ(static_cast<int>(events.size()), 5);
    for (const auto& e : events) CHECK(e.type == GameEventType::Rejected);
}

// T004
TEST(T004_SingleDirectionFlip)
{
    Board board;
    board.Initialize();
    const FlipList flips = MoveValidator::FindFlips(board, Pos{ 3, 2 }, CellOwner::Player, kClassic);
    CHECK_EQ(flips.count, 1);
    CHECK(ContainsFlip(flips, Pos{ 3, 3 }));

    GameSession session;
    session.Start(GameConfig{});
    CHECK(session.TryPlace(Pos{ 3, 2 }));
    const Board& b = session.State().board;
    CHECK(b.GetCell(3, 2).owner == CellOwner::Player);
    CHECK(b.GetCell(3, 3).owner == CellOwner::Player);
    CHECK_EQ(b.CountStones(CellOwner::Player), 4);
    CHECK_EQ(b.CountStones(CellOwner::Enemy), 1);
    CHECK(session.State().turn.Current() == CellOwner::Enemy);
}

// T005
TEST(T005_AllEightDirections)
{
    const char* rows[8] = {
        "B..B..B.",
        ".W.W.W..",
        "..WWW...",
        "BWW.WWB.",
        "..WWW...",
        ".W.W.W..",
        "B..B..B.",
        "........",
    };
    const Board board = tf::MakeBoard(rows);
    const FlipList flips = MoveValidator::FindFlips(board, Pos{ 3, 3 }, CellOwner::Player, kClassic);
    CHECK_EQ(flips.count, 16);

    GameState state = tf::MakeState(RuleMode::Classic, rows, CellOwner::Player);
    CHECK(GameRules::ApplyPlacement(state, Pos{ 3, 3 }));
    CHECK_EQ(state.board.CountStones(CellOwner::Enemy), 0);
    CHECK_EQ(state.board.CountStones(CellOwner::Player), 8 + 16 + 1);
}

TEST(Validator_EdgeEmptyAndAdjacentOwnCases)
{
    // Row 0: white run reaching the edge with no closing stone -> no flip to the right.
    // Row 2: white then empty -> broken. Row 4: own stone adjacent -> nothing sandwiched.
    const char* rows[8] = {
        "..WWWWW.",
        "........",
        "..W.B...",
        "........",
        "..BW....",
        "........",
        "........",
        "........",
    };
    const Board board = tf::MakeBoard(rows);
    CHECK(MoveValidator::FindFlips(board, Pos{ 1, 0 }, CellOwner::Player, kClassic).Empty());
    CHECK(MoveValidator::FindFlips(board, Pos{ 1, 2 }, CellOwner::Player, kClassic).Empty());
    CHECK(MoveValidator::FindFlips(board, Pos{ 1, 4 }, CellOwner::Player, kClassic).Empty());
    // Closing stone on the far side works: Player at (4,4) sandwiches (3,4).
    const FlipList ok = MoveValidator::FindFlips(board, Pos{ 4, 4 }, CellOwner::Player, kClassic);
    CHECK_EQ(ok.count, 1);
    CHECK(ContainsFlip(ok, Pos{ 3, 4 }));
    // Corner with line along the edge closed by own stone.
    const char* rows2[8] = {
        ".WWB....", "........", "........", "........",
        "........", "........", "........", "........",
    };
    const Board b2 = tf::MakeBoard(rows2);
    CHECK_EQ(MoveValidator::FindFlips(b2, Pos{ 0, 0 }, CellOwner::Player, kClassic).count, 2);
}

// T006
TEST(T006_AutomaticPass)
{
    // Black has no move; white has.
    const char* rows[8] = {
        "WWWWWWWW", "WWWWWWWW", "WWWWWWWW", "WWWWWWWW",
        "WWWWWWWW", "WWWWWWWW", "WWWWWWB.", "WWWWWWW.",
    };
    tf::TestSession session;
    session.Load(tf::MakeState(RuleMode::Classic, rows, CellOwner::Player));
    // Black cannot play: (7,6) is next to B only via white? (6,6)=B: placing at (7,6) needs white between -> none.
    CHECK(!session.IsOver());
    CHECK(session.State().turn.Current() == CellOwner::Enemy);
    CHECK_EQ(session.State().turn.ConsecutivePasses(), 1);
    bool sawPass = false;
    for (const auto& e : session.TakeEvents())
        if (e.type == GameEventType::Passed && e.actor == CellOwner::Player) sawPass = true;
    CHECK(sawPass);
    // Manual pass is refused while a legal move exists.
    CHECK(!session.TryPass());
}

// T007
TEST(T007_BothPassEndsGame)
{
    const char* rows[8] = {
        "BBBB....", "BBBB....", "........", "........",
        "........", "........", "........", "....WWWW",
    };
    tf::TestSession session;
    session.Load(tf::MakeState(RuleMode::Classic, rows, CellOwner::Player));
    CHECK(session.IsOver());
    CHECK(session.State().turn.IsBothPassed());
    const GameResult r = session.Result();
    CHECK(r.winner == CellOwner::Player);
    CHECK_EQ(r.playerStones, 8);
    CHECK_EQ(r.enemyStones, 4);
    bool sawOver = false;
    for (const auto& e : session.TakeEvents()) if (e.type == GameEventType::GameOver) sawOver = true;
    CHECK(sawOver);
}

// T008
TEST(T008_FullBoardJudgement)
{
    const char* blackWins[8] = {
        "BBBBBBBB", "BBBBBBBB", "BBBBBBBB", "BBBBBBBB",
        "BBBBBBBB", "WWWWWWWW", "WWWWWWWW", "WWWWWWW.",
    };
    tf::TestSession s1;
    s1.LoadRaw(tf::MakeState(RuleMode::Classic, blackWins, CellOwner::Player));
    CHECK(s1.TryPlace(Pos{ 7, 7 })); // black fills the last square
    CHECK(s1.IsOver());
    CHECK(s1.State().board.IsFull());
    CHECK(s1.Result().winner == CellOwner::Player);

    const char* whiteWins[8] = {
        "WWWWWWWW", "WWWWWWWW", "WWWWWWWW", "WWWWWWWW",
        "WWWWWWWW", "BBBBBBBB", "BBBBBBBB", "BBBBBBBB",
    };
    CHECK(VictoryManager::Judge(tf::MakeBoard(whiteWins)).winner == CellOwner::Enemy);

    const char* draw[8] = {
        "BBBBBBBB", "BBBBBBBB", "BBBBBBBB", "BBBBBBBB",
        "WWWWWWWW", "WWWWWWWW", "WWWWWWWW", "WWWWWWWW",
    };
    const GameResult d = VictoryManager::Judge(tf::MakeBoard(draw));
    CHECK(d.IsDraw());
    CHECK_EQ(d.playerStones, 32);
    CHECK_EQ(d.enemyStones, 32);
    TurnManager tm;
    tm.Reset();
    CHECK(VictoryManager::IsGameOver(tf::MakeBoard(draw), tm));
}

TEST(Classic_RandomPlayoutsStayConsistent)
{
    std::mt19937 rng(12345);
    for (int game = 0; game < 300; ++game)
    {
        GameSession session;
        session.Start(GameConfig{});
        int guard = 0;
        while (!session.IsOver() && guard++ < 200)
        {
            const auto moves = GameRules::LegalMoves(session.State());
            CHECK(!moves.empty()); // a waiting side always has a move in classic mode
            if (moves.empty()) break;
            const Pos pick = moves[std::uniform_int_distribution<size_t>(0, moves.size() - 1)(rng)];
            const int before = session.State().board.CountEmpty();
            CHECK(session.TryPlace(pick));
            CHECK_EQ(session.State().board.CountEmpty(), before - 1);
        }
        CHECK(session.IsOver());
        const Board& b = session.State().board;
        CHECK_EQ(b.CountStones(CellOwner::Player) + b.CountStones(CellOwner::Enemy) + b.CountEmpty(), 64);
    }
}
