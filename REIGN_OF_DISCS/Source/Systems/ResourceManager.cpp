#include "Systems/ResourceManager.h"
#include <algorithm>

namespace
{
    int AddOne(int& value, int delta, int max)
    {
        const int before = value;
        value = std::min(max, value + delta);
        return value - before;
    }

    void Accumulate(Resources& total, const Resources& add)
    {
        total.mana += add.mana;
        total.ore += add.ore;
        total.influence += add.influence;
    }
}

Resources ResourceManager::AddCapped(Player& player, const Resources& delta)
{
    Resources added;
    added.mana = AddOne(player.resources.mana, delta.mana, Const::MANA_MAX);
    added.ore = AddOne(player.resources.ore, delta.ore, Const::ORE_MAX);
    added.influence = AddOne(player.resources.influence, delta.influence, Const::INFLUENCE_MAX);
    return added;
}

bool ResourceManager::Pay(Player& player, const Resources& cost)
{
    if (!CanAfford(player, cost))
    {
        return false;
    }
    player.resources.mana -= cost.mana;
    player.resources.ore -= cost.ore;
    player.resources.influence -= cost.influence;
    return true;
}

std::string ResourceManager::Shortage(const Player& player, const Resources& cost)
{
    if (player.resources.mana < cost.mana) return "マナが不足しています";
    if (player.resources.ore < cost.ore) return "鉱石が不足しています";
    if (player.resources.influence < cost.influence) return "影響力が不足しています";
    return "";
}

Resources ResourceManager::CellIncome(const Cell& cell)
{
    Resources income;
    switch (cell.type)
    {
    case CellType::ManaMine:    income.mana += 2; break;
    case CellType::OreMine:     income.ore += 2; break;
    case CellType::Altar:       income.mana += 1; income.influence += 1; break;
    case CellType::NeutralBase: income.influence += 1; break;
    default: break;
    }
    if (!cell.IsFacilityDisabled())
    {
        switch (cell.facility)
        {
        case FacilityType::MiningFacility: income.ore += 1; break;
        case FacilityType::ManaGenerator:  income.mana += 1; break;
        case FacilityType::Altar:          income.influence += 1; break;
        default: break; // Fort gives defence, not resources
        }
    }
    return income;
}

Resources ResourceManager::TurnIncome(const GameState& state, CellOwner side)
{
    Resources total;
    for (int y = 0; y < Board::SIZE; ++y)
    {
        for (int x = 0; x < Board::SIZE; ++x)
        {
            const Cell& cell = state.board.GetCell(x, y);
            if (cell.owner == side)
            {
                Accumulate(total, CellIncome(cell));
            }
        }
    }
    return total;
}
