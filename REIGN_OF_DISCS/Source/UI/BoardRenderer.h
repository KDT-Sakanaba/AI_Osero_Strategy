#pragma once
// Draws the board, stones, move hints and hover frame. Converts screen <-> board coordinates.

#include <vector>
#include "Board/Board.h"
#include "UI/UiConstants.h"

// Animated stone drawn instead of the real one (F004).
struct StoneOverride
{
    Pos pos{ -1, -1 };
    CellOwner owner = CellOwner::None;
    double widthScale = 1.0;
    double scale = 1.0;
};

// Extra things to show on top of the board state.
struct BoardView
{
    std::vector<StoneOverride> overrides;
    Pos highlight{ -1, -1 };             // just-placed square glow
    double highlightStrength = 0.0;      // 0..1
    std::vector<Pos> hints;              // legal-move markers
    CellOwner hintOwner = CellOwner::None;
    Pos hover{ -1, -1 };
    bool showSpecialCells = false;       // strategy only (F002 AC-01)
    std::vector<Pos> targets;            // selectable targets (strategy ability / build)
};

class BoardRenderer
{
public:
    static bool ScreenToCell(int mouseX, int mouseY, Pos& out);
    static int CellLeft(int x);
    static int CellTop(int y);
    static int CellCenterX(int x) { return CellLeft(x) + Ui::CELL / 2; }
    static int CellCenterY(int y) { return CellTop(y) + Ui::CELL / 2; }

    void Draw(const Board& board, const BoardView& view, int smallFont) const;

    // Draws one stone; widthScale 0..1 squeezes it horizontally (flip animation), scale enlarges (place).
    static void DrawStone(int centerX, int centerY, CellOwner owner, double widthScale = 1.0, double scale = 1.0);

private:
    void DrawCellBackground(const Cell& cell, int x, int y, bool showSpecial, int smallFont) const;
};
