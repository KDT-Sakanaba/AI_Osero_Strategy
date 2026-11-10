#include "Scenes/SettingsScene.h"
#include <algorithm>
#include <memory>
#include "Core/Settings.h"
#include "DxLib.h"
#include "Input/InputManager.h"
#include "Scenes/TitleScene.h"
#include "UI/FontSet.h"

namespace
{
    constexpr int LABEL_X = 330;
    constexpr int VALUE_X = 760;
    constexpr int ROW_Y[3] = { 220, 320, 420 };

    void Step(int& value, int delta)
    {
        value = std::clamp(value + delta, 0, Settings::VOLUME_MAX);
    }
}

SettingsScene::SettingsScene(AppContext& context) : SceneBase(context)
{
    bgmDown_ = Button{ 620, ROW_Y[0], 60, 52, "−" };
    bgmUp_ = Button{ 880, ROW_Y[0], 60, 52, "＋" };
    seDown_ = Button{ 620, ROW_Y[1], 60, 52, "−" };
    seUp_ = Button{ 880, ROW_Y[1], 60, 52, "＋" };
    speedNormal_ = Button{ 620, ROW_Y[2], 150, 52, "通常" };
    speedFast_ = Button{ 790, ROW_Y[2], 150, 52, "速い" };
    back_ = Button{ (Ui::SCREEN_W - 220) / 2, 560, 220, 56, "戻る" };
    Refresh();
}

void SettingsScene::OnEnter()
{
    PlayBgm(BgmId::Title);
}

void SettingsScene::Refresh()
{
    const Settings& s = *ctx_.settings;
    bgmDown_.enabled = s.bgmVolume > 0;
    bgmUp_.enabled = s.bgmVolume < Settings::VOLUME_MAX;
    seDown_.enabled = s.seVolume > 0;
    seUp_.enabled = s.seVolume < Settings::VOLUME_MAX;
    speedNormal_.selected = !s.fastAnimation;
    speedFast_.selected = s.fastAnimation;
}

void SettingsScene::Update(double)
{
    const InputManager& in = *ctx_.input;
    const int mx = in.MouseX();
    const int my = in.MouseY();
    const bool click = in.LeftClicked();
    Settings& s = *ctx_.settings;
    bool changed = true;

    if (bgmDown_.Clicked(mx, my, click)) Step(s.bgmVolume, -Settings::VOLUME_STEP);
    else if (bgmUp_.Clicked(mx, my, click)) Step(s.bgmVolume, Settings::VOLUME_STEP);
    else if (seDown_.Clicked(mx, my, click)) Step(s.seVolume, -Settings::VOLUME_STEP);
    else if (seUp_.Clicked(mx, my, click)) Step(s.seVolume, Settings::VOLUME_STEP);
    else if (speedNormal_.Clicked(mx, my, click)) s.fastAnimation = false;
    else if (speedFast_.Clicked(mx, my, click)) s.fastAnimation = true;
    else if (back_.Clicked(mx, my, click) || in.KeyPressed(KEY_INPUT_ESCAPE) || in.RightClicked())
    {
        PlaySe(SoundId::Button);
        ctx_.scenes->Change(std::make_unique<TitleScene>(ctx_));
        return;
    }
    else changed = false;

    if (changed)
    {
        if (ctx_.audio != nullptr) ctx_.audio->ApplyVolumes(s); // takes effect at once (F004 AC-03)
        PlaySe(SoundId::Button);
        Refresh();
    }
}

void SettingsScene::Draw() const
{
    const FontSet& f = *ctx_.fonts;
    const Settings& s = *ctx_.settings;
    const InputManager& in = *ctx_.input;
    const int mx = in.MouseX();
    const int my = in.MouseY();
    const bool pressed = in.LeftDown();

    DrawUtil::FillRect(0, 0, Ui::SCREEN_W, Ui::SCREEN_H, Ui::BG);
    DrawUtil::TextCentered(Ui::SCREEN_W / 2, 80, "設定", Ui::TEXT_ACCENT, f.Title());

    DrawUtil::Text(LABEL_X, ROW_Y[0] + 10, "BGM 音量", Ui::TEXT, f.Large());
    DrawUtil::TextCentered(VALUE_X + 20, ROW_Y[0] + 10, std::to_string(s.bgmVolume) + " %", Ui::TEXT_ACCENT, f.Large());
    DrawUtil::Text(LABEL_X, ROW_Y[1] + 10, "効果音 音量", Ui::TEXT, f.Large());
    DrawUtil::TextCentered(VALUE_X + 20, ROW_Y[1] + 10, std::to_string(s.seVolume) + " %", Ui::TEXT_ACCENT, f.Large());
    DrawUtil::Text(LABEL_X, ROW_Y[2] + 10, "アニメーション速度", Ui::TEXT, f.Large());

    bgmDown_.Draw(f.Large(), mx, my, pressed);
    bgmUp_.Draw(f.Large(), mx, my, pressed);
    seDown_.Draw(f.Large(), mx, my, pressed);
    seUp_.Draw(f.Large(), mx, my, pressed);
    speedNormal_.Draw(f.Large(), mx, my, pressed);
    speedFast_.Draw(f.Large(), mx, my, pressed);
    back_.Draw(f.Large(), mx, my, pressed);

    DrawUtil::TextCentered(Ui::SCREEN_W / 2, 500, "設定はゲームを終了するまで有効です（保存はしません）", Ui::TEXT_DIM, f.Medium());
}
