#pragma once
// Stone animations (original 9.1, 9.2). Drawing only: the game state is already final (F004 R-01).

#include <vector>
#include "UI/BoardRenderer.h"

class BoardAnimator
{
public:
    // A stone placed at 'pos' with these flips (flip order = list order).
    void OnPlaced(Pos pos, CellOwner owner, const std::vector<Pos>& flips, double speedScale);
    // A single stone changed by an ability (Forced Flip = flip, Tactical Placement = place).
    void OnFlipped(Pos pos, CellOwner newOwner, double speedScale);
    void OnAppeared(Pos pos, CellOwner owner, double speedScale);

    void Update(double deltaSec);
    bool Busy() const;            // input is blocked while true (original 9.2)
    void Clear();

    // Adds stone overrides / highlight for the current moment.
    void FillView(BoardView& view) const;

    // Timing constants (seconds, before the speed scale).
    static constexpr double PLACE_SEC = 0.18;
    static constexpr double HIGHLIGHT_SEC = 0.6;
    static constexpr double FLIP_SEC = 0.2;
    static constexpr double FLIP_INTERVAL_SEC = 0.05;

private:
    enum class Kind { Place, Flip };
    struct Anim
    {
        Kind kind;
        Pos pos;
        CellOwner to;
        double start;
        double duration;
    };

    std::vector<Anim> anims_;
    double now_ = 0.0;
    Pos highlight_{ -1, -1 };
    double highlightEnd_ = 0.0;
    double highlightDuration_ = HIGHLIGHT_SEC;
};
