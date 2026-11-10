#pragma once
// Mode selection: rule (classic / strategy) x opponent (local / AI x3) (F001 spec 2).

#include <array>
#include "Core/SceneManager.h"
#include "UI/DrawUtil.h"
#include "Utils/Types.h"

class SelectScene : public SceneBase
{
public:
    explicit SelectScene(AppContext& context);
    void OnEnter() override { PlayBgm(BgmId::Title); }
    void Update(double deltaSec) override;
    void Draw() const override;

private:
    void RefreshSelection();

    GameConfig config_;
    std::array<Button, 2> ruleButtons_;
    std::array<Button, 4> opponentButtons_;
    std::array<Button, 2> mapButtons_;
    Button start_;
    Button back_;
};
