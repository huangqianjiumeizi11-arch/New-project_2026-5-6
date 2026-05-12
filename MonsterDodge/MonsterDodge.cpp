#include "Game.h"

#include <time.h>
#include <stdlib.h>

int main() {
    srand((unsigned int)time(nullptr));
    initgraph(SCREEN_W, SCREEN_H);
    SetWindowText(GetHWnd(), L"能天使的大冒险");
    BeginBatchDraw();

    if (!ShowStartMenu()) {
        EndBatchDraw();
        musicManager.Close();
        closegraph();
        return 0;
    }
    startGame();

    while (true) {
        updateState();
        if (returnToStartMenuRequested) {
            returnToStartMenuRequested = false;
            if (!ShowStartMenu()) break;
            startGame();
            continue;
        }
        drawFrame();
        FlushBatchDraw();
        Sleep(16);
    }

    EndBatchDraw();
    musicManager.Close();
    closegraph();
    return 0;
}
