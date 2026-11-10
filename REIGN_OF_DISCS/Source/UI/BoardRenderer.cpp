#include "UI/BoardRenderer.h"
#include "DxLib.h"
#include "UI/DrawUtil.h"
#include "UI/UiConstants.h"

namespace
{
    const char* CellMark(CellType type)
    {
        switch (type)
        {
        case CellType::ManaMine:    return "マナ";
        case CellType::OreMine:     return "鉱石";
        case CellType::Altar:       return "祭壇";
        case CellType::NeutralBase: return "拠点";
        default:                    return "";
        }
    }

    Ui::Rgb CellColor(CellType type)
    {
        switch (type)
        {
        case CellType::ManaMine:    return Ui::MANA_CELL;
        case CellType::OreMine:     return Ui::ORE_CELL;
        case CellType::Altar:       return Ui::ALTAR_CELL;
        case CellType::NeutralBase: return Ui::BASE_CELL;
        default:                    return Ui::BOARD_BG;
        }
    }

    const char* FacilityMark(FacilityType type)
    {
        switch (type)
        {
        case FacilityType::MiningFacility: return "採";
        case FacilityType::ManaGenerator:  return "炉";
        case FacilityType::Fort:           return "砦";
        case FacilityType::Altar:          return "壇";
        default:                           return "";
        }
    }
}

bool BoardRenderer::ScreenToCell(int mouseX, int mouseY, Pos& out)
{
    if (mouseX < Ui::BOARD_X || mouseY < Ui::BOARD_Y)
    {
        return false;
    }
    const int x = (mouseX - Ui::BOARD_X) / Ui::CELL;
    const int y = (mouseY - Ui::BOARD_Y) / Ui::CELL;
    if (!Board::IsInside(x, y))
    {
        return false;
    }
    out = Pos{ x, y };
    return true;
}

int BoardRenderer::CellLeft(int x)
{
    return Ui::BOARD_X + x * Ui::CELL;
}

int BoardRenderer::CellTop(int y)
{
    return Ui::BOARD_Y + y * Ui::CELL;
}

void BoardRenderer::DrawStone(int centerX, int centerY, CellOwner owner, double widthScale, double scale)
{
    if (owner == CellOwner::None)
    {
        return;
    }
    const double r = Ui::STONE_RADIUS * scale;
    const float rx = static_cast<float>(r * (widthScale < 0.05 ? 0.05 : widthScale));
    const float ry = static_cast<float>(r);
    const Ui::Rgb fill = owner == CellOwner::Player ? Ui::BLACK_STONE : Ui::WHITE_STONE;
    DrawOvalAA(static_cast<float>(centerX), static_cast<float>(centerY), rx, ry, 48, DrawUtil::Color(fill), TRUE);
    DrawOvalAA(static_cast<float>(centerX), static_cast<float>(centerY), rx, ry, 48, DrawUtil::Color(Ui::STONE_EDGE), FALSE, 1.5f);
}

void BoardRenderer::DrawCellBackground(const Cell& cell, int x, int y, bool showSpecial, int smallFont) const
{
    const int left = CellLeft(x);
    const int top = CellTop(y);
    const Ui::Rgb color = showSpecial ? CellColor(cell.type) : Ui::BOARD_BG;
    DrawUtil::FillRect(left, top, Ui::CELL, Ui::CELL, color);
    if (showSpecial && cell.type != CellType::Normal)
    {
        DrawUtil::Text(left + 4, top + 2, CellMark(cell.type), Ui::TEXT, smallFont);
    }
}

void BoardRenderer::Draw(const Board& board, const BoardView& view, int smallFont) const
{
    for (int y = 0; y < Board::SIZE; ++y)
    {
        for (int x = 0; x < Board::SIZE; ++x)
        {
            DrawCellBackground(board.GetCell(x, y), x, y, view.showSpecialCells, smallFont);
        }
    }
    for (int i = 0; i <= Board::SIZE; ++i)
    {
        const unsigned int grid = DrawUtil::Color(Ui::GRID);
        DrawLine(Ui::BOARD_X + i * Ui::CELL, Ui::BOARD_Y, Ui::BOARD_X + i * Ui::CELL, Ui::BOARD_Y + Ui::BOARD_PIXELS, grid, 2);
        DrawLine(Ui::BOARD_X, Ui::BOARD_Y + i * Ui::CELL, Ui::BOARD_X + Ui::BOARD_PIXELS, Ui::BOARD_Y + i * Ui::CELL, grid, 2);
    }

    if (Board::IsInside(view.highlight) && view.highlightStrength > 0.0)
    {
        SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(140 * view.highlightStrength));
        DrawUtil::FillRect(CellLeft(view.highlight.x), CellTop(view.highlight.y), Ui::CELL, Ui::CELL, Ui::HOVER_FRAME);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    for (int y = 0; y < Board::SIZE; ++y)
    {
        for (int x = 0; x < Board::SIZE; ++x)
        {
            const Cell& cell = board.GetCell(x, y);
            const StoneOverride* over = nullptr;
            for (const StoneOverride& o : view.overrides)
            {
                if (o.pos == Pos{ x, y }) over = &o; // the latest animation for this square wins
            }
            if (over != nullptr)
            {
                DrawStone(CellCenterX(x), CellCenterY(y), over->owner, over->widthScale, over->scale);
            }
            else
            {
                DrawStone(CellCenterX(x), CellCenterY(y), cell.owner);
            }
            if (cell.HasFacility())
            {
                // Facility: a square badge in the lower-right corner, distinct from stones.
                const int bx = CellLeft(x) + Ui::CELL - 26;
                const int by = CellTop(y) + Ui::CELL - 26;
                const Ui::Rgb badge = cell.IsFacilityDisabled() ? Ui::BUTTON_DISABLED : Ui::TEXT_ACCENT;
                DrawUtil::FillRect(bx, by, 22, 22, badge);
                DrawUtil::Text(bx + 3, by + 2, FacilityMark(cell.facility), Ui::BLACK_STONE, smallFont);
            }
            if (cell.isDefended)
            {
                DrawUtil::FrameRect(CellLeft(x) + 3, CellTop(y) + 3, Ui::CELL - 6, Ui::CELL - 6, Ui::DEFENSE_MARK, 3);
            }
            if (cell.IsEnhanced())
            {
                DrawCircleAA(static_cast<float>(CellCenterX(x)), static_cast<float>(CellCenterY(y)),
                    static_cast<float>(Ui::STONE_RADIUS + 3), 48, DrawUtil::Color(Ui::ENHANCE_MARK), FALSE, 3.0f);
            }
        }
    }

    if (view.hintOwner != CellOwner::None)
    {
        const Ui::Rgb hint = view.hintOwner == CellOwner::Player ? Ui::BLACK_STONE : Ui::WHITE_STONE;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
        for (const Pos& p : view.hints)
        {
            DrawCircleAA(static_cast<float>(CellCenterX(p.x)), static_cast<float>(CellCenterY(p.y)),
                static_cast<float>(Ui::HINT_RADIUS), 24, DrawUtil::Color(hint), TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    for (const Pos& p : view.targets)
    {
        DrawUtil::FrameRect(CellLeft(p.x) + 1, CellTop(p.y) + 1, Ui::CELL - 2, Ui::CELL - 2, Ui::TARGET_MARK, 2);
    }

    if (Board::IsInside(view.hover))
    {
        DrawUtil::FrameRect(CellLeft(view.hover.x), CellTop(view.hover.y), Ui::CELL, Ui::CELL, Ui::HOVER_FRAME, 3);
    }
}
