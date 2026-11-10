#include "UI/FontSet.h"
#include "DxLib.h"

namespace
{
    constexpr const char* FONT_NAME = "Meiryo UI";

    int MakeFont(int size, int thick)
    {
        return CreateFontToHandle(FONT_NAME, size, thick, DX_FONTTYPE_ANTIALIASING_4X4);
    }

    void DeleteIfValid(int& handle)
    {
        if (handle != -1)
        {
            DeleteFontToHandle(handle);
            handle = -1;
        }
    }
}

bool FontSet::Create()
{
    title_ = MakeFont(64, 6);
    large_ = MakeFont(30, 4);
    medium_ = MakeFont(20, 3);
    small_ = MakeFont(16, 2);
    return title_ != -1 && large_ != -1 && medium_ != -1 && small_ != -1;
}

void FontSet::Release()
{
    DeleteIfValid(title_);
    DeleteIfValid(large_);
    DeleteIfValid(medium_);
    DeleteIfValid(small_);
}
