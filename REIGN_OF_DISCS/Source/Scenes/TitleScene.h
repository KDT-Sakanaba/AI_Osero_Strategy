#pragma once

#include "Core/SceneManager.h"
#include "UI/DrawUtil.h"
#include "UI/HelpOverlay.h"

class TitleScene : public SceneBase
{
public:
    explicit TitleScene(AppContext& context);
    void OnEnter() override;
    void Update(double deltaSec) override;
    void Draw() const override;

private:
    Button start_;
    Button howTo_;
    HelpOverlay help_;
    Button settings_;
    Button quit_;
};
