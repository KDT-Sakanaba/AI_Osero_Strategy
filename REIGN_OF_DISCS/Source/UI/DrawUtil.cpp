#include "UI/DrawUtil.h"
#include "DxLib.h"

namespace DrawUtil
{
    unsigned int Color(const Ui::Rgb& rgb)
    {
        return GetColor(rgb.r, rgb.g, rgb.b);
    }

    void Text(int x, int y, const std::string& text, const Ui::Rgb& color, int font)
    {
        DrawStringToHandle(x, y, text.c_str(), Color(color), font);
    }

    int TextWidth(const std::string& text, int font)
    {
        return GetDrawStringWidthToHandle(text.c_str(), static_cast<int>(text.size()), font);
    }

    void TextCentered(int centerX, int y, const std::string& text, const Ui::Rgb& color, int font)
    {
        Text(centerX - TextWidth(text, font) / 2, y, text, color, font);
    }

    void FillRect(int x, int y, int w, int h, const Ui::Rgb& color)
    {
        DrawBox(x, y, x + w, y + h, Color(color), TRUE);
    }

    void FrameRect(int x, int y, int w, int h, const Ui::Rgb& color, int thickness)
    {
        for (int i = 0; i < thickness; ++i)
        {
            DrawBox(x + i, y + i, x + w - i, y + h - i, Color(color), FALSE);
        }
    }

    void Panel(int x, int y, int w, int h)
    {
        FillRect(x, y, w, h, Ui::PANEL_BG);
        FrameRect(x, y, w, h, Ui::PANEL_FRAME);
    }

    void Overlay(int alpha)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        FillRect(0, 0, Ui::SCREEN_W, Ui::SCREEN_H, Ui::OVERLAY);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

void Button::Draw(int font, int mouseX, int mouseY, bool pressed) const
{
    const bool hovered = enabled && Contains(mouseX, mouseY);
    const int offset = (hovered && pressed) ? 2 : 0;
    Ui::Rgb fill = Ui::BUTTON;
    if (!enabled)
    {
        fill = Ui::BUTTON_DISABLED;
    }
    else if (selected)
    {
        fill = Ui::BUTTON_SELECTED;
    }
    else if (hovered)
    {
        fill = Ui::BUTTON_HOVER;
    }
    DrawUtil::FillRect(x + offset, y + offset, w, h, fill);
    DrawUtil::FrameRect(x + offset, y + offset, w, h, hovered ? Ui::HOVER_FRAME : Ui::PANEL_FRAME);
    const int textH = GetFontSizeToHandle(font);
    DrawUtil::TextCentered(x + w / 2 + offset, y + (h - textH) / 2 + offset, label,
        enabled ? Ui::TEXT : Ui::TEXT_DIM, font);
}
