#pragma once
// Result screen: winner, final score, resources (strategy), retry / title (original 9.4).

#include "Core/SceneManager.h"
#include "Systems/GameState.h"
#include "Systems/VictoryManager.h"
#include "UI/DrawUtil.h"

class ResultScene : public SceneBase
{
public:
    ResultScene(AppContext& context, const GameConfig& config, const GameState& finalState);
    void OnEnter() override;
    void Update(double deltaSec) override;
    void Draw() const override;

private:
    std::string Headline() const;

    GameConfig config_;
    GameState final_;
    GameResult result_;
    Button retry_;
    Button toTitle_;
};
