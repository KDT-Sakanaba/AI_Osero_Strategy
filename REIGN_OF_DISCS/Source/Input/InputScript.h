#pragma once
// Verification mode: replays scripted mouse / key input inside the game and saves screenshots,
// without touching the OS input (started with: REIGN_OF_DISCS.exe --script <file> --shots <dir>).
//
// Script lines (one command per line, '#' = comment):
//   wait <frames>        idle
//   move <x> <y>         move the virtual mouse (client coordinates)
//   click <x> <y>        left click (press 2 frames, release 2 frames)
//   rclick <x> <y>       right click
//   cell <cx> <cy>       left click on a board square
//   key esc              press ESC
//   shot <name>          save the current frame as <dir>/<name>.png
//   audio <name>         write the audio state (loaded files, BGM playing) to <dir>/<name>.txt
//   drawresult           open the result screen with a 32-32 draw (verification of the draw screen)
//   quit                 end the program

#include <string>
#include <vector>

struct ScriptFrame
{
    int mouseX = 0;
    int mouseY = 0;
    int mouseButtons = 0;   // MOUSE_INPUT_* bits
    bool escDown = false;
    std::string shotName;   // non-empty: capture this frame
    std::string audioReport; // non-empty: write the audio state
    bool drawResult = false;
    bool quit = false;
};

class InputScript
{
public:
    bool Load(const std::string& path);
    bool Active() const { return active_; }

    // Produces the virtual input of the next frame.
    ScriptFrame Next();

private:
    struct Step
    {
        std::string op;
        int a = 0;
        int b = 0;
        std::string text;
    };

    void Expand(const Step& step);

    std::vector<ScriptFrame> frames_;
    size_t index_ = 0;
    int mouseX_ = 0;
    int mouseY_ = 0;
    bool active_ = false;
};
