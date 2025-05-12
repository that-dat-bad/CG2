#include "Engine.h"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    Engine engine;

    if (!engine.Initialize(hInstance, nCmdShow)) {
        return 1;
    }

    engine.Run();

    engine.Finalize();

    return 0;
}