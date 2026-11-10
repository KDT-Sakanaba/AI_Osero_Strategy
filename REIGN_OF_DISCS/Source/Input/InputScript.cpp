#include "Input/InputScript.h"
#include <fstream>
#include <sstream>
#include "DxLib.h"
#include "UI/UiConstants.h"

bool InputScript::Load(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        return false;
    }
    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }
        std::istringstream ss(line);
        Step step;
        ss >> step.op;
        if (step.op == "shot" || step.op == "key" || step.op == "audio")
        {
            ss >> step.text;
        }
        else
        {
            ss >> step.a >> step.b;
        }
        Expand(step);
    }
    ScriptFrame end;
    end.mouseX = mouseX_;
    end.mouseY = mouseY_;
    end.quit = true;
    frames_.push_back(end);
    active_ = true;
    return true;
}

void InputScript::Expand(const Step& step)
{
    auto idle = [this](int count) {
        for (int i = 0; i < count; ++i)
        {
            ScriptFrame f;
            f.mouseX = mouseX_;
            f.mouseY = mouseY_;
            frames_.push_back(f);
        }
    };
    auto press = [this, &idle](int buttons, bool esc) {
        for (int i = 0; i < 2; ++i)
        {
            ScriptFrame f;
            f.mouseX = mouseX_;
            f.mouseY = mouseY_;
            f.mouseButtons = buttons;
            f.escDown = esc;
            frames_.push_back(f);
        }
        idle(2);
    };

    if (step.op == "wait")
    {
        idle(step.a);
    }
    else if (step.op == "move")
    {
        mouseX_ = step.a;
        mouseY_ = step.b;
        idle(1);
    }
    else if (step.op == "click" || step.op == "rclick")
    {
        mouseX_ = step.a;
        mouseY_ = step.b;
        idle(1);
        press(step.op == "click" ? MOUSE_INPUT_LEFT : MOUSE_INPUT_RIGHT, false);
    }
    else if (step.op == "cell")
    {
        mouseX_ = Ui::BOARD_X + step.a * Ui::CELL + Ui::CELL / 2;
        mouseY_ = Ui::BOARD_Y + step.b * Ui::CELL + Ui::CELL / 2;
        idle(1);
        press(MOUSE_INPUT_LEFT, false);
    }
    else if (step.op == "key" && step.text == "esc")
    {
        press(0, true);
    }
    else if (step.op == "shot")
    {
        ScriptFrame f;
        f.mouseX = mouseX_;
        f.mouseY = mouseY_;
        f.shotName = step.text;
        frames_.push_back(f);
    }
    else if (step.op == "audio" || step.op == "drawresult")
    {
        ScriptFrame f;
        f.mouseX = mouseX_;
        f.mouseY = mouseY_;
        if (step.op == "audio") f.audioReport = step.text; else f.drawResult = true;
        frames_.push_back(f);
    }
    else if (step.op == "quit")
    {
        ScriptFrame f;
        f.quit = true;
        frames_.push_back(f);
    }
}

ScriptFrame InputScript::Next()
{
    if (index_ >= frames_.size())
    {
        ScriptFrame f;
        f.quit = true;
        return f;
    }
    return frames_[index_++];
}
