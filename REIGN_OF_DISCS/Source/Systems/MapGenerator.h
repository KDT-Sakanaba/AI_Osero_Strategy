#pragma once
// Special-square layouts for the strategy mode.
// Random layouts follow the same rules the fixed layout satisfies:
//  - point-symmetric about the board centre ((x,y) <-> (7-x,7-y)),
//  - same counts (mana 4, ore 4, altar 2, base 4),
//  - never on a corner, the four centre squares, or the 8 squares next to them (the opening moves),
//  - no two special squares side by side (up/down/left/right; diagonal is allowed).

#include <array>
#include <cstdint>
#include "Utils/Constants.h"

using SpecialLayout = std::array<Const::SpecialCell, Const::SPECIAL_CELLS.size()>;

class MapGenerator
{
public:
    static SpecialLayout Generate(MapLayout layout, std::uint32_t seed);
    static SpecialLayout Standard() { return Const::SPECIAL_CELLS; }

    // True if the layout follows all the rules above (used by tests).
    static bool IsValid(const SpecialLayout& layout);
};
