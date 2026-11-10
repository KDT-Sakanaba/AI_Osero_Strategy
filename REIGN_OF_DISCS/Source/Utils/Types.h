#pragma once
// Basic shared types of the logic layer. Must not depend on DxLib.

#include <cstdint>

enum class CellOwner : std::uint8_t
{
    None,
    Player,  // black, moves first
    Enemy    // white, moves second
};

inline CellOwner Opponent(CellOwner owner)
{
    switch (owner)
    {
    case CellOwner::Player: return CellOwner::Enemy;
    case CellOwner::Enemy:  return CellOwner::Player;
    default:                return CellOwner::None;
    }
}

// Index into per-side arrays (0 = Player, 1 = Enemy). Owner must not be None.
inline int SideIndex(CellOwner owner)
{
    return owner == CellOwner::Enemy ? 1 : 0;
}

inline CellOwner SideOwner(int index)
{
    return index == 1 ? CellOwner::Enemy : CellOwner::Player;
}

enum class CellType : std::uint8_t
{
    Normal,
    ManaMine,
    OreMine,
    Altar,
    NeutralBase
};

enum class FacilityType : std::uint8_t
{
    None,
    MiningFacility,
    ManaGenerator,
    Fort,
    Altar
};

// Special abilities (original 4.2). Values index Player::abilityUses.
enum class AbilityType : std::uint8_t
{
    MagicReinforce,     // 魔力強化
    ForcedFlip,         // 強制反転
    ResourceMining,     // 資源採掘
    SiegeBreak,         // 破城
    TacticalPlacement   // 戦術的配置
};

inline int AbilityIndex(AbilityType type) { return static_cast<int>(type); }

// Terrain is stored as data only. Effects other than Plain are out of scope (F002 R-04).
enum class TerrainType : std::uint8_t
{
    Plain
};

struct Pos
{
    int x = 0;
    int y = 0;

    bool operator==(const Pos& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Pos& other) const { return !(*this == other); }
};

enum class RuleMode
{
    Classic,
    Strategy
};

enum class OpponentType
{
    Local,
    AiEasy,
    AiNormal,
    AiHard
};

inline bool IsAiOpponent(OpponentType type)
{
    return type != OpponentType::Local;
}

// Special-square layout of the strategy mode.
enum class MapLayout
{
    Standard,   // the fixed layout (F002 R-02)
    Random      // random point-symmetric layout from mapSeed (user request 2026-11-10)
};

struct GameConfig
{
    RuleMode rule = RuleMode::Classic;
    OpponentType opponent = OpponentType::Local;
    MapLayout layout = MapLayout::Standard;
    std::uint32_t mapSeed = 0;   // used when layout == Random
};
