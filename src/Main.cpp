#include "Game.h"
#include <memory>
#include <Windows.h>

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Construct and run the Game instance (keeps scope so destructor runs before exit)
    {
        auto game = std::make_shared<Game>();
        if (FAILED(game->Initialize(hInstance, nCmdShow)))
            return 0;
        return game->Run();
    }
}