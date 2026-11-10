#include "Systems/AbilityManager.h"
#include "Systems/FacilityManager.h"
#include "Systems/ResourceManager.h"

namespace
{
    const std::array<AbilityInfo, Const::ABILITY_COUNT> kAbilities =
    { {
        { AbilityType::MagicReinforce,    "魔力強化",   Resources{ 3, 0, 0 }, true,  0,
          "自分の石1つを、相手の次のターン終了まで反転されなくする" },
        { AbilityType::ForcedFlip,        "強制反転",   Resources{ 5, 0, 0 }, true,  0,
          "敵の石1つを自分の石にする（防御・強化中は不可）" },
        { AbilityType::ResourceMining,    "資源採掘",   Resources{ 0, 0, 0 }, false, Const::MINING_ABILITY_LIMIT,
          "鉱石+2（ゲーム中3回まで）" },
        { AbilityType::SiegeBreak,        "破城",       Resources{ 0, 0, 3 }, true,  0,
          "敵の施設1つの効果を、相手のターン2回分無効にする" },
        { AbilityType::TacticalPlacement, "戦術的配置", Resources{ 0, 0, 5 }, true,  Const::TACTICAL_PLACEMENT_LIMIT,
          "自分の石の隣の空きマスに石を追加で置く（反転なし・ゲーム中1回）" },
    } };

    bool HasOwnNeighbour(const Board& board, Pos p, CellOwner owner)
    {
        for (const auto& d : Const::DIRECTIONS)
        {
            const Cell* n = board.TryGetCell(p.x + d[0], p.y + d[1]);
            if (n != nullptr && n->owner == owner)
            {
                return true;
            }
        }
        return false;
    }
}

const std::array<AbilityInfo, Const::ABILITY_COUNT>& AbilityManager::All()
{
    return kAbilities;
}

const AbilityInfo& AbilityManager::Info(AbilityType type)
{
    return kAbilities[AbilityIndex(type)];
}

int AbilityManager::RemainingUses(const Player& player, AbilityType type)
{
    const AbilityInfo& info = Info(type);
    if (info.gameLimit == 0)
    {
        return -1;
    }
    return info.gameLimit - player.abilityUses[AbilityIndex(type)];
}

std::string AbilityManager::CheckUse(const GameState& state, AbilityType type)
{
    if (!state.IsStrategy() || state.over)
    {
        return "能力は使えません";
    }
    if (state.actions.abilityUsed)
    {
        return "このターンはすでに能力を使用しました";
    }
    const Player& player = state.Current();
    if (RemainingUses(player, type) == 0)
    {
        return "この能力は使用回数の上限に達しています";
    }
    std::string shortage = ResourceManager::Shortage(player, Info(type).cost);
    if (!shortage.empty())
    {
        return shortage;
    }
    if (type == AbilityType::ResourceMining && player.resources.ore >= Const::ORE_MAX)
    {
        return "鉱石がすでに最大です";
    }
    if (Info(type).needsTarget && ValidTargets(state, type).empty())
    {
        return "対象にできるマスがありません";
    }
    return "";
}

bool AbilityManager::IsValidTarget(const GameState& state, AbilityType type, Pos pos)
{
    const Cell* cell = state.board.TryGetCell(pos.x, pos.y);
    if (cell == nullptr)
    {
        return false;
    }
    const CellOwner me = state.turn.Current();
    switch (type)
    {
    case AbilityType::MagicReinforce:
        return cell->owner == me && !cell->IsEnhanced();
    case AbilityType::ForcedFlip:
        return cell->owner == Opponent(me) && !cell->IsProtected();
    case AbilityType::SiegeBreak:
        return cell->owner == Opponent(me) && cell->HasFacility() && !cell->IsFacilityDisabled();
    case AbilityType::TacticalPlacement:
        return cell->IsEmpty() && HasOwnNeighbour(state.board, pos, me);
    default:
        return false; // Resource Mining has no target
    }
}

std::vector<Pos> AbilityManager::ValidTargets(const GameState& state, AbilityType type)
{
    std::vector<Pos> targets;
    if (!Info(type).needsTarget)
    {
        return targets;
    }
    for (int y = 0; y < Board::SIZE; ++y)
    {
        for (int x = 0; x < Board::SIZE; ++x)
        {
            if (IsValidTarget(state, type, Pos{ x, y }))
            {
                targets.push_back(Pos{ x, y });
            }
        }
    }
    return targets;
}

bool AbilityManager::CanUseAny(const GameState& state)
{
    for (const auto& info : kAbilities)
    {
        if (CheckUse(state, info.type).empty())
        {
            return true;
        }
    }
    return false;
}

std::string AbilityManager::Use(GameState& state, AbilityType type, Pos target, AbilityResult* result)
{
    std::string reason = CheckUse(state, type);
    if (!reason.empty())
    {
        return reason;
    }
    if (Info(type).needsTarget && !IsValidTarget(state, type, target))
    {
        return "そのマスは対象にできません";
    }

    Player& player = state.Current();
    const CellOwner me = state.turn.Current();
    AbilityResult local;
    ResourceManager::Pay(player, Info(type).cost);

    switch (type)
    {
    case AbilityType::MagicReinforce:
        state.board.SetEnhancedUntil(target, 1); // until the opponent finishes its next turn
        break;
    case AbilityType::ForcedFlip:
        if (state.board.GetCell(target).facility == FacilityType::Fort)
        {
            state.actions.capturedFort = true; // taking a Fort gives an extra turn
        }
        state.board.SetOwner(target, me); // facility ownership follows the stone (R-12)
        local.changed.push_back(target);
        break;
    case AbilityType::ResourceMining:
        local.gained = ResourceManager::AddCapped(player, Resources{ 0, Const::MINING_ABILITY_ORE, 0 });
        break;
    case AbilityType::SiegeBreak:
        state.board.SetDisabledUntil(target, Const::SIEGE_BREAK_OWNER_TURNS);
        break;
    case AbilityType::TacticalPlacement:
        state.board.SetOwner(target, me); // placed without flipping (R-23)
        local.changed.push_back(target);
        break;
    }

    ++player.abilityUses[AbilityIndex(type)];
    ++player.abilityUsedCount;
    state.actions.abilityUsed = true;
    FacilityManager::RecomputeDefense(state.board);
    if (result != nullptr)
    {
        *result = local;
    }
    return "";
}
