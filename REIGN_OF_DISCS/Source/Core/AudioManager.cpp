#include "Core/AudioManager.h"
#include "Core/Settings.h"
#include "DxLib.h"

namespace
{
    constexpr const char* SE_NAMES[] = { "se_place", "se_flip", "se_resource", "se_button", "se_error" };
    constexpr const char* BGM_NAMES[] = { "", "bgm_title", "bgm_game" };
    constexpr const char* EXTENSIONS[] = { ".wav", ".ogg", ".mp3" };

    int ToDxVolume(int percent)
    {
        return percent * 255 / Settings::VOLUME_MAX;
    }
}

std::string AudioManager::SoundFolder()
{
    // DxLib takes UTF-8 paths here (SetUseCharCodeFormat), so convert from UTF-16.
    wchar_t wide[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, wide, MAX_PATH);
    char utf8[MAX_PATH * 3] = {};
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, static_cast<int>(sizeof(utf8)), nullptr, nullptr);
    std::string dir(utf8);
    const size_t slash = dir.find_last_of("\\/");
    dir = (slash == std::string::npos) ? std::string(".") : dir.substr(0, slash);
    return dir + "\\Resources\\Sounds\\";
}

int AudioManager::LoadOptional(const std::string& folder, const char* baseName)
{
    for (const char* ext : EXTENSIONS)
    {
        const std::string file = folder + baseName + ext;
        if (FileRead_size(file.c_str()) <= 0)
        {
            continue; // not present: try the next extension
        }
        const int handle = LoadSoundMem(file.c_str());
        if (handle != -1)
        {
            return handle;
        }
    }
    return -1;
}

void AudioManager::Load()
{
    se_.fill(-1);
    bgm_.fill(-1);
    const std::string folder = SoundFolder();
    for (int i = 0; i < static_cast<int>(SoundId::Count); ++i)
    {
        se_[i] = LoadOptional(folder, SE_NAMES[i]);
    }
    for (int i = 1; i < static_cast<int>(bgm_.size()); ++i)
    {
        bgm_[i] = LoadOptional(folder, BGM_NAMES[i]);
    }
    loaded_ = true;
}

void AudioManager::Release()
{
    if (!loaded_)
    {
        return;
    }
    for (int& h : se_)
    {
        if (h != -1) { DeleteSoundMem(h); h = -1; }
    }
    for (int& h : bgm_)
    {
        if (h != -1) { StopSoundMem(h); DeleteSoundMem(h); h = -1; }
    }
    currentBgm_ = BgmId::None;
    loaded_ = false;
}

void AudioManager::PlaySe(SoundId id) const
{
    const int h = loaded_ ? se_[static_cast<int>(id)] : -1;
    if (h != -1)
    {
        PlaySoundMem(h, DX_PLAYTYPE_BACK, TRUE);
    }
}

void AudioManager::PlayBgm(BgmId id)
{
    if (!loaded_ || id == currentBgm_)
    {
        return; // same track keeps playing
    }
    const int current = bgm_[static_cast<int>(currentBgm_)];
    if (current != -1)
    {
        StopSoundMem(current);
    }
    currentBgm_ = id;
    const int next = bgm_[static_cast<int>(id)];
    if (next != -1)
    {
        PlaySoundMem(next, DX_PLAYTYPE_LOOP, TRUE);
    }
}

void AudioManager::ApplyVolumes(const Settings& settings)
{
    if (!loaded_)
    {
        return;
    }
    for (int h : se_)
    {
        if (h != -1) ChangeVolumeSoundMem(ToDxVolume(settings.seVolume), h);
    }
    for (int h : bgm_)
    {
        if (h != -1) ChangeVolumeSoundMem(ToDxVolume(settings.bgmVolume), h);
    }
}

std::string AudioManager::Report() const
{
    std::string text = "loaded=" + std::to_string(LoadedCount()) + "\n";
    for (int i = 0; i < static_cast<int>(SoundId::Count); ++i)
    {
        text += std::string(SE_NAMES[i]) + "=" + (se_[i] != -1 ? "loaded" : "missing") + "\n";
    }
    for (int i = 1; i < static_cast<int>(bgm_.size()); ++i)
    {
        const int h = bgm_[i];
        text += std::string(BGM_NAMES[i]) + "=" + (h != -1 ? "loaded" : "missing");
        if (h != -1)
        {
            text += std::string(" playing=") + (CheckSoundMem(h) == 1 ? "yes" : "no") +
                " volume=" + std::to_string(GetVolumeSoundMem2(h));
        }
        text += "\n";
    }
    return text;
}

int AudioManager::LoadedCount() const
{
    int n = 0;
    for (int h : se_) if (h != -1) ++n;
    for (int h : bgm_) if (h != -1) ++n;
    return n;
}
