#include "Scenes/ResultScene.h"
#include <memory>
#include "Input/InputManager.h"
#include "Scenes/GameScene.h"
#include "Scenes/TitleScene.h"
#include "UI/BoardRenderer.h"
#include "UI/FontSet.h"

ResultScene::ResultScene(AppContext& context, const GameConfig& config, const GameState& finalState)
    : SceneBase(context), config_(config), final_(finalState), result_(VictoryManager::Judge(finalState.board))
{
    retry_ = Button{ Ui::SCREEN_W / 2 - 280, 600, 260, 60, "もう一度" };
    toTitle_ = Button{ Ui::SCREEN_W / 2 + 20, 600, 260, 60, "タイトルへ" };
}

void ResultScene::OnEnter()
{
    PlayBgm(BgmId::Title);
}

std::string ResultScene::Headline() const
{
    if (result_.IsDraw())
    {
        return "引き分け";
    }
    if (IsAiOpponent(config_.opponent))
    {
        return result_.winner == CellOwner::Player ? "勝利！ プレイヤーが勝利しました" : "敗北… 敵AIの勝利です";
    }
    return final_.Side(result_.winner).name + "が勝利しました";
}

void ResultScene::Update(double)
{
    const InputManager& in = *ctx_.input;
    if (retry_.Clicked(in.MouseX(), in.MouseY(), in.LeftClicked()))
    {
        PlaySe(SoundId::Button);
        ctx_.scenes->Change(std::make_unique<GameScene>(ctx_, config_));
    }
    else if (toTitle_.Clicked(in.MouseX(), in.MouseY(), in.LeftClicked()))
    {
        PlaySe(SoundId::Button);
        ctx_.scenes->Change(std::make_unique<TitleScene>(ctx_));
    }
}

void ResultScene::Draw() const
{
    const FontSet& f = *ctx_.fonts;
    DrawUtil::FillRect(0, 0, Ui::SCREEN_W, Ui::SCREEN_H, Ui::BG);
    DrawUtil::TextCentered(Ui::SCREEN_W / 2, 80, "結果", Ui::TEXT_DIM, f.Large());
    DrawUtil::TextCentered(Ui::SCREEN_W / 2, 140, Headline(), Ui::TEXT_ACCENT, f.Title());

    // Final score = stone count (F004 spec: 最終スコア).
    const int colX[2] = { Ui::SCREEN_W / 2 - 300, Ui::SCREEN_W / 2 + 60 };
    for (int i = 0; i < 2; ++i)
    {
        const CellOwner side = SideOwner(i);
        const Player& p = final_.Side(side);
        int y = 260;
        BoardRenderer::DrawStone(colX[i] + 20, y + 16, side, 1.0, 0.7);
        DrawUtil::Text(colX[i] + 50, y, p.name, Ui::TEXT, f.Large());
        y += 56;
        const int stones = side == CellOwner::Player ? result_.playerStones : result_.enemyStones;
        DrawUtil::Text(colX[i], y, "最終スコア（石の数）: " + std::to_string(stones), Ui::TEXT, f.Medium());
        if (final_.IsStrategy())
        {
            y += 40;
            DrawUtil::Text(colX[i], y, "残り資源", Ui::TEXT_DIM, f.Medium());
            y += 32;
            DrawUtil::Text(colX[i], y, "マナ " + std::to_string(p.resources.mana) + "  鉱石 " + std::to_string(p.resources.ore) +
                "  影響力 " + std::to_string(p.resources.influence), Ui::TEXT, f.Medium());
        }
    }

    const int mx = ctx_.input->MouseX();
    const int my = ctx_.input->MouseY();
    const bool pressed = ctx_.input->LeftDown();
    retry_.Draw(f.Large(), mx, my, pressed);
    toTitle_.Draw(f.Large(), mx, my, pressed);
}
