#pragma once
// Settings: BGM volume, SE volume, animation speed (F004 spec 6). Applied immediately, not saved.

#include "Core/SceneManager.h"
#include "UI/DrawUtil.h"

class SettingsScene : public SceneBase
{
public:
    explicit SettingsScene(AppContext& context);
    void OnEnter() override;
    void Update(double deltaSec) override;
    void Draw() const override;

private:
    void Refresh();

    Button bgmDown_;
    Button bgmUp_;
    Button seDown_;
    Button seUp_;
    Button speedNormal_;
    Button speedFast_;
    Button back_;
};
