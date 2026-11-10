#include "Scenes/TitleScene.h"
#include <memory>
#include "Core/Features.h"
#include "Input/InputManager.h"
#include "Scenes/SelectScene.h"
#include "Scenes/SettingsScene.h"
#include "UI/FontSet.h"

namespace
{
    constexpr int BUTTON_W = 240;
    constexpr int BUTTON_H = 56;
    constexpr int BUTTON_X = (Ui::SCREEN_W - BUTTON_W) / 2;
}

TitleScene::TitleScene(AppContext& context) : SceneBase(context)
{
    start_ = Button{ BUTTON_X, 340, BUTTON_W, BUTTON_H, "ゲーム開始" };
    howTo_ = Button{ BUTTON_X, 412, BUTTON_W, BUTTON_H, "遊び方" };
    settings_ = Button{ BUTTON_X, 484, BUTTON_W, BUTTON_H, "設定" };
    settings_.enabled = Features::SETTINGS_SCREEN;
    quit_ = Button{ BUTTON_X, 556, BUTTON_W, BUTTON_H, "終了" };
}

void TitleScene::OnEnter()
{
    PlayBgm(BgmId::Title);
}

void TitleScene::Update(double)
{
    const InputManager& in = *ctx_.input;
    const int mx = in.MouseX();
    const int my = in.MouseY();
    const bool click = in.LeftClicked();

    if (help_.IsOpen())
    {
        if (help_.Update(in)) PlaySe(SoundId::Button);
        return;
    }
    if (howTo_.Clicked(mx, my, click))
    {
        PlaySe(SoundId::Button);
        help_.Open(0);
        return;
    }
    if (start_.Clicked(mx, my, click))
    {
        PlaySe(SoundId::Button);
        ctx_.scenes->Change(std::make_unique<SelectScene>(ctx_));
    }
    else if (settings_.Clicked(mx, my, click))
    {
        PlaySe(SoundId::Button);
        ctx_.scenes->Change(std::make_unique<SettingsScene>(ctx_));
    }
    else if (quit_.Clicked(mx, my, click))
    {
        ctx_.scenes->RequestQuit();
    }
}

void TitleScene::Draw() const
{
    DrawUtil::FillRect(0, 0, Ui::SCREEN_W, Ui::SCREEN_H, Ui::BG);
    const FontSet& f = *ctx_.fonts;
    DrawUtil::TextCentered(Ui::SCREEN_W / 2, 130, "REIGN OF DISCS", Ui::TEXT_ACCENT, f.Title());
    DrawUtil::TextCentered(Ui::SCREEN_W / 2, 220, "盤上の覇権 ― オセロ × 領土戦略", Ui::TEXT, f.Large());

    const int mx = ctx_.input->MouseX();
    const int my = ctx_.input->MouseY();
    const bool pressed = ctx_.input->LeftDown();
    start_.Draw(f.Large(), mx, my, pressed);
    howTo_.Draw(f.Large(), mx, my, pressed);
    settings_.Draw(f.Large(), mx, my, pressed);
    quit_.Draw(f.Large(), mx, my, pressed);
    help_.Draw(f, mx, my, pressed);
}
