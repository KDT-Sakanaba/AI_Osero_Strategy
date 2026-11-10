#include "TestFramework.h"
#include <chrono>
#include <cstring>

namespace
{
    int g_failures = 0;
    int g_failuresInTest = 0;
}

namespace tf
{
    std::vector<TestCase>& Registry()
    {
        static std::vector<TestCase> registry;
        return registry;
    }

    void ReportFailure(const char* file, int line, const std::string& expr)
    {
        ++g_failures;
        ++g_failuresInTest;
        if (g_failuresInTest <= 5)
        {
            std::printf("    FAILED %s(%d): %s\n", file, line, expr.c_str());
        }
    }

    Board MakeBoard(const char* const rows[8])
    {
        Board board;
        board.Initialize();
        for (int y = 0; y < Board::SIZE; ++y)
        {
            for (int x = 0; x < Board::SIZE; ++x)
            {
                const char c = rows[y][x];
                board.SetOwner(Pos{ x, y }, c == 'B' ? CellOwner::Player : c == 'W' ? CellOwner::Enemy : CellOwner::None);
            }
        }
        return board;
    }

    GameState MakeState(RuleMode rule, const char* const rows[8], CellOwner toMove)
    {
        GameState state;
        GameConfig config;
        config.rule = rule;
        GameRules::Setup(state, config);
        const Board stones = MakeBoard(rows);
        for (int y = 0; y < Board::SIZE; ++y)
        {
            for (int x = 0; x < Board::SIZE; ++x)
            {
                state.board.SetOwner(Pos{ x, y }, stones.GetCell(x, y).owner);
            }
        }
        if (toMove == CellOwner::Enemy)
        {
            state.turn.EndTurn(false); // hand the move to white (turn number becomes 2)
        }
        return state;
    }
}

// Usage: ReignTests.exe [name-filter]
int main(int argc, char** argv)
{
    const char* filter = argc > 1 ? argv[1] : nullptr;
    int run = 0;
    int failedTests = 0;
    for (const auto& test : tf::Registry())
    {
        if (filter != nullptr && std::strstr(test.name.c_str(), filter) == nullptr)
        {
            continue;
        }
        if (filter == nullptr && test.name.rfind("SLOW_", 0) == 0)
        {
            continue; // heavy tests run only when named explicitly
        }
        g_failuresInTest = 0;
        const auto start = std::chrono::steady_clock::now();
        test.body();
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        ++run;
        if (g_failuresInTest > 0)
        {
            ++failedTests;
            std::printf("[FAIL] %s (%d failures, %lld ms)\n", test.name.c_str(), g_failuresInTest, static_cast<long long>(ms));
        }
        else
        {
            std::printf("[ OK ] %s (%lld ms)\n", test.name.c_str(), static_cast<long long>(ms));
        }
    }
    std::printf("\n%d tests, %d failed, %d check failures\n", run, failedTests, g_failures);
    return failedTests == 0 ? 0 : 1;
}
