#include "Scenes/SelectScene.h"
#include <memory>
#include "DxLib.h"
#include "Core/Features.h"
#include "Input/InputManager.h"
#include "Scenes/GameScene.h"
#include "Scenes/TitleScene.h"
#include "UI/FontSet.h"

namespace
{
    constexpr RuleMode RULES[2] = { RuleMode::Classic, RuleMode::Strategy };
    constexpr OpponentType OPPONENTS[4] = {
        OpponentType::Local, OpponentType::AiEasy, OpponentType::AiNormal, OpponentType::AiHard };

    std::string WithAvailability(const char* label, bool available)
    {
        return available ? std::string(label) : std::string(label) + "（未実装）";
    }
}

SelectScene::SelectScene(AppContext& context) : SceneBase(context)
{
    const int ruleW = 260;
    const int ruleX = (Ui::SCREEN_W - (ruleW * 2 + 40)) / 2;
    ruleButtons_[0] = Button{ ruleX, 186, ruleW, 56, "通常オセロ" };
    ruleButtons_[1] = Button{ ruleX + ruleW + 40, 186, ruleW, 56,
        WithAvailability("ストラテジー", Features::STRATEGY_MODE) };
    ruleButtons_[1].enabled = Features::STRATEGY_MODE;

    const char* labels[4] = { "ローカル2人対戦", "AI EASY", "AI NORMAL", "AI HARD" };
    const int oppW = 230;
    const int oppX = (Ui::SCREEN_W - (oppW * 4 + 30 * 3)) / 2;
    for (int i = 0; i < 4; ++i)
    {
        const bool available = (i == 0) || Features::AI_OPPONENT;
        opponentButtons_[i] = Button{ oppX + i * (oppW + 30), 302, oppW, 56, WithAvailability(labels[i], available) };
        opponentButtons_[i].enabled = available;
    }

    mapButtons_[0] = Button{ ruleX, 418, ruleW, 56, "ランダム配置" };
    mapButtons_[1] = Button{ ruleX + ruleW + 40, 418, ruleW, 56, "標準配置" };
    config_.layout = MapLayout::Random; // strategy maps are random unless the player picks the standard one

    start_ = Button{ (Ui::SCREEN_W - 260) / 2, 520, 260, 60, "ゲーム開始" };
    back_ = Button{ (Ui::SCREEN_W - 200) / 2, 600, 200, 48, "戻る" };
    RefreshSelection();
}

void SelectScene::RefreshSelection()
{
    for (int i = 0; i < 2; ++i)
    {
        ruleButtons_[i].selected = (config_.rule == RULES[i]);
    }
    for (int i = 0; i < 4; ++i)
    {
        opponentButtons_[i].selected = (config_.opponent == OPPONENTS[i]);
    }
    const bool strategy = (config_.rule == RuleMode::Strategy);
    for (int i = 0; i < 2; ++i)
    {
        mapButtons_[i].enabled = strategy; // the map only matters in the strategy mode
        mapButtons_[i].selected = strategy && (config_.layout == (i == 0 ? MapLayout::Random : MapLayout::Standard));
    }
}

void SelectScene::Update(double)
{
    const InputManager& in = *ctx_.input;
    const int mx = in.MouseX();
    const int my = in.MouseY();
    const bool click = in.LeftClicked();

    for (int i = 0; i < 2; ++i)
    {
        if (ruleButtons_[i].Clicked(mx, my, click))
        {
            config_.rule = RULES[i];
            PlaySe(SoundId::Button);
        }
    }
    for (int i = 0; i < 4; ++i)
    {
        if (opponentButtons_[i].Clicked(mx, my, click))
        {
            config_.opponent = OPPONENTS[i];
            PlaySe(SoundId::Button);
        }
    }
    for (int i = 0; i < 2; ++i)
    {
        if (mapButtons_[i].Clicked(mx, my, click))
        {
            config_.layout = (i == 0) ? MapLayout::Random : MapLayout::Standard;
            PlaySe(SoundId::Button);
        }
    }
    RefreshSelection();

    if (start_.Clicked(mx, my, click))
    {
        PlaySe(SoundId::Button);
        ctx_.scenes->Change(std::make_unique<GameScene>(ctx_, config_));
    }
    else if (back_.Clicked(mx, my, click) || in.KeyPressed(KEY_INPUT_ESCAPE))
    {
        PlaySe(SoundId::Button);
        ctx_.scenes->Change(std::make_unique<TitleScene>(ctx_));
    }
}

void SelectScene::Draw() const
{
    DrawUtil::FillRect(0, 0, Ui::SCREEN_W, Ui::SCREEN_H, Ui::BG);
    const FontSet& f = *ctx_.fonts;
    const int mx = ctx_.input->MouseX();
    const int my = ctx_.input->MouseY();
    const bool pressed = ctx_.input->LeftDown();

    DrawUtil::TextCentered(Ui::SCREEN_W / 2, 70, "モード選択", Ui::TEXT_ACCENT, f.Title());
    DrawUtil::TextCentered(Ui::SCREEN_W / 2, 152, "ルール", Ui::TEXT, f.Medium());
    for (const auto& b : ruleButtons_) b.Draw(f.Medium(), mx, my, pressed);
    DrawUtil::TextCentered(Ui::SCREEN_W / 2, 268, "対戦相手", Ui::TEXT, f.Medium());
    for (const auto& b : opponentButtons_) b.Draw(f.Medium(), mx, my, pressed);
    DrawUtil::TextCentered(Ui::SCREEN_W / 2, 384, config_.rule == RuleMode::Strategy
        ? "マップ（特殊マスの配置）" : "マップ（ストラテジーのみ）", Ui::TEXT, f.Medium());
    for (const auto& b : mapButtons_) b.Draw(f.Medium(), mx, my, pressed);
    start_.Draw(f.Large(), mx, my, pressed);
    back_.Draw(f.Medium(), mx, my, pressed);
}
