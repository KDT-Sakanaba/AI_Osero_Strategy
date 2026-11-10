#include "Board/MoveValidator.h"

FlipList MoveValidator::FindFlips(const Board& board, Pos pos, CellOwner owner, const FlipRule& rule)
{
    FlipList result;
    if (owner == CellOwner::None || !Board::IsInside(pos) || !board.GetCell(pos).IsEmpty())
    {
        return result;
    }

    const CellOwner enemy = Opponent(owner);
    for (const auto& dir : Const::DIRECTIONS)
    {
        FlipList line;
        int x = pos.x + dir[0];
        int y = pos.y + dir[1];
        bool closed = false;

        while (Board::IsInside(x, y))
        {
            const Cell& cell = board.GetCell(x, y);
            if (cell.owner == enemy)
            {
                if (!(rule.respectProtection && cell.IsProtected()))
                {
                    line.Add(Pos{ x, y });
                }
                x += dir[0];
                y += dir[1];
                continue;
            }
            closed = (cell.owner == owner); // own stone closes the line; empty breaks it
            break;
        }

        if (closed && !line.Empty())
        {
            for (const Pos& p : line)
            {
                result.Add(p);
            }
        }
    }
    return result;
}

bool MoveValidator::IsLegalMove(const Board& board, Pos pos, CellOwner owner, const FlipRule& rule)
{
    return !FindFlips(board, pos, owner, rule).Empty();
}

std::vector<Pos> MoveValidator::GetLegalMoves(const Board& board, CellOwner owner, const FlipRule& rule)
{
    std::vector<Pos> moves;
    for (int y = 0; y < Board::SIZE; ++y)
    {
        for (int x = 0; x < Board::SIZE; ++x)
        {
            if (IsLegalMove(board, Pos{ x, y }, owner, rule))
            {
                moves.push_back(Pos{ x, y });
            }
        }
    }
    return moves;
}

int MoveValidator::CountLegalMoves(const Board& board, CellOwner owner, const FlipRule& rule)
{
    int count = 0;
    for (int y = 0; y < Board::SIZE; ++y)
    {
        for (int x = 0; x < Board::SIZE; ++x)
        {
            if (IsLegalMove(board, Pos{ x, y }, owner, rule))
            {
                ++count;
            }
        }
    }
    return count;
}

bool MoveValidator::HasLegalMove(const Board& board, CellOwner owner, const FlipRule& rule)
{
    for (int y = 0; y < Board::SIZE; ++y)
    {
        for (int x = 0; x < Board::SIZE; ++x)
        {
            if (IsLegalMove(board, Pos{ x, y }, owner, rule))
            {
                return true;
            }
        }
    }
    return false;
}
