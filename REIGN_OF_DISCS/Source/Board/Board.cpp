#include "Board/Board.h"
#include <cassert>

namespace
{
    const Cell kEmptyCell{};
}

void Board::Initialize()
{
    for (auto& row : cells_)
    {
        for (auto& cell : row)
        {
            cell = Cell{};
        }
    }
    for (const auto& stone : Const::INITIAL_STONES)
    {
        PlaceStone(stone.pos.x, stone.pos.y, stone.owner);
    }
}

const Cell& Board::GetCell(int x, int y) const
{
    assert(IsInside(x, y));
    if (!IsInside(x, y))
    {
        return kEmptyCell;
    }
    return cells_[y][x];
}

const Cell* Board::TryGetCell(int x, int y) const
{
    return IsInside(x, y) ? &cells_[y][x] : nullptr;
}

Cell* Board::MutableCell(Pos p)
{
    return IsInside(p) ? &cells_[p.y][p.x] : nullptr;
}

bool Board::PlaceStone(int x, int y, CellOwner owner)
{
    return SetOwner(Pos{ x, y }, owner);
}

bool Board::SetOwner(Pos p, CellOwner owner)
{
    Cell* cell = MutableCell(p);
    if (cell == nullptr)
    {
        return false;
    }
    cell->owner = owner;
    return true;
}

bool Board::SetFacility(Pos p, FacilityType facility)
{
    Cell* cell = MutableCell(p);
    if (cell == nullptr)
    {
        return false;
    }
    cell->facility = facility;
    return true;
}

bool Board::SetDefended(Pos p, bool defended)
{
    Cell* cell = MutableCell(p);
    if (cell == nullptr)
    {
        return false;
    }
    cell->isDefended = defended;
    return true;
}

bool Board::SetEnhancedUntil(Pos p, int turn)
{
    Cell* cell = MutableCell(p);
    if (cell == nullptr)
    {
        return false;
    }
    cell->enhancedUntil = turn;
    return true;
}

bool Board::SetDisabledUntil(Pos p, int turn)
{
    Cell* cell = MutableCell(p);
    if (cell == nullptr)
    {
        return false;
    }
    cell->disabledUntil = turn;
    return true;
}

bool Board::SetCellType(Pos p, CellType type)
{
    Cell* cell = MutableCell(p);
    if (cell == nullptr)
    {
        return false;
    }
    cell->type = type;
    return true;
}

int Board::CountStones(CellOwner owner) const
{
    int count = 0;
    for (const auto& row : cells_)
    {
        for (const auto& cell : row)
        {
            if (cell.owner == owner)
            {
                ++count;
            }
        }
    }
    return count;
}

int Board::CountEmpty() const
{
    return CountStones(CellOwner::None);
}
