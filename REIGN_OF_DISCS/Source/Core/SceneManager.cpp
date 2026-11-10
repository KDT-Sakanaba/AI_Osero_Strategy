#include "Core/SceneManager.h"

void SceneManager::Change(std::unique_ptr<SceneBase> next)
{
    pending_ = std::move(next);
}

bool SceneManager::ApplyPending()
{
    if (!pending_)
    {
        return false;
    }
    if (current_)
    {
        current_->OnExit();
    }
    current_ = std::move(pending_);
    current_->OnEnter();
    return true;
}

void SceneManager::Update(double deltaSec)
{
    if (current_)
    {
        current_->Update(deltaSec);
    }
}

void SceneManager::Draw() const
{
    if (current_)
    {
        current_->Draw();
    }
}

void SceneManager::Clear()
{
    if (current_)
    {
        current_->OnExit();
    }
    current_.reset();
    pending_.reset();
}
