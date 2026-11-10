#include "Board/Cell.h"

const char* CellTypeName(CellType type)
{
    switch (type)
    {
    case CellType::ManaMine:    return "マナ鉱脈";
    case CellType::OreMine:     return "鉱石鉱脈";
    case CellType::Altar:       return "祭壇マス";
    case CellType::NeutralBase: return "中立拠点";
    default:                    return "通常マス";
    }
}

const char* FacilityName(FacilityType type)
{
    switch (type)
    {
    case FacilityType::MiningFacility: return "採掘所";
    case FacilityType::ManaGenerator:  return "魔力炉";
    case FacilityType::Fort:           return "砦";
    case FacilityType::Altar:          return "祭壇（施設）";
    default:                           return "なし";
    }
}
