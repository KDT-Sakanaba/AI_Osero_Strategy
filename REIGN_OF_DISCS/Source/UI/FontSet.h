#pragma once
// Font handles created once and released at shutdown (original 15: resource leaks).

class FontSet
{
public:
    bool Create();
    void Release();

    int Title() const { return title_; }
    int Large() const { return large_; }
    int Medium() const { return medium_; }
    int Small() const { return small_; }

private:
    int title_ = -1;
    int large_ = -1;
    int medium_ = -1;
    int small_ = -1;
};
