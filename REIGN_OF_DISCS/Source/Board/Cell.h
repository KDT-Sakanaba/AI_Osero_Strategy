#pragma once
// One board square. UI-only state (selection, hover) is kept in GameUI, not here
// (deviation from the original 11.4 'isSelected', see F001 plan.md).

#include "Utils/Types.h"

struct Cell
{
    CellOwner owner = CellOwner::None;
    CellType type = CellType::Normal;
    FacilityType facility = FacilityType::None;
    TerrainType terrain = TerrainType::Plain;

    // Derived each time the board changes (F002 R-17). Never set by hand outside FacilityManager.
    bool isDefended = false;

    // Magic Reinforcement: non-zero while enhanced; cleared when the owner's opponent finishes a turn.
    int enhancedUntil = 0;

    // Siege Break: facility owner turns left while disabled (0 = active).
    int disabledUntil = 0;

    bool IsEmpty() const { return owner == CellOwner::None; }
    bool HasFacility() const { return facility != FacilityType::None; }
    bool IsEnhanced() const { return enhancedUntil > 0; }
    bool IsFacilityDisabled() const { return disabledUntil > 0; }

    // Protected stones stay in place during normal flipping (F002 R-16, R-19).
    bool IsProtected() const { return isDefended || IsEnhanced(); }
};

const char* CellTypeName(CellType type);
const char* FacilityName(FacilityType type);
