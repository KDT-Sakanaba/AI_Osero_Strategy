#include "Systems/FacilityManager.h"
#include "Systems/ResourceManager.h"

namespace
{
    const std::array<FacilityInfo, Const::FACILITY_TYPE_COUNT> kFacilities =
    { {
        { FacilityType::MiningFacility, "採掘所",       Resources{ 0, 3, 0 }, "ターン開始時に鉱石+1" },
        { FacilityType::ManaGenerator,  "魔力炉",       Resources{ 2, 2, 0 }, "ターン開始時にマナ+1" },
        { FacilityType::Fort,           "砦",           Resources{ 0, 4, 0 }, "相手の次の1ターンだけ上下左右の石を防御（取られると相手が追加ターン）" },
        { FacilityType::Altar,          "祭壇（施設）", Resources{ 0, 3, 1 }, "ターン開始時に影響力+1" },
    } };
}

const std::array<FacilityInfo, Const::FACILITY_TYPE_COUNT>& FacilityManager::All()
{
    return kFacilities;
}

const FacilityInfo& FacilityManager::Info(FacilityType type)
{
    for (const auto& info : kFacilities)
    {
        if (info.type == type)
        {
            return info;
        }
    }
    return kFacilities[0];
}

std::string FacilityManager::CheckTurn(const GameState& state)
{
    if (!state.IsStrategy() || state.over)
    {
        return "建設できません";
    }
    if (state.actions.built)
    {
        return "このターンはすでに建設しました";
    }
    return "";
}

std::string FacilityManager::CheckBuild(const GameState& state, Pos pos, FacilityType type)
{
    std::string reason = CheckTurn(state);
    if (!reason.empty())
    {
        return reason;
    }
    if (type == FacilityType::None)
    {
        return "建設する施設を選んでください";
    }
    const Cell* cell = state.board.TryGetCell(pos.x, pos.y);
    if (cell == nullptr || cell->owner != state.turn.Current() || cell->HasFacility())
    {
        return "このマスには建設できません";
    }
    return ResourceManager::Shortage(state.Current(), Info(type).cost);
}

std::vector<Pos> FacilityManager::BuildableCells(const GameState& state, FacilityType type)
{
    std::vector<Pos> cells;
    for (int y = 0; y < Board::SIZE; ++y)
    {
        for (int x = 0; x < Board::SIZE; ++x)
        {
            if (CheckBuild(state, Pos{ x, y }, type).empty())
            {
                cells.push_back(Pos{ x, y });
            }
        }
    }
    return cells;
}

bool FacilityManager::CanBuildAnything(const GameState& state)
{
    if (!CheckTurn(state).empty())
    {
        return false;
    }
    for (const auto& info : kFacilities)
    {
        if (!ResourceManager::CanAfford(state.Current(), info.cost))
        {
            continue;
        }
        for (int y = 0; y < Board::SIZE; ++y)
        {
            for (int x = 0; x < Board::SIZE; ++x)
            {
                const Cell& c = state.board.GetCell(x, y);
                if (c.owner == state.turn.Current() && !c.HasFacility())
                {
                    return true;
                }
            }
        }
    }
    return false;
}

std::string FacilityManager::Build(GameState& state, Pos pos, FacilityType type)
{
    std::string reason = CheckBuild(state, pos, type);
    if (!reason.empty())
    {
        return reason;
    }
    Player& player = state.Current();
    ResourceManager::Pay(player, Info(type).cost);
    state.board.SetFacility(pos, type);
    state.board.SetDisabledUntil(pos, 0);
    ++player.facilityCount;
    state.actions.built = true;
    RecomputeDefense(state.board);
    return "";
}

void FacilityManager::RecomputeDefense(Board& board)
{
    for (int y = 0; y < Board::SIZE; ++y)
    {
        for (int x = 0; x < Board::SIZE; ++x)
        {
            board.SetDefended(Pos{ x, y }, false);
        }
    }
    for (int y = 0; y < Board::SIZE; ++y)
    {
        for (int x = 0; x < Board::SIZE; ++x)
        {
            const Cell& fort = board.GetCell(x, y);
            if (fort.facility != FacilityType::Fort || fort.IsEmpty() || fort.IsFacilityDisabled())
            {
                continue;
            }
            for (const auto& d : Const::ORTHOGONAL)
            {
                const Cell* n = board.TryGetCell(x + d[0], y + d[1]);
                if (n != nullptr && n->owner == fort.owner)
                {
                    board.SetDefended(Pos{ x + d[0], y + d[1] }, true);
                }
            }
        }
    }
}
