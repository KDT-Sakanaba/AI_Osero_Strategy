#include "Input/InputManager.h"
#include "DxLib.h"

void InputManager::Update()
{
    prevMouse_ = mouse_;
    prevKeys_ = keys_;
    mouse_ = GetMouseInput();
    GetMousePoint(&mouseX_, &mouseY_);
    GetHitKeyStateAll(keys_.data());
}

void InputManager::UpdateVirtual(int mouseX, int mouseY, int mouseButtons, bool escDown)
{
    prevMouse_ = mouse_;
    prevKeys_ = keys_;
    mouse_ = mouseButtons;
    mouseX_ = mouseX;
    mouseY_ = mouseY;
    keys_.fill(0);
    keys_[KEY_INPUT_ESCAPE] = escDown ? 1 : 0;
}

void InputManager::Flush()
{
    prevMouse_ = mouse_;
    prevKeys_ = keys_;
}

bool InputManager::LeftClicked() const
{
    return (mouse_ & MOUSE_INPUT_LEFT) != 0 && (prevMouse_ & MOUSE_INPUT_LEFT) == 0;
}

bool InputManager::RightClicked() const
{
    return (mouse_ & MOUSE_INPUT_RIGHT) != 0 && (prevMouse_ & MOUSE_INPUT_RIGHT) == 0;
}

bool InputManager::KeyPressed(int keyCode) const
{
    if (keyCode < 0 || keyCode >= static_cast<int>(keys_.size()))
    {
        return false;
    }
    return keys_[keyCode] != 0 && prevKeys_[keyCode] == 0;
}
