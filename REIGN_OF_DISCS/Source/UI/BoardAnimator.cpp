#include "UI/BoardAnimator.h"
#include <algorithm>

void BoardAnimator::OnPlaced(Pos pos, CellOwner owner, const std::vector<Pos>& flips, double speedScale)
{
    anims_.push_back(Anim{ Kind::Place, pos, owner, now_, PLACE_SEC * speedScale });
    highlight_ = pos;
    highlightDuration_ = HIGHLIGHT_SEC * speedScale;
    highlightEnd_ = now_ + highlightDuration_;
    double start = now_ + PLACE_SEC * speedScale;
    for (const Pos& p : flips)
    {
        anims_.push_back(Anim{ Kind::Flip, p, owner, start, FLIP_SEC * speedScale });
        start += FLIP_INTERVAL_SEC * speedScale; // one after another, short interval
    }
}

void BoardAnimator::OnFlipped(Pos pos, CellOwner newOwner, double speedScale)
{
    anims_.push_back(Anim{ Kind::Flip, pos, newOwner, now_, FLIP_SEC * speedScale });
    highlight_ = pos;
    highlightDuration_ = HIGHLIGHT_SEC * speedScale;
    highlightEnd_ = now_ + highlightDuration_;
}

void BoardAnimator::OnAppeared(Pos pos, CellOwner owner, double speedScale)
{
    OnPlaced(pos, owner, {}, speedScale);
}

void BoardAnimator::Update(double deltaSec)
{
    now_ += deltaSec;
    anims_.erase(std::remove_if(anims_.begin(), anims_.end(),
        [this](const Anim& a) { return now_ >= a.start + a.duration; }), anims_.end());
}

bool BoardAnimator::Busy() const
{
    return !anims_.empty();
}

void BoardAnimator::Clear()
{
    anims_.clear();
    highlightEnd_ = 0.0;
}

void BoardAnimator::FillView(BoardView& view) const
{
    for (const Anim& a : anims_)
    {
        StoneOverride o;
        o.pos = a.pos;
        const double t = a.duration > 0.0 ? std::clamp((now_ - a.start) / a.duration, 0.0, 1.0) : 1.0;
        if (a.kind == Kind::Place)
        {
            o.owner = now_ < a.start ? CellOwner::None : a.to;
            o.scale = 0.3 + 0.7 * t;   // grows into place
        }
        else
        {
            // First half: old colour shrinking; second half: new colour widening.
            const bool firstHalf = t < 0.5;
            o.owner = firstHalf ? Opponent(a.to) : a.to;
            o.widthScale = firstHalf ? 1.0 - t * 2.0 : (t - 0.5) * 2.0;
        }
        view.overrides.push_back(o);
    }
    if (now_ < highlightEnd_ && highlightDuration_ > 0.0)
    {
        view.highlight = highlight_;
        view.highlightStrength = (highlightEnd_ - now_) / highlightDuration_;
    }
}
