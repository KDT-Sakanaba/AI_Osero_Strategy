#pragma once
// Scene ownership and switching. A change request is applied at the start of the next frame,
// and the input of that frame is discarded (F001 R-09).

#include <memory>
#include "Core/AudioManager.h"

class InputManager;
class FontSet;
class SceneManager;
struct Settings;

// Shared services handed to every scene.
struct AppContext
{
    SceneManager* scenes = nullptr;
    InputManager* input = nullptr;
    FontSet* fonts = nullptr;
    AudioManager* audio = nullptr;
    Settings* settings = nullptr;
};

class SceneBase
{
public:
    explicit SceneBase(AppContext& context) : ctx_(context) {}
    virtual ~SceneBase() = default;

    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void Update(double deltaSec) = 0;
    virtual void Draw() const = 0;

protected:
    void PlaySe(SoundId id) const
    {
        if (ctx_.audio != nullptr) ctx_.audio->PlaySe(id);
    }
    void PlayBgm(BgmId id) const
    {
        if (ctx_.audio != nullptr) ctx_.audio->PlayBgm(id);
    }

    AppContext& ctx_;
};

class SceneManager
{
public:
    void Change(std::unique_ptr<SceneBase> next);
    void RequestQuit() { quit_ = true; }
    bool QuitRequested() const { return quit_; }

    // Returns true when a scene change was applied (caller then flushes input).
    bool ApplyPending();
    void Update(double deltaSec);
    void Draw() const;
    void Clear();

private:
    std::unique_ptr<SceneBase> current_;
    std::unique_ptr<SceneBase> pending_;
    bool quit_ = false;
};
