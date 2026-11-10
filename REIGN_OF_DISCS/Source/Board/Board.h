#pragma once
// 8x8 board state. A plain value type (copyable) so AI search can copy it.
// All changes go through these functions; nobody touches the array directly.

#include <array>
#include "Board/Cell.h"
#include "Utils/Constants.h"

class Board
{
public:
    static constexpr int SIZE = Const::BOARD_SIZE;

    // Empty board with the standard four centre stones (F001 R-03).
    void Initialize();

    // Sets the special-square layout (F002 R-02; fixed or random, see MapGenerator). Strategy mode only.
    template <size_t Count>
    void ApplyStrategyLayout(const std::array<Const::SpecialCell, Count>& layout)
    {
        for (const auto& special : layout)
        {
            SetCellType(special.pos, special.type);
        }
    }

    static bool IsInside(int x, int y) { return x >= 0 && x < SIZE && y >= 0 && y < SIZE; }
    static bool IsInside(Pos p) { return IsInside(p.x, p.y); }

    // Precondition: IsInside. Out-of-range: assert in Debug, an empty dummy cell in Release.
    const Cell& GetCell(int x, int y) const;
    const Cell& GetCell(Pos p) const { return GetCell(p.x, p.y); }

    // nullptr when outside the board.
    const Cell* TryGetCell(int x, int y) const;

    // Mutators. Each ignores out-of-range positions (returns false).
    bool PlaceStone(int x, int y, CellOwner owner);
    bool SetOwner(Pos p, CellOwner owner);
    bool SetFacility(Pos p, FacilityType facility);
    bool SetDefended(Pos p, bool defended);
    bool SetEnhancedUntil(Pos p, int turn);
    bool SetDisabledUntil(Pos p, int turn);
    bool SetCellType(Pos p, CellType type);

    int CountStones(CellOwner owner) const;
    int CountEmpty() const;
    bool IsFull() const { return CountEmpty() == 0; }

private:
    Cell* MutableCell(Pos p);

    std::array<std::array<Cell, SIZE>, SIZE> cells_{}; // [y][x]
};
