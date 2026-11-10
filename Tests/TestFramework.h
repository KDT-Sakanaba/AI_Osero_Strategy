#pragma once
// Minimal self-registering test runner (no external test framework, constitution 4).

#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include "Board/Board.h"
#include "Systems/GameSession.h"

namespace tf
{
    struct TestCase
    {
        std::string name;
        std::function<void()> body;
    };

    std::vector<TestCase>& Registry();
    void ReportFailure(const char* file, int line, const std::string& expr);

    struct Registrar
    {
        Registrar(const char* name, std::function<void()> body) { Registry().push_back({ name, std::move(body) }); }
    };

    // Board from 8 strings, row y = 0..7, 'B' = Player (black), 'W' = Enemy (white), '.' = empty.
    Board MakeBoard(const char* const rows[8]);

    // Session whose state can be injected.
    class TestSession : public GameSession
    {
    public:
        // Replaces the state and runs turn-start processing (income, automatic pass).
        void Load(const GameState& state)
        {
            state_ = state;
            events_.clear();
            StartTurn();
        }
        // Replaces the state without turn-start processing.
        void LoadRaw(const GameState& state) { state_ = state; events_.clear(); }
        GameState& Mutable() { return state_; }
    };

    GameState MakeState(RuleMode rule, const char* const rows[8], CellOwner toMove);
}

#define TF_CONCAT_(a, b) a##b
#define TF_CONCAT(a, b) TF_CONCAT_(a, b)
#define TEST(name) \
    static void name(); \
    static tf::Registrar TF_CONCAT(reg_, name)(#name, name); \
    static void name()

#define CHECK(expr) \
    do { if (!(expr)) { tf::ReportFailure(__FILE__, __LINE__, #expr); } } while (0)

#define CHECK_EQ(a, b) \
    do { if (!((a) == (b))) { tf::ReportFailure(__FILE__, __LINE__, std::string(#a " == " #b " (got ") + std::to_string(a) + " vs " + std::to_string(b) + ")"); } } while (0)
