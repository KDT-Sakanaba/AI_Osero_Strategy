#pragma once
// Mouse and keyboard state, with "pressed this frame" detection (F001 R-09).

#include <array>

class InputManager
{
public:
    // Reads the device state once per frame.
    void Update();

    // Verification mode: takes the frame's state from a script instead of the devices.
    void UpdateVirtual(int mouseX, int mouseY, int mouseButtons, bool escDown);

    bool LeftDown() const { return (mouse_ & 1) != 0; } // MOUSE_INPUT_LEFT

    // Discards this frame's presses (used right after a scene change, original 15).
    void Flush();

    int MouseX() const { return mouseX_; }
    int MouseY() const { return mouseY_; }
    bool LeftClicked() const;
    bool RightClicked() const;
    bool KeyPressed(int keyCode) const;   // KEY_INPUT_* code

private:
    int mouseX_ = 0;
    int mouseY_ = 0;
    int mouse_ = 0;
    int prevMouse_ = 0;
    std::array<char, 256> keys_{};
    std::array<char, 256> prevKeys_{};
};
