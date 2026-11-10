#pragma once
// Legal-move search and flip-target collection (original 12). Stateless.
// The flip list returned by FindFlips is exactly what the placement applies (original 12.3).

#include <vector>
#include "Board/Board.h"

// Rule differences between modes, passed in from outside.
// Classic mode always uses the default (no protection) so its results never change (F002 R-28).
struct FlipRule
{
    // Strategy mode: protected stones (defended / enhanced) may sit inside a sandwiched line
    // but are not flipped; a direction counts only if at least one stone would flip (F002 R-16).
    bool respectProtection = false;
};

struct FlipList
{
    std::array<Pos, Const::CELL_COUNT> items{};
    int count = 0;

    void Add(Pos p) { if (count < static_cast<int>(items.size())) { items[count++] = p; } }
    bool Empty() const { return count == 0; }
    const Pos* begin() const { return items.data(); }
    const Pos* end() const { return items.data() + count; }
};

class MoveValidator
{
public:
    // Stones that would flip if 'owner' placed at 'pos'. Empty = illegal move.
    static FlipList FindFlips(const Board& board, Pos pos, CellOwner owner, const FlipRule& rule);

    static bool IsLegalMove(const Board& board, Pos pos, CellOwner owner, const FlipRule& rule);
    static std::vector<Pos> GetLegalMoves(const Board& board, CellOwner owner, const FlipRule& rule);
    static int CountLegalMoves(const Board& board, CellOwner owner, const FlipRule& rule);
    static bool HasLegalMove(const Board& board, CellOwner owner, const FlipRule& rule);
};
