#include "Core/Game.h"
#include <fstream>
#include <memory>
#include <sstream>
#include "DxLib.h"
#include "Scenes/ResultScene.h"
#include "Scenes/TitleScene.h"
#include "Systems/GameRules.h"
#include "UI/UiConstants.h"

namespace
{
    constexpr double SCRIPT_FRAME_SEC = 1.0 / 60.0; // fixed step in verification mode
    constexpr double MAX_FRAME_SEC = 0.1;
}

void Game::ParseCommandLine()
{
    std::istringstream args(GetCommandLineA());
    std::string token;
    while (args >> token)
    {
        if (token == "--script" && (args >> token))
        {
            scriptPath_ = token;
        }
        else if (token == "--shots" && (args >> token))
        {
            shotsDir_ = token;
        }
    }
}

bool Game::Initialize()
{
    ParseCommandLine();
    const bool scripted = !scriptPath_.empty();

    SetOutApplicationLogValidFlag(FALSE);
    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8); // source strings are UTF-8 (/utf-8)
    ChangeWindowMode(TRUE);
    SetGraphMode(Ui::SCREEN_W, Ui::SCREEN_H, Ui::COLOR_BIT);
    SetMainWindowText(Ui::WINDOW_TITLE);
    SetAlwaysRunFlag(TRUE);
    SetWaitVSyncFlag(scripted ? FALSE : TRUE);
    if (scripted)
    {
        SetWindowVisibleFlag(FALSE); // verification mode never shows a window or takes focus
    }

    if (DxLib_Init() == -1)
    {
        return false;
    }
    dxInitialized_ = true;
    SetDrawScreen(DX_SCREEN_BACK);

    if (!fonts_.Create())
    {
        return false;
    }
    if (scripted && !script_.Load(scriptPath_))
    {
        return false;
    }

    audio_.Load();
    audio_.ApplyVolumes(settings_);

    context_.scenes = &scenes_;
    context_.input = &input_;
    context_.fonts = &fonts_;
    context_.audio = &audio_;
    context_.settings = &settings_;
    scenes_.Change(std::make_unique<TitleScene>(context_));
    return true;
}

void Game::Run()
{
    LONGLONG previous = GetNowHiPerformanceCount();
    while (ProcessMessage() == 0 && !scenes_.QuitRequested())
    {
        const LONGLONG now = GetNowHiPerformanceCount();
        double delta = static_cast<double>(now - previous) / 1000000.0;
        previous = now;
        if (delta > MAX_FRAME_SEC)
        {
            delta = MAX_FRAME_SEC; // avoid huge steps after window drag etc.
        }

        ScriptFrame frame;
        if (script_.Active())
        {
            frame = script_.Next();
            if (frame.quit)
            {
                break;
            }
            input_.UpdateVirtual(frame.mouseX, frame.mouseY, frame.mouseButtons, frame.escDown);
            delta = SCRIPT_FRAME_SEC;
            if (!frame.audioReport.empty() && !shotsDir_.empty())
            {
                std::ofstream(shotsDir_ + "\\" + frame.audioReport + ".txt") << audio_.Report();
            }
            if (frame.drawResult)
            {
                // A finished 32-32 board, to show the draw result screen (verification only).
                GameState draw;
                GameRules::Setup(draw, GameConfig{});
                for (int y = 0; y < Board::SIZE; ++y)
                {
                    for (int x = 0; x < Board::SIZE; ++x)
                    {
                        draw.board.SetOwner(Pos{ x, y }, y < Board::SIZE / 2 ? CellOwner::Player : CellOwner::Enemy);
                    }
                }
                draw.over = true;
                scenes_.Change(std::make_unique<ResultScene>(context_, GameConfig{}, draw));
            }
        }
        else
        {
            input_.Update();
        }

        if (scenes_.ApplyPending())
        {
            input_.Flush(); // presses that caused the change must not reach the new scene
        }
        scenes_.Update(delta);

        ClearDrawScreen();
        scenes_.Draw();
        if (!frame.shotName.empty() && !shotsDir_.empty())
        {
            const std::string path = shotsDir_ + "\\" + frame.shotName + ".png";
            SaveDrawScreenToPNG(0, 0, Ui::SCREEN_W, Ui::SCREEN_H, path.c_str());
        }
        ScreenFlip();
    }
}

void Game::Finalize()
{
    scenes_.Clear();
    if (dxInitialized_)
    {
        audio_.Release();
        fonts_.Release();
        DxLib_End();
        dxInitialized_ = false;
    }
}
