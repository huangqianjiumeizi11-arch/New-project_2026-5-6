#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <easyx.h>
#include <windows.h>

#include <vector>

#include "MusicManager.h"
#include "Types.h"

extern Player player;
extern MusicManager musicManager;
extern GameState state;
extern GameState previousState;
extern int currentLevel;
extern int defeatedCount;
extern int selectedUpgrade;
extern int score;
extern int combo;
extern int comboTimer;
extern bool mouseDownLast;
extern bool returnToStartMenuRequested;

extern std::vector<RectF> walls;
extern std::vector<Monster> monsters;
extern std::vector<Supply> supplies;
extern std::vector<Bullet> bullets;
extern std::vector<Slash> slashes;

bool ShowStartMenu();
void startGame();
void updateState();
void drawFrame();
