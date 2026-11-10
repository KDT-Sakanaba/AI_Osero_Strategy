#pragma once
// Sound effects and BGM. Files are optional: a missing file is silently skipped (F004 R-03).
// Files: <exe folder>/Resources/Sounds/<name>.(wav|ogg|mp3) — see Resources/Sounds/README.md.

#include <array>
#include <string>

struct Settings;

enum class SoundId
{
    Place,     // se_place
    Flip,      // se_flip
    Resource,  // se_resource
    Button,    // se_button
    Error,     // se_error
    Count
};

enum class BgmId
{
    None,
    Title,     // bgm_title
    Game       // bgm_game
};

class AudioManager
{
public:
    void Load();
    void Release();

    void PlaySe(SoundId id) const;
    void PlayBgm(BgmId id);
    void ApplyVolumes(const Settings& settings);

    int LoadedCount() const;

    // Verification: loaded files and whether the current BGM is actually playing.
    std::string Report() const;

private:
    static std::string SoundFolder();
    static int LoadOptional(const std::string& folder, const char* baseName);

    std::array<int, static_cast<int>(SoundId::Count)> se_{};
    std::array<int, 3> bgm_{};
    BgmId currentBgm_ = BgmId::None;
    bool loaded_ = false;
};
