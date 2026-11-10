#pragma once
// In-game help: rules, legend of board marks, resources and abilities, controls.
// Opened from the game screen (button / H key), the pause menu and the title ("遊び方").

#include "UI/DrawUtil.h"

class FontSet;
class InputManager;

class HelpOverlay
{
public:
    HelpOverlay();

    void Open(int page = 0);
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    // Returns true when a button was pressed (for the click sound).
    bool Update(const InputManager& input);
    void Draw(const FontSet& fonts, int mouseX, int mouseY, bool pressed) const;

    static constexpr int PAGE_COUNT = 4;

private:
    void DrawPage(const FontSet& fonts) const;

    bool open_ = false;
    int page_ = 0;
    Button prev_;
    Button next_;
    Button close_;
    Button tabs_[PAGE_COUNT];
};
