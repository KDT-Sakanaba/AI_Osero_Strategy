// F002: strategy mode logic (original 17: T009-T013, F002 AC-01..AC-11).
#include "TestFramework.h"
#include <algorithm>
#include <random>
#include "Systems/AbilityManager.h"
#include "Systems/FacilityManager.h"
#include "Systems/MapGenerator.h"
#include "Systems/ResourceManager.h"

namespace
{
    const char* kEmpty[8] = {
        "........", "........", "........", "........",
        "........", "........", "........", "........",
    };

    GameState Strategy(const char* const rows[8], CellOwner toMove = CellOwner::Player)
    {
        return tf::MakeState(RuleMode::Strategy, rows, toMove);
    }

    void PutFacility(GameState& s, Pos p, FacilityType f)
    {
        s.board.SetFacility(p, f);
        FacilityManager::RecomputeDefense(s.board);
    }

    bool SameResources(const Resources& a, const Resources& b)
    {
        return a.mana == b.mana && a.ore == b.ore && a.influence == b.influence;
    }
}

TEST(F002_LayoutIsFixedAndSymmetric)
{
    GameState s;
    GameConfig cfg;
    cfg.rule = RuleMode::Strategy;
    GameRules::Setup(s, cfg);
    int counts[5] = {};
    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            const CellType t = s.board.GetCell(x, y).type;
            ++counts[static_cast<int>(t)];
            CHECK(t == s.board.GetCell(7 - x, 7 - y).type); // point symmetry
        }
    }
    CHECK_EQ(counts[static_cast<int>(CellType::ManaMine)], 4);
    CHECK_EQ(counts[static_cast<int>(CellType::OreMine)], 4);
    CHECK_EQ(counts[static_cast<int>(CellType::Altar)], 2);
    CHECK_EQ(counts[static_cast<int>(CellType::NeutralBase)], 4);
    for (Pos corner : { Pos{ 0, 0 }, Pos{ 7, 0 }, Pos{ 0, 7 }, Pos{ 7, 7 }, Pos{ 3, 3 }, Pos{ 4, 4 }, Pos{ 3, 4 }, Pos{ 4, 3 } })
        CHECK(s.board.GetCell(corner).type == CellType::Normal);
    CHECK(s.board.GetCell(2, 0).type == CellType::ManaMine);
    CHECK(s.board.GetCell(5, 2).type == CellType::Altar);
    CHECK(s.board.GetCell(3, 1).type == CellType::NeutralBase);
    CHECK_EQ(s.players[0].resources.mana, 3);
    CHECK_EQ(s.players[0].resources.ore, 2);
    CHECK_EQ(s.players[0].resources.influence, 0);

    GameState classic;
    GameRules::Setup(classic, GameConfig{});
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            CHECK(classic.board.GetCell(x, y).type == CellType::Normal);
    CHECK(classic.players[0].resources.IsZero());
}

// T009
TEST(T009_ResourceIncome)
{
    const char* rows[8] = {
        "..B..W..",   // (2,0) mana mine black, (5,0) ore mine white
        "...B....",   // (3,1) neutral base black
        "B....B..",   // (0,2) ore mine black, (5,2) altar black
        "...WB...",
        "...BW...",
        "........",
        "........",
        "........",
    };
    GameState s = Strategy(rows);
    // mana: 2 (mine) + 1 (altar) ; ore: 2 ; influence: 1 (base) + 1 (altar)
    const Resources income = ResourceManager::TurnIncome(s, CellOwner::Player);
    CHECK_EQ(income.mana, 3);
    CHECK_EQ(income.ore, 2);
    CHECK_EQ(income.influence, 2);
    const Resources gained = GameRules::BeginTurn(s);
    CHECK(SameResources(gained, income));
    CHECK_EQ(s.players[0].resources.mana, 6);
    CHECK_EQ(s.players[0].resources.ore, 4);
    CHECK_EQ(s.players[0].resources.influence, 2);
    CHECK_EQ(s.players[1].resources.ore, 2); // white gets nothing on black's turn

    // Cap: overflow is lost
    s.players[0].resources = Resources{ 14, 15, 10 };
    const Resources capped = GameRules::BeginTurn(s);
    CHECK_EQ(capped.mana, 1);
    CHECK_EQ(capped.ore, 0);
    CHECK_EQ(capped.influence, 0);
    CHECK_EQ(s.players[0].resources.mana, 15);

    // Classic mode: never any income
    GameState c = tf::MakeState(RuleMode::Classic, rows, CellOwner::Player);
    CHECK(GameRules::BeginTurn(c).IsZero());
}

TEST(T009_IncomeOnlyAtOwnTurnStartViaSession)
{
    const char* rows[8] = {
        "..B.....", "........", "........", "...WB...",
        "...BW...", "........", "........", "........",
    };
    tf::TestSession session;
    session.Load(Strategy(rows)); // black's turn starts: +2 mana from (2,0)
    CHECK_EQ(session.State().players[0].resources.mana, 5);
    CHECK(session.TryPlace(Pos{ 2, 3 }));     // black plays; white's turn starts
    CHECK_EQ(session.State().players[0].resources.mana, 5);
    CHECK_EQ(session.State().players[1].resources.mana, 3);
}

// T010
TEST(T010_ShortageRefusedAndNothingChanges)
{
    const char* rows[8] = {
        "........", "........", "........", "...WB...",
        "...BW...", "........", "........", "........",
    };
    tf::TestSession session;
    session.LoadRaw(Strategy(rows));
    const GameState before = session.State();
    CHECK(!session.TryUseAbility(AbilityType::ForcedFlip, Pos{ 3, 3 })); // mana 3 < 5
    CHECK(!session.TryUseAbility(AbilityType::SiegeBreak, Pos{ 3, 3 })); // influence 0
    CHECK(!session.TryBuild(FacilityType::Fort, Pos{ 4, 3 }));            // ore 2 < 4
    CHECK(!session.TryBuild(FacilityType::MiningFacility, Pos{ 3, 3 }));   // enemy square
    const GameState& after = session.State();
    CHECK(SameResources(before.players[0].resources, after.players[0].resources));
    CHECK(after.board.GetCell(3, 3).owner == CellOwner::Enemy);
    CHECK(!after.board.GetCell(4, 3).HasFacility());
    CHECK(!after.actions.abilityUsed);
    CHECK(!after.actions.built);
    const auto events = session.TakeEvents();
    CHECK_EQ(static_cast<int>(events.size()), 4);
    CHECK(events[0].message == "マナが不足しています");
    CHECK(events[1].message == "影響力が不足しています");
    CHECK(events[2].message == "鉱石が不足しています");
    CHECK(events[3].message == "このマスには建設できません");
}

// T011
TEST(T011_BuildAllFacilitiesAndEffects)
{
    const char* rows[8] = {
        "........", "........", "..BBBB..", "...WB...",
        "...BW...", "........", "........", "........",
    };
    GameState s = Strategy(rows);
    s.players[0].resources = Resources{ 15, 15, 10 };

    CHECK(FacilityManager::Build(s, Pos{ 2, 2 }, FacilityType::MiningFacility).empty());
    CHECK_EQ(s.players[0].resources.ore, 12);
    CHECK(FacilityManager::Build(s, Pos{ 3, 2 }, FacilityType::ManaGenerator) == "このターンはすでに建設しました");
    s.actions = TurnActions{};
    CHECK(FacilityManager::Build(s, Pos{ 3, 2 }, FacilityType::ManaGenerator).empty());
    CHECK_EQ(s.players[0].resources.ore, 10);
    CHECK_EQ(s.players[0].resources.mana, 13);
    s.actions = TurnActions{};
    CHECK(FacilityManager::Build(s, Pos{ 4, 2 }, FacilityType::Altar).empty());
    CHECK_EQ(s.players[0].resources.ore, 7);
    CHECK_EQ(s.players[0].resources.influence, 9);
    s.actions = TurnActions{};
    CHECK(FacilityManager::Build(s, Pos{ 5, 2 }, FacilityType::Fort).empty()); // (5,2) is an altar square: allowed
    CHECK_EQ(s.players[0].resources.ore, 3);
    s.actions = TurnActions{};
    CHECK(FacilityManager::Build(s, Pos{ 5, 2 }, FacilityType::MiningFacility) == "このマスには建設できません"); // one per square
    CHECK(FacilityManager::Build(s, Pos{ 6, 6 }, FacilityType::MiningFacility) == "このマスには建設できません"); // empty square

    // Next own turn: facility income. ore+1 (mine), mana+1 (generator), influence+1 (altar facility),
    // plus altar square (5,2): mana+1 influence+1.
    // Fort defends (4,2) up/down/left/right neighbours of (5,2): (4,2) own, (6,2) empty, (5,1) empty, (5,3) empty
    CHECK(s.board.GetCell(4, 2).isDefended);
    CHECK(!s.board.GetCell(5, 2).isDefended); // the Fort square itself is not defended

    s.players[0].resources = Resources{ 0, 0, 0 };
    GameRules::EndTurn(s, false);                       // end of black's turn: the Fort stays
    CHECK(s.board.GetCell(5, 2).facility == FacilityType::Fort);
    CHECK(s.board.GetCell(4, 2).isDefended);            // protects during white's turn
    const GameRules::TurnEndInfo info = GameRules::EndTurn(s, true); // end of white's turn: Fort gone
    CHECK_EQ(info.fortsRemoved, 1);
    CHECK(s.board.GetCell(5, 2).facility == FacilityType::None);
    CHECK(!s.board.GetCell(4, 2).isDefended);
    const Resources g = GameRules::BeginTurn(s);
    CHECK_EQ(g.ore, 1);
    CHECK_EQ(g.mana, 2);
    CHECK_EQ(g.influence, 2);
}

// User rule 2026-11-10: a Fort protects for one opponent turn; taking it gives one extra turn.
TEST(F002_FortCaptureGivesExtraTurn)
{
    // White fort at (2,3) on a white stone; black at (3,3) can sandwich it from (1,3).
    const char* rows[8] = {
        "........", "........", "........", "..WB....",
        "........", "........", "........", "........",
    };
    tf::TestSession session;
    GameState st = Strategy(rows, CellOwner::Player);
    st.board.SetFacility(Pos{ 2, 3 }, FacilityType::Fort);
    FacilityManager::RecomputeDefense(st.board);
    session.LoadRaw(st);
    const int turnBefore = session.State().turn.TurnNumber();
    CHECK(session.TryPlace(Pos{ 1, 3 }));
    const GameState& after = session.State();
    CHECK(after.turn.Current() == CellOwner::Player);           // black moves again
    CHECK_EQ(after.turn.TurnNumber(), turnBefore + 1);
    CHECK(after.board.GetCell(2, 3).owner == CellOwner::Player);
    CHECK(after.board.GetCell(2, 3).facility == FacilityType::Fort); // the Fort is black's now
    bool sawExtra = false;
    for (const auto& e : session.TakeEvents()) if (e.type == GameEventType::ExtraTurn) sawExtra = true;
    CHECK(sawExtra);
    CHECK(!after.actions.capturedFort);

    // The taken Fort protects through white's next turn, then disappears.
    GameState& m = session.Mutable();
    m.board.SetOwner(Pos{ 2, 2 }, CellOwner::Player);
    FacilityManager::RecomputeDefense(m.board);
    CHECK(m.board.GetCell(2, 2).isDefended);
    GameRules::EndTurn(m, true);                                 // black's extra turn ends: kept
    CHECK(m.board.GetCell(2, 3).facility == FacilityType::Fort);
    GameRules::EndTurn(m, true);                                 // white's turn ends: gone
    CHECK(m.board.GetCell(2, 3).facility == FacilityType::None);
    CHECK(!m.board.GetCell(2, 2).isDefended);
}

TEST(F002_FortCaptureByForcedFlip)
{
    const char* rows[8] = {
        "........", "........", "........", "...WB...",
        "...BW...", "........", "........", "........",
    };
    tf::TestSession session;
    GameState st = Strategy(rows, CellOwner::Player);
    st.board.SetFacility(Pos{ 3, 3 }, FacilityType::Fort);
    FacilityManager::RecomputeDefense(st.board);
    st.players[0].resources.mana = 5;
    session.LoadRaw(st);
    CHECK(session.TryUseAbility(AbilityType::ForcedFlip, Pos{ 3, 3 }));
    CHECK(session.State().actions.capturedFort);
    const auto moves = GameRules::LegalMoves(session.State());
    CHECK(!moves.empty());
    if (!moves.empty()) CHECK(session.TryPlace(moves.front()));
    CHECK(session.State().turn.Current() == CellOwner::Player);  // extra turn after the placement
    CHECK_EQ(session.State().turn.ConsecutivePasses(), 0);
}

// T012
TEST(T012_FacilityChangesOwnerWithStone)
{
    const char* rows[8] = {
        "........", "........", "........", "..WB....",
        "........", "........", "........", "........",
    };
    GameState s = Strategy(rows, CellOwner::Player);
    PutFacility(s, Pos{ 2, 3 }, FacilityType::MiningFacility); // white owns the mine (stone at (2,3))
    CHECK_EQ(ResourceManager::TurnIncome(s, CellOwner::Enemy).ore, 1);
    CHECK(GameRules::ApplyPlacement(s, Pos{ 1, 3 }));          // black sandwiches (2,3)
    CHECK(s.board.GetCell(2, 3).owner == CellOwner::Player);
    CHECK(s.board.GetCell(2, 3).facility == FacilityType::MiningFacility);
    CHECK_EQ(ResourceManager::TurnIncome(s, CellOwner::Player).ore, 1);
    CHECK_EQ(ResourceManager::TurnIncome(s, CellOwner::Enemy).ore, 0);

    // Forced Flip also takes the facility.
    GameState f = Strategy(rows, CellOwner::Player);
    PutFacility(f, Pos{ 2, 3 }, FacilityType::ManaGenerator);
    f.players[0].resources.mana = 5;
    CHECK(AbilityManager::Use(f, AbilityType::ForcedFlip, Pos{ 2, 3 }).empty());
    CHECK_EQ(ResourceManager::TurnIncome(f, CellOwner::Player).mana, 1);
}

TEST(F002_FortDefenceRangeOverlapAndCapture)
{
    const char* rows[8] = {
        "........",
        "..BBB...",
        "..BBBW..",
        "..BBB...",
        "........",
        "........",
        "........",
        "........",
    };
    GameState s = Strategy(rows);
    PutFacility(s, Pos{ 3, 2 }, FacilityType::Fort);
    CHECK(s.board.GetCell(3, 1).isDefended);
    CHECK(s.board.GetCell(3, 3).isDefended);
    CHECK(s.board.GetCell(2, 2).isDefended);
    CHECK(s.board.GetCell(4, 2).isDefended);
    CHECK(!s.board.GetCell(2, 1).isDefended); // diagonal
    CHECK(!s.board.GetCell(3, 2).isDefended); // fort square itself
    CHECK(!s.board.GetCell(5, 2).isDefended); // enemy stone not in range anyway

    PutFacility(s, Pos{ 4, 2 }, FacilityType::Fort); // overlap: still simply "defended"
    CHECK(s.board.GetCell(3, 2).isDefended);         // now defended by the second fort
    CHECK(s.board.GetCell(4, 1).isDefended);

    // Fort captured: defence moves to the new owner.
    GameState c = Strategy(rows);
    PutFacility(c, Pos{ 3, 2 }, FacilityType::Fort);
    c.board.SetOwner(Pos{ 3, 2 }, CellOwner::Enemy);
    FacilityManager::RecomputeDefense(c.board);
    CHECK(!c.board.GetCell(3, 1).isDefended);
    CHECK(!c.board.GetCell(2, 2).isDefended);
    c.board.SetOwner(Pos{ 4, 2 }, CellOwner::Enemy);
    FacilityManager::RecomputeDefense(c.board);
    CHECK(c.board.GetCell(4, 2).isDefended);
}

TEST(F002_R16_ProtectedStonesInsideLines)
{
    // White to move. Row 0: . B B B W  -> (2,0) defended by black fort at (2,1).
    const char* rows[8] = {
        ".BBBW...",
        "..B.....",
        "........",
        "W.......",
        "B.......",
        "........",
        "........",
        "........",
    };
    GameState s = Strategy(rows, CellOwner::Enemy);
    PutFacility(s, Pos{ 2, 1 }, FacilityType::Fort);
    CHECK(s.board.GetCell(2, 0).isDefended);
    const FlipList flips = GameRules::FindFlips(s, Pos{ 0, 0 }, CellOwner::Enemy);
    CHECK_EQ(flips.count, 2);
    CHECK(std::find(flips.begin(), flips.end(), Pos{ 1, 0 }) != flips.end());
    CHECK(std::find(flips.begin(), flips.end(), Pos{ 3, 0 }) != flips.end());
    CHECK(std::find(flips.begin(), flips.end(), Pos{ 2, 0 }) == flips.end());

    // A direction with only protected stones is not a move: W at (0,3), enhanced B at (0,4), place (0,5).
    GameState e = Strategy(rows, CellOwner::Enemy);
    e.board.SetEnhancedUntil(Pos{ 0, 4 }, 99);
    CHECK(GameRules::FindFlips(e, Pos{ 0, 5 }, CellOwner::Enemy).Empty());
    // Classic mode ignores protection entirely.
    GameState c = tf::MakeState(RuleMode::Classic, rows, CellOwner::Enemy);
    c.board.SetEnhancedUntil(Pos{ 0, 4 }, 99);
    CHECK_EQ(GameRules::FindFlips(c, Pos{ 0, 5 }, CellOwner::Enemy).count, 1);
}

// T013: abilities
TEST(T013_MagicReinforce)
{
    const char* rows[8] = {
        "........", "........", "........", "..WBB...",
        "........", "........", "........", "........",
    };
    tf::TestSession session;
    session.LoadRaw(Strategy(rows, CellOwner::Player));
    CHECK(!session.TryUseAbility(AbilityType::MagicReinforce, Pos{ 2, 3 })); // not own stone
    CHECK(session.TryUseAbility(AbilityType::MagicReinforce, Pos{ 3, 3 }));
    CHECK_EQ(session.State().players[0].resources.mana, 0);
    CHECK(!session.TryUseAbility(AbilityType::ResourceMining, Pos{}));         // one ability per turn
    CHECK(session.TryPass() == false);                                         // black has moves
    GameState& st = session.Mutable();
    GameRules::EndTurn(st, false);                       // end black's turn (turn 1)
    CHECK(st.board.GetCell(3, 3).IsEnhanced());
    // White's turn (turn 2): (3,3) is protected, so W at (5,3) flips only (4,3).
    const FlipList flips = GameRules::FindFlips(st, Pos{ 5, 3 }, CellOwner::Enemy);
    CHECK_EQ(flips.count, 1);
    CHECK(flips.items[0] == (Pos{ 4, 3 }));
    // Forced Flip cannot target it either.
    st.players[1].resources.mana = 5;
    CHECK(!AbilityManager::IsValidTarget(st, AbilityType::ForcedFlip, Pos{ 3, 3 }));
    GameRules::EndTurn(st, true);                        // end of white's next turn: expires
    CHECK(!st.board.GetCell(3, 3).IsEnhanced());
}

TEST(T013_ForcedFlipAndProtectionRules)
{
    const char* rows[8] = {
        "........", "..W.....", "..W.....", "..BW....",
        "........", "........", "........", "........",
    };
    GameState s = Strategy(rows, CellOwner::Player);
    s.players[0].resources.mana = 10;
    CHECK(AbilityManager::Use(s, AbilityType::ForcedFlip, Pos{ 3, 3 }).empty());
    CHECK(s.board.GetCell(3, 3).owner == CellOwner::Player);
    CHECK(s.board.GetCell(2, 2).owner == CellOwner::Enemy); // no chain reaction
    CHECK_EQ(s.players[0].resources.mana, 5);

    GameState d = Strategy(rows, CellOwner::Player);
    d.players[0].resources.mana = 10;
    PutFacility(d, Pos{ 2, 1 }, FacilityType::Fort); // white fort defends (2,2)
    CHECK(d.board.GetCell(2, 2).isDefended);
    CHECK(AbilityManager::Use(d, AbilityType::ForcedFlip, Pos{ 2, 2 }) == "そのマスは対象にできません");
    CHECK_EQ(d.players[0].resources.mana, 10);
}

TEST(T013_ResourceMiningLimits)
{
    GameState s = Strategy(kEmpty, CellOwner::Player);
    s.board.SetOwner(Pos{ 0, 0 }, CellOwner::Player);
    for (int i = 0; i < 3; ++i)
    {
        AbilityResult r;
        CHECK(AbilityManager::Use(s, AbilityType::ResourceMining, Pos{}, &r).empty());
        CHECK_EQ(r.gained.ore, 2);
        s.actions = TurnActions{};
    }
    CHECK_EQ(s.players[0].resources.ore, 8);
    CHECK(AbilityManager::CheckUse(s, AbilityType::ResourceMining) == "この能力は使用回数の上限に達しています");
    GameState m = Strategy(kEmpty, CellOwner::Player);
    m.players[0].resources.ore = 15;
    CHECK(AbilityManager::CheckUse(m, AbilityType::ResourceMining) == "鉱石がすでに最大です");
    m.players[0].resources.ore = 14;
    AbilityResult r2;
    CHECK(AbilityManager::Use(m, AbilityType::ResourceMining, Pos{}, &r2).empty());
    CHECK_EQ(r2.gained.ore, 1);
}

TEST(T013_SiegeBreakDuration)
{
    const char* rows[8] = {
        "........", "...W....", "..WWW...", "...W....",
        "........", "...B....", "........", "........",
    };
    GameState s = Strategy(rows, CellOwner::Player);  // turn 1, black
    PutFacility(s, Pos{ 3, 2 }, FacilityType::Fort);  // white fort
    CHECK(s.board.GetCell(3, 3).isDefended);
    s.players[0].resources.influence = 3;
    CHECK(AbilityManager::Use(s, AbilityType::SiegeBreak, Pos{ 3, 2 }).empty());
    CHECK(!s.board.GetCell(3, 3).isDefended);                    // Fort effect off immediately
    CHECK_EQ(s.players[0].resources.influence, 0);

    // Duration: until the facility owner (white) has finished 2 turns. Measured on a mine.
    GameState d = Strategy(rows, CellOwner::Player);
    d.board.SetFacility(Pos{ 3, 1 }, FacilityType::MiningFacility);
    d.players[0].resources.influence = 3;
    CHECK(AbilityManager::Use(d, AbilityType::SiegeBreak, Pos{ 3, 1 }).empty());
    GameRules::EndTurn(d, true);                                // end of black's turn (cast)
    CHECK(d.board.GetCell(3, 1).IsFacilityDisabled());
    GameRules::EndTurn(d, true);                                // end of white's 1st turn
    CHECK(d.board.GetCell(3, 1).IsFacilityDisabled());
    GameRules::EndTurn(d, true);                                // end of black's turn
    CHECK(d.board.GetCell(3, 1).IsFacilityDisabled());
    GameRules::EndTurn(d, true);                                // end of white's 2nd turn -> restored
    CHECK(!d.board.GetCell(3, 1).IsFacilityDisabled());

    // Income is lost while disabled.
    GameState i = Strategy(rows, CellOwner::Player);
    i.board.SetFacility(Pos{ 3, 1 }, FacilityType::MiningFacility);
    i.players[0].resources.influence = 3;
    CHECK(AbilityManager::Use(i, AbilityType::SiegeBreak, Pos{ 3, 1 }).empty());
    CHECK_EQ(ResourceManager::TurnIncome(i, CellOwner::Enemy).ore, 0);
    CHECK(AbilityManager::CheckUse(i, AbilityType::SiegeBreak) == "このターンはすでに能力を使用しました");
    i.actions = TurnActions{};
    i.players[0].resources.influence = 3;
    CHECK(!AbilityManager::IsValidTarget(i, AbilityType::SiegeBreak, Pos{ 3, 1 })); // already disabled
}

TEST(T013_TacticalPlacement)
{
    const char* rows[8] = {
        "........", "........", "........", "...WB...",
        "...BW...", "........", "........", "........",
    };
    tf::TestSession session;
    GameState s = Strategy(rows, CellOwner::Player);
    s.players[0].resources.influence = 10;
    session.LoadRaw(s);
    CHECK(!session.TryUseAbility(AbilityType::TacticalPlacement, Pos{ 0, 0 }));  // not adjacent
    CHECK(session.TryUseAbility(AbilityType::TacticalPlacement, Pos{ 2, 5 }));   // adjacent to (3,4)
    const GameState& st = session.State();
    CHECK(st.board.GetCell(2, 5).owner == CellOwner::Player);
    CHECK(st.board.GetCell(3, 3).owner == CellOwner::Enemy);   // nothing flipped
    CHECK_EQ(st.players[0].resources.influence, 5);
    CHECK(st.turn.Current() == CellOwner::Player);             // normal placement still to come
    CHECK(session.TryPlace(Pos{ 5, 4 }));
    CHECK(st.turn.Current() == CellOwner::Enemy);
    GameState& m = session.Mutable();
    GameRules::EndTurn(m, true);
    m.players[0].resources.influence = 10;
    CHECK(AbilityManager::CheckUse(m, AbilityType::TacticalPlacement) == "この能力は使用回数の上限に達しています");
}

TEST(F002_PassRulesInStrategy)
{
    // Black has no legal move but may still mine -> no automatic pass, pass button allowed.
    const char* rows[8] = {
        "WWWWWWWW", "WWWWWWWW", "WWWWWWWW", "WWWWWWWW",
        "WWWWWWWW", "WWWWWWWW", "WWWWWWB.", "WWWWWWW.",
    };
    tf::TestSession session;
    session.Load(Strategy(rows, CellOwner::Player));
    CHECK(session.State().turn.Current() == CellOwner::Player);
    CHECK(GameRules::CanPass(session.State()));
    CHECK(!GameRules::MustAutoPass(session.State()));
    CHECK(session.TryUseAbility(AbilityType::ResourceMining, Pos{}));
    CHECK(session.TryPass());
    CHECK(session.State().turn.Current() == CellOwner::Enemy);
    CHECK_EQ(session.State().turn.ConsecutivePasses(), 1);

    // Nothing at all possible -> automatic pass.
    GameState s = Strategy(rows, CellOwner::Player);
    s.players[0].abilityUses[AbilityIndex(AbilityType::ResourceMining)] = 3;
    s.players[0].resources = Resources{ 0, 0, 0 };
    tf::TestSession auto_;
    auto_.Load(s);
    CHECK(auto_.State().turn.Current() == CellOwner::Enemy);
}

TEST(F002_OneBuildOneAbilityThenPlacementEndsTurn)
{
    const char* rows[8] = {
        "........", "........", "........", "...WB...",
        "...BW...", "........", "........", "........",
    };
    tf::TestSession session;
    GameState s = Strategy(rows, CellOwner::Player);
    s.players[0].resources = Resources{ 15, 15, 10 };
    session.LoadRaw(s);
    CHECK(session.TryBuild(FacilityType::MiningFacility, Pos{ 4, 3 }));
    CHECK(!session.TryBuild(FacilityType::ManaGenerator, Pos{ 3, 4 }));
    CHECK(session.TryUseAbility(AbilityType::ResourceMining, Pos{}));
    CHECK(!session.TryUseAbility(AbilityType::ResourceMining, Pos{}));
    CHECK(session.State().turn.Current() == CellOwner::Player);
    CHECK(session.TryPlace(Pos{ 3, 2 }));
    CHECK(session.State().turn.Current() == CellOwner::Enemy);
    CHECK(!session.State().actions.built);
    CHECK(!session.State().actions.abilityUsed);
}

TEST(F002_RandomStrategyGamesStayConsistent)
{
    std::mt19937 rng(777);
    int totalTurns = 0;
    for (int game = 0; game < 200; ++game)
    {
        GameConfig cfg;
        cfg.rule = RuleMode::Strategy;
        GameSession session;
        session.Start(cfg);
        int guard = 0;
        while (!session.IsOver() && guard++ < 400)
        {
            const GameState& st = session.State();
            // Random ability / build attempts (may be refused, which must change nothing).
            if (rng() % 2 == 0)
            {
                const auto type = static_cast<AbilityType>(rng() % Const::ABILITY_COUNT);
                const auto targets = AbilityManager::ValidTargets(st, type);
                const Pos t = targets.empty() ? Pos{ 0, 0 } : targets[rng() % targets.size()];
                session.TryUseAbility(type, t);
            }
            if (!session.IsOver() && rng() % 2 == 0)
            {
                const auto type = static_cast<FacilityType>(1 + rng() % Const::FACILITY_TYPE_COUNT);
                const auto cells = FacilityManager::BuildableCells(session.State(), type);
                if (!cells.empty()) session.TryBuild(type, cells[rng() % cells.size()]);
            }
            if (session.IsOver()) break;
            const auto moves = GameRules::LegalMoves(session.State());
            if (moves.empty())
            {
                CHECK(session.TryPass());
            }
            else
            {
                CHECK(session.TryPlace(moves[rng() % moves.size()]));
            }
            // Invariants
            const GameState& s2 = session.State();
            for (const Player& p : s2.players)
            {
                CHECK(p.resources.mana >= 0 && p.resources.mana <= Const::MANA_MAX);
                CHECK(p.resources.ore >= 0 && p.resources.ore <= Const::ORE_MAX);
                CHECK(p.resources.influence >= 0 && p.resources.influence <= Const::INFLUENCE_MAX);
                CHECK(p.abilityUses[AbilityIndex(AbilityType::ResourceMining)] <= Const::MINING_ABILITY_LIMIT);
                CHECK(p.abilityUses[AbilityIndex(AbilityType::TacticalPlacement)] <= Const::TACTICAL_PLACEMENT_LIMIT);
            }
            for (int y = 0; y < 8; ++y)
                for (int x = 0; x < 8; ++x)
                {
                    const Cell& c = s2.board.GetCell(x, y);
                    if (c.HasFacility()) CHECK(!c.IsEmpty());
                    if (c.isDefended) CHECK(!c.IsEmpty());
                }
        }
        CHECK(session.IsOver());
        totalTurns += session.State().turn.TurnNumber();
    }
    std::printf("    strategy random games: average %.1f turns\n", totalTurns / 200.0);
}

// User request 2026-11-10: random special-square layout.
TEST(F002_RandomLayoutRules)
{
    CHECK(MapGenerator::IsValid(MapGenerator::Standard()));
    int differentFromStandard = 0;
    int distinctPairs = 0;
    for (std::uint32_t seed = 1; seed <= 500; ++seed)
    {
        const SpecialLayout a = MapGenerator::Generate(MapLayout::Random, seed);
        CHECK(MapGenerator::IsValid(a));
        const SpecialLayout again = MapGenerator::Generate(MapLayout::Random, seed);
        bool same = true, standard = true, sameAsNext = true;
        const SpecialLayout next = MapGenerator::Generate(MapLayout::Random, seed + 1);
        for (size_t i = 0; i < a.size(); ++i)
        {
            same = same && a[i].pos == again[i].pos && a[i].type == again[i].type;
            standard = standard && a[i].pos == MapGenerator::Standard()[i].pos;
            sameAsNext = sameAsNext && a[i].pos == next[i].pos;
        }
        CHECK(same); // deterministic per seed (the AI and tests can reproduce a game)
        if (!standard) ++differentFromStandard;
        if (!sameAsNext) ++distinctPairs;
    }
    CHECK(differentFromStandard >= 495);
    CHECK(distinctPairs >= 495);

    // Setup uses the generated layout; Standard stays the fixed one.
    GameConfig cfg;
    cfg.rule = RuleMode::Strategy;
    cfg.layout = MapLayout::Random;
    cfg.mapSeed = 42;
    GameState s;
    GameRules::Setup(s, cfg);
    for (const auto& sc : MapGenerator::Generate(MapLayout::Random, 42))
        CHECK(s.board.GetCell(sc.pos).type == sc.type);
    cfg.layout = MapLayout::Standard;
    GameRules::Setup(s, cfg);
    CHECK(s.board.GetCell(2, 0).type == CellType::ManaMine);
}

TEST(F002_RandomLayoutGamesPlayable)
{
    std::mt19937 rng(99);
    for (std::uint32_t seed = 1; seed <= 100; ++seed)
    {
        GameConfig cfg;
        cfg.rule = RuleMode::Strategy;
        cfg.layout = MapLayout::Random;
        cfg.mapSeed = seed;
        GameSession session;
        session.Start(cfg);
        int guard = 0;
        while (!session.IsOver() && guard++ < 300)
        {
            const auto moves = GameRules::LegalMoves(session.State());
            if (moves.empty()) CHECK(session.TryPass()); else CHECK(session.TryPlace(moves[rng() % moves.size()]));
        }
        CHECK(session.IsOver());
    }
}
