#pragma once
// Small drawing helpers and a clickable button.

#include <string>
#include "UI/UiConstants.h"

namespace DrawUtil
{
    unsigned int Color(const Ui::Rgb& rgb);
    void Text(int x, int y, const std::string& text, const Ui::Rgb& color, int font);
    void TextCentered(int centerX, int y, const std::string& text, const Ui::Rgb& color, int font);
    int TextWidth(const std::string& text, int font);
    void Panel(int x, int y, int w, int h);
    void FillRect(int x, int y, int w, int h, const Ui::Rgb& color);
    void FrameRect(int x, int y, int w, int h, const Ui::Rgb& color, int thickness = 1);
    void Overlay(int alpha);
}

struct Button
{
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    std::string label;
    bool enabled = true;
    bool selected = false;   // toggled state (mode selection)

    bool Contains(int mx, int my) const { return mx >= x && mx < x + w && my >= y && my < y + h; }
    bool Clicked(int mx, int my, bool leftClicked) const { return enabled && leftClicked && Contains(mx, my); }

    // pressed: mouse button held on the button (sinks the button, F004 R ボタン演出).
    void Draw(int font, int mouseX, int mouseY, bool pressed = false) const;
};
