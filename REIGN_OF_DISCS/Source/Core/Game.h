#pragma once
// Application: DxLib start-up, main loop, shutdown (original 10.3 Game).

#include <string>
#include "Core/AudioManager.h"
#include "Core/SceneManager.h"
#include "Core/Settings.h"
#include "Input/InputManager.h"
#include "Input/InputScript.h"
#include "UI/FontSet.h"

class Game
{
public:
    bool Initialize();
    void Run();
    void Finalize();

private:
    void ParseCommandLine();

    SceneManager scenes_;
    InputManager input_;
    FontSet fonts_;
    AudioManager audio_;
    Settings settings_;
    AppContext context_;
    bool dxInitialized_ = false;

    // Verification mode (--script / --shots)
    InputScript script_;
    std::string scriptPath_;
    std::string shotsDir_;
};
