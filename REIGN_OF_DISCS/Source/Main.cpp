#include "DxLib.h"
#include "Core/Game.h"

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    Game game;
    if (game.Initialize())
    {
        game.Run();
    }
    game.Finalize();
    return 0;
}
