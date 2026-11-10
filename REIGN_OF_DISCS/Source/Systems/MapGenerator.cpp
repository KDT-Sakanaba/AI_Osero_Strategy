#include "Systems/MapGenerator.h"
#include <algorithm>
#include <cstdlib>
#include <random>
#include <vector>

namespace
{
    constexpr int N = Const::BOARD_SIZE;
    constexpr int MAX_ATTEMPTS = 200;

    // Pairs to place: 2 mana, 2 ore, 1 altar, 2 base (each pair = 2 squares).
    constexpr CellType PAIRS[] = {
        CellType::ManaMine, CellType::ManaMine, CellType::OreMine, CellType::OreMine,
        CellType::Altar, CellType::NeutralBase, CellType::NeutralBase };

    Pos Mirror(Pos p) { return Pos{ N - 1 - p.x, N - 1 - p.y }; }

    bool IsCorner(Pos p) { return (p.x == 0 || p.x == N - 1) && (p.y == 0 || p.y == N - 1); }
    bool IsCentre(Pos p) { return (p.x == 3 || p.x == 4) && (p.y == 3 || p.y == 4); }

    // The 8 squares side by side with the centre 2x2: the first moves of both players are made here,
    // so a special square there would be a free gift to whoever moves first.
    bool IsOpeningSquare(Pos p)
    {
        const bool inRing = p.x >= 2 && p.x <= 5 && p.y >= 2 && p.y <= 5 && !IsCentre(p);
        const bool diagonal = (p.x == 2 || p.x == 5) && (p.y == 2 || p.y == 5);
        return inRing && !diagonal;
    }

    // Side by side (up / down / left / right). Diagonal neighbours are allowed, as in the standard layout.
    bool Touches(Pos a, Pos b) { return std::abs(a.x - b.x) + std::abs(a.y - b.y) <= 1; }
}

SpecialLayout MapGenerator::Generate(MapLayout layout, std::uint32_t seed)
{
    if (layout == MapLayout::Standard)
    {
        return Standard();
    }

    // Candidate squares in one half; the other half is the mirror image.
    std::vector<Pos> candidates;
    for (int y = 0; y < N; ++y)
    {
        for (int x = 0; x < N; ++x)
        {
            const Pos p{ x, y };
            const Pos m = Mirror(p);
            const bool firstHalf = (y < m.y) || (y == m.y && x < m.x);
            if (firstHalf && !IsCorner(p) && !IsCentre(p) && !IsOpeningSquare(p) && !Touches(p, m))
            {
                candidates.push_back(p);
            }
        }
    }

    std::mt19937 rng(seed);
    for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
    {
        std::shuffle(candidates.begin(), candidates.end(), rng);
        std::vector<Pos> chosen;
        for (const Pos& p : candidates)
        {
            const Pos m = Mirror(p);
            const bool clear = std::none_of(chosen.begin(), chosen.end(),
                [&](Pos c) { return Touches(c, p) || Touches(c, m); });
            if (clear)
            {
                chosen.push_back(p);
                chosen.push_back(m);
                if (chosen.size() == Const::SPECIAL_CELLS.size()) break;
            }
        }
        if (chosen.size() != Const::SPECIAL_CELLS.size())
        {
            continue; // dead end: shuffle again
        }
        SpecialLayout out{};
        for (size_t i = 0; i < std::size(PAIRS); ++i)
        {
            out[i * 2] = Const::SpecialCell{ chosen[i * 2], PAIRS[i] };
            out[i * 2 + 1] = Const::SpecialCell{ chosen[i * 2 + 1], PAIRS[i] };
        }
        return out;
    }
    return Standard(); // practically unreachable; keeps the game playable
}

bool MapGenerator::IsValid(const SpecialLayout& layout)
{
    int counts[5] = {};
    for (size_t i = 0; i < layout.size(); ++i)
    {
        const Pos p = layout[i].pos;
        if (p.x < 0 || p.x >= N || p.y < 0 || p.y >= N || IsCorner(p) || IsCentre(p) || IsOpeningSquare(p))
        {
            return false;
        }
        ++counts[static_cast<int>(layout[i].type)];
        bool mirrored = false;
        for (size_t j = 0; j < layout.size(); ++j)
        {
            if (i == j) continue;
            if (Touches(p, layout[j].pos)) return false;
            if (layout[j].pos == Mirror(p) && layout[j].type == layout[i].type) mirrored = true;
        }
        if (!mirrored) return false;
    }
    return counts[static_cast<int>(CellType::ManaMine)] == 4 && counts[static_cast<int>(CellType::OreMine)] == 4 &&
        counts[static_cast<int>(CellType::Altar)] == 2 && counts[static_cast<int>(CellType::NeutralBase)] == 4;
}
