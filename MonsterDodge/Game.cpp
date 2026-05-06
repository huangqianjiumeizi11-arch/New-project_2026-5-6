#include "Game.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
Player player;
MusicManager musicManager;
GameState state = GameState::Menu;
GameState previousState = GameState::Menu;
int currentLevel = 1;
int defeatedCount = 0;
int selectedUpgrade = 0;
int score = 0;
int combo = 0;
int comboTimer = 0;
bool mouseDownLast = false;
bool returnToStartMenuRequested = false;

std::vector<RectF> walls;
std::vector<Monster> monsters;
std::vector<Supply> supplies;
std::vector<Bullet> bullets;
std::vector<Slash> slashes;

float dist(Vec2 a, Vec2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

Vec2 normalize(Vec2 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y);
    if (len < 0.001f) return { 0, 0 };
    return { v.x / len, v.y / len };
}

float dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

bool pointInRect(Vec2 p, RectF r) {
    return p.x >= r.x && p.x <= r.x + r.w && p.y >= r.y && p.y <= r.y + r.h;
}

bool circleRectHit(Vec2 c, float radius, RectF r) {
    float cx = std::max(r.x, std::min(c.x, r.x + r.w));
    float cy = std::max(r.y, std::min(c.y, r.y + r.h));
    float dx = c.x - cx;
    float dy = c.y - cy;
    return dx * dx + dy * dy <= radius * radius;
}

bool blocked(Vec2 pos, float radius) {
    if (pos.x < 20 || pos.x > SCREEN_W - 20 || pos.y < 20 || pos.y > SCREEN_H - 20) return true;
    for (const auto& wall : walls) {
        if (circleRectHit(pos, radius, wall)) return true;
    }
    return false;
}

void moveWithCollision(Vec2& pos, float radius, Vec2 delta) {
    Vec2 nextX{ pos.x + delta.x, pos.y };
    if (!blocked(nextX, radius)) pos.x = nextX.x;
    Vec2 nextY{ pos.x, pos.y + delta.y };
    if (!blocked(nextY, radius)) pos.y = nextY.y;
}

void resetPlayer() {
    player = Player();
}

void addSupply(int type, float x, float y) {
    supplies.push_back(Supply{ type, { x, y }, 12, true });
}

float randFloat(float minValue, float maxValue) {
    return minValue + (maxValue - minValue) * ((float)rand() / (float)RAND_MAX);
}

bool supplyPositionClear(Vec2 pos, float radius) {
    if (blocked(pos, radius)) return false;
    if (dist(pos, player.pos) < 90) return false;

    for (const auto& supply : supplies) {
        if (dist(pos, supply.pos) < radius + supply.radius + 60) return false;
    }
    for (const auto& monster : monsters) {
        if (dist(pos, monster.pos) < radius + monster.radius + 55) return false;
    }
    return true;
}

Vec2 randomOpenPosition(float radius) {
    for (int i = 0; i < 300; ++i) {
        Vec2 pos{ randFloat(40.0f, SCREEN_W - 40.0f), randFloat(70.0f, SCREEN_H - 40.0f) };
        if (supplyPositionClear(pos, radius)) return pos;
    }

    for (int y = 60; y <= SCREEN_H - 40; y += 36) {
        for (int x = 40; x <= SCREEN_W - 40; x += 36) {
            Vec2 pos{ (float)x, (float)y };
            if (supplyPositionClear(pos, radius)) return pos;
        }
    }
    return { 90, 560 };
}

void addRandomSupply(int type) {
    Vec2 pos = randomOpenPosition(12.0f);
    addSupply(type, pos.x, pos.y);
}

void addMonster(MonsterType type, float x, float y, int hp, float speed) {
    Monster m;
    m.type = type;
    m.pos = { x, y };
    m.maxHp = hp;
    m.hp = hp;
    m.speed = speed;
    if (type == MonsterType::Boss) m.radius = 26.0f;
    else m.radius = (type == MonsterType::Shooter) ? 16.0f : 15.0f;
    m.shootCooldown = (float)(60 + rand() % 60);
    monsters.push_back(m);
}

void loadLevel(int level) {
    currentLevel = level;
    walls.clear();
    monsters.clear();
    supplies.clear();
    bullets.clear();
    slashes.clear();
    defeatedCount = 0;
    selectedUpgrade = 0;

    player.pos = { 90, 90 };
    player.hp = std::min(player.hp + 2, player.maxHp);
    player.dashEnergy = 100;
    player.attackCooldown = 0;
    player.hurtCooldown = 0;
    player.dashInvincible = 0;

    if (level == 1) {
        walls.push_back({ 280, 120, 80, 260 });
        walls.push_back({ 560, 250, 90, 240 });
        walls.push_back({ 410, 500, 180, 40 });
        addMonster(MonsterType::Chaser, 760, 130, 2, 1.15f);
        addMonster(MonsterType::Chaser, 780, 470, 2, 1.20f);
        addMonster(MonsterType::Chaser, 470, 330, 2, 1.10f);
        addRandomSupply(0);
        addRandomSupply(1);
    }
    else if (level == 2) {
        walls.push_back({ 190, 180, 140, 55 });
        walls.push_back({ 190, 390, 140, 55 });
        walls.push_back({ 460, 100, 60, 210 });
        walls.push_back({ 460, 360, 60, 210 });
        walls.push_back({ 690, 225, 110, 170 });
        addMonster(MonsterType::Chaser, 785, 95, 3, 1.30f);
        addMonster(MonsterType::Chaser, 820, 525, 3, 1.35f);
        addMonster(MonsterType::Chaser, 550, 310, 3, 1.25f);
        addMonster(MonsterType::Shooter, 650, 310, 2, 0.75f);
        addRandomSupply(0);
        addRandomSupply(1);
    }
    else {
        walls.push_back({ 150, 125, 70, 390 });
        walls.push_back({ 355, 95, 250, 55 });
        walls.push_back({ 355, 490, 250, 55 });
        walls.push_back({ 730, 125, 70, 390 });
        walls.push_back({ 420, 250, 110, 130 });
        addMonster(MonsterType::Chaser, 825, 100, 4, 1.45f);
        addMonster(MonsterType::Chaser, 835, 535, 4, 1.45f);
        addMonster(MonsterType::Chaser, 280, 520, 4, 1.40f);
        addMonster(MonsterType::Shooter, 700, 320, 3, 0.85f);
        addMonster(MonsterType::Shooter, 300, 120, 3, 0.85f);
        addMonster(MonsterType::Boss, 830, 320, 10, 0.80f);
        addRandomSupply(0);
        addRandomSupply(1);
    }
}

void startGame() {
    resetPlayer();
    score = 0;
    combo = 0;
    comboTimer = 0;
    player.level = 1;
    loadLevel(1);
    state = GameState::Playing;
}

void drawCenteredText(int y, const wchar_t* text, int size, COLORREF color) {
    settextstyle(size, 0, L"Microsoft YaHei");
    settextcolor(color);
    setbkmode(TRANSPARENT);
    int w = textwidth(text);
    outtextxy((SCREEN_W - w) / 2, y, text);
}

void drawBar(int x, int y, int w, int h, float value, float maxValue, COLORREF color) {
    setfillcolor(RGB(38, 43, 52));
    solidroundrect(x, y, x + w, y + h, 6, 6);
    float rate = std::max(0.0f, std::min(1.0f, value / maxValue));
    setfillcolor(color);
    solidroundrect(x, y, x + (int)(w * rate), y + h, 6, 6);
}
void drawWorld() {
    setbkcolor(RGB(24, 29, 34));
    cleardevice();

    setlinecolor(RGB(35, 42, 48));
    for (int x = 0; x < SCREEN_W; x += 40) line(x, 0, x, SCREEN_H);
    for (int y = 0; y < SCREEN_H; y += 40) line(0, y, SCREEN_W, y);

    setfillcolor(RGB(94, 103, 110));
    setlinecolor(RGB(122, 134, 143));
    for (const auto& wall : walls) {
        solidroundrect((int)wall.x, (int)wall.y, (int)(wall.x + wall.w), (int)(wall.y + wall.h), 8, 8);
    }

    for (const auto& s : supplies) {
        if (!s.alive) continue;
        setfillcolor(s.type == 0 ? RGB(226, 85, 85) : RGB(242, 196, 76));
        solidcircle((int)s.pos.x, (int)s.pos.y, (int)s.radius);
        setlinecolor(RGB(255, 255, 255));
        circle((int)s.pos.x, (int)s.pos.y, (int)s.radius);
    }

    for (const auto& b : bullets) {
        if (!b.alive) continue;
        setfillcolor(RGB(169, 103, 235));
        solidcircle((int)b.pos.x, (int)b.pos.y, (int)b.radius);
    }

    for (const auto& slash : slashes) {
        float t = slash.life / 8.0f;
        setlinecolor(RGB(97, 202, 255));
        setlinestyle(PS_SOLID, 5);
        Vec2 side{ -slash.dir.y, slash.dir.x };
        Vec2 a{ slash.pos.x + slash.dir.x * 25 + side.x * 28, slash.pos.y + slash.dir.y * 25 + side.y * 28 };
        Vec2 b{ slash.pos.x + slash.dir.x * 64, slash.pos.y + slash.dir.y * 64 };
        Vec2 c{ slash.pos.x + slash.dir.x * 25 - side.x * 28, slash.pos.y + slash.dir.y * 25 - side.y * 28 };
        line((int)a.x, (int)a.y, (int)b.x, (int)b.y);
        line((int)b.x, (int)b.y, (int)c.x, (int)c.y);
        setlinestyle(PS_SOLID, 1);
    }

    for (const auto& m : monsters) {
        COLORREF body = RGB(221, 90, 84);
        if (m.type == MonsterType::Shooter) body = RGB(185, 111, 222);
        if (m.type == MonsterType::Boss) body = RGB(232, 126, 56);
        setfillcolor(body);
        solidcircle((int)m.pos.x, (int)m.pos.y, (int)m.radius);
        if (m.type == MonsterType::Boss) {
            setlinecolor(RGB(255, 213, 92));
            setlinestyle(PS_SOLID, 3);
            circle((int)m.pos.x, (int)m.pos.y, (int)m.radius + 5);
            setlinestyle(PS_SOLID, 1);
        }
        setfillcolor(RGB(27, 30, 34));
        solidcircle((int)(m.pos.x - 5), (int)(m.pos.y - 4), 3);
        solidcircle((int)(m.pos.x + 5), (int)(m.pos.y - 4), 3);
        int hpBarW = (m.type == MonsterType::Boss) ? 58 : 36;
        drawBar((int)m.pos.x - hpBarW / 2, (int)m.pos.y - (int)m.radius - 16, hpBarW, 5, (float)m.hp, (float)m.maxHp, RGB(246, 90, 90));
    }

    POINT mouse;
    GetCursorPos(&mouse);
    ScreenToClient(GetHWnd(), &mouse);
    Vec2 aim = normalize({ (float)mouse.x - player.pos.x, (float)mouse.y - player.pos.y });
    setfillcolor(player.dashInvincible > 0 ? RGB(91, 214, 210) : RGB(82, 184, 132));
    solidcircle((int)player.pos.x, (int)player.pos.y, (int)player.radius);
    setlinecolor(RGB(183, 244, 207));
    line((int)player.pos.x, (int)player.pos.y, (int)(player.pos.x + aim.x * 28), (int)(player.pos.y + aim.y * 28));

    drawBar(18, 16, 180, 14, (float)player.hp, (float)player.maxHp, RGB(222, 78, 78));
    drawBar(18, 38, 180, 10, player.dashEnergy, 100, RGB(238, 190, 65));

    settextstyle(20, 0, L"Microsoft YaHei");
    settextcolor(RGB(230, 236, 240));
    setbkmode(TRANSPARENT);
    wchar_t info[128];
    swprintf_s(info, L"关卡 %d / 3    等级 %d    攻击 %d    剩余怪物 %d    分数 %d", currentLevel, player.level, player.attack, (int)monsters.size(), score);
    outtextxy(220, 16, info);
    if (combo > 1 && comboTimer > 0) {
        wchar_t comboInfo[64];
        swprintf_s(comboInfo, L"连击 x%d", combo);
        settextcolor(RGB(255, 213, 92));
        outtextxy(220, 42, comboInfo);
    }
}

void drawMenu() {
    setbkcolor(RGB(23, 28, 34));
    cleardevice();
    drawCenteredText(140, L"MonsterDodge", 52, RGB(236, 241, 245));
    drawCenteredText(205, L"闪避、拉扯、反击，活过三关", 24, RGB(103, 211, 151));
    drawCenteredText(250, L"WASD 移动，鼠标瞄准，左键攻击，空格冲刺", 22, RGB(197, 207, 215));
    drawCenteredText(290, L"清空怪物后选择升级，第三关会出现 Boss", 22, RGB(197, 207, 215));
    drawCenteredText(365, L"按 Enter 开始", 28, RGB(103, 211, 151));
}

void drawOverlay(const wchar_t* title, const wchar_t* tip) {
    setfillcolor(RGB(16, 19, 23));
    solidrectangle(250, 170, 710, 470);
    setlinecolor(RGB(94, 110, 121));
    roundrect(250, 170, 710, 470, 10, 10);
    drawCenteredText(220, title, 36, RGB(240, 245, 247));
    drawCenteredText(310, tip, 22, RGB(205, 214, 220));
}

bool mouseInRect(POINT mouse, RectF rect) {
    return mouse.x >= rect.x && mouse.x <= rect.x + rect.w && mouse.y >= rect.y && mouse.y <= rect.y + rect.h;
}

void drawMenuButton(RectF rect, const wchar_t* text, bool hovered) {
    COLORREF fill = hovered ? RGB(88, 151, 123) : RGB(42, 50, 58);
    COLORREF border = hovered ? RGB(155, 229, 187) : RGB(94, 110, 121);
    setfillcolor(fill);
    solidroundrect((int)rect.x, (int)rect.y, (int)(rect.x + rect.w), (int)(rect.y + rect.h), 8, 8);
    setlinecolor(border);
    roundrect((int)rect.x, (int)rect.y, (int)(rect.x + rect.w), (int)(rect.y + rect.h), 8, 8);

    settextstyle(22, 0, L"Microsoft YaHei");
    settextcolor(RGB(236, 244, 240));
    setbkmode(TRANSPARENT);
    int textW = textwidth(text);
    int textH = textheight(text);
    outtextxy((int)(rect.x + (rect.w - textW) / 2), (int)(rect.y + (rect.h - textH) / 2), text);
}

POINT getClientMouse() {
    POINT mouse;
    GetCursorPos(&mouse);
    ScreenToClient(GetHWnd(), &mouse);
    return mouse;
}

void drawStartBackground(const wchar_t* title, const wchar_t* subtitle) {
    setbkcolor(RGB(23, 28, 34));
    cleardevice();
    setlinecolor(RGB(35, 42, 48));
    for (int x = 0; x < SCREEN_W; x += 40) line(x, 0, x, SCREEN_H);
    for (int y = 0; y < SCREEN_H; y += 40) line(0, y, SCREEN_W, y);

    drawCenteredText(82, L"MonsterDodge", 50, RGB(236, 241, 245));
    drawCenteredText(142, title, 28, RGB(103, 211, 151));
    if (subtitle != nullptr) drawCenteredText(178, subtitle, 18, RGB(197, 207, 215));
}

int getHoveredButton(const std::vector<RectF>& buttons, POINT mouse) {
    for (int i = 0; i < (int)buttons.size(); ++i) {
        if (mouseInRect(mouse, buttons[i])) return i;
    }
    return -1;
}

void drawMainMenu(POINT mouse) {
    drawStartBackground(L"主菜单", L"闪避、拉扯、反击，活过三关");
    std::vector<RectF> buttons{
        { 360, 238, 240, 48 },
        { 360, 302, 240, 48 },
        { 360, 366, 240, 48 },
        { 360, 430, 240, 48 }
    };
    const wchar_t* labels[] = { L"开始游戏", L"游戏设置", L"操作说明", L"退出游戏" };
    int hovered = getHoveredButton(buttons, mouse);
    for (int i = 0; i < 4; ++i) drawMenuButton(buttons[i], labels[i], hovered == i);
}

void drawSettingsMenu(POINT mouse) {
    drawStartBackground(L"游戏设置", L"调整背景音乐后返回主菜单或直接开始");
    RectF toggleButton{ 330, 230, 300, 46 };
    RectF minusButton{ 315, 326, 54, 44 };
    RectF plusButton{ 591, 326, 54, 44 };
    RectF backButton{ 315, 448, 150, 46 };
    RectF startButton{ 495, 448, 150, 46 };

    wchar_t musicLine[64];
    swprintf_s(musicLine, L"背景音乐：%s", musicManager.IsEnabled() ? L"开启" : L"关闭");
    drawMenuButton(toggleButton, musicLine, mouseInRect(mouse, toggleButton));

    wchar_t volumeLine[64];
    swprintf_s(volumeLine, L"音量：%d", musicManager.GetVolume());
    drawCenteredText(298, volumeLine, 22, RGB(230, 236, 240));
    drawMenuButton(minusButton, L"-", mouseInRect(mouse, minusButton));
    drawBar(385, 340, 190, 16, (float)musicManager.GetVolume(), 100, RGB(103, 211, 151));
    drawMenuButton(plusButton, L"+", mouseInRect(mouse, plusButton));

    if (musicManager.IsOpened()) {
        drawCenteredText(390, L"音乐文件已加载", 16, RGB(160, 220, 185));
    }
    else if (!musicManager.GetLastError().empty()) {
        std::wstring errorLine = L"音乐加载失败：" + musicManager.GetLastError();
        drawCenteredText(390, errorLine.c_str(), 16, RGB(235, 125, 116));
    }

    drawMenuButton(backButton, L"返回主菜单", mouseInRect(mouse, backButton));
    drawMenuButton(startButton, L"开始游戏", mouseInRect(mouse, startButton));
    drawCenteredText(528, L"也可以用 ↑ / ↓ 调整音量，点击背景音乐按钮开关音乐", 16, RGB(160, 172, 181));
}

void drawControlsMenu(POINT mouse) {
    drawStartBackground(L"操作说明", L"熟悉按键后返回主菜单开始游戏");
    settextstyle(22, 0, L"Microsoft YaHei");
    settextcolor(RGB(230, 236, 240));
    setbkmode(TRANSPARENT);

    const wchar_t* lines[] = {
        L"WASD：移动",
        L"鼠标移动：瞄准",
        L"鼠标左键：攻击",
        L"Space：冲刺 / 短暂无敌",
        L"Esc：游戏中打开暂停菜单",
        L"R：失败或胜利后重新开始"
    };
    for (int i = 0; i < 6; ++i) outtextxy(330, 235 + i * 38, lines[i]);

    RectF backButton{ 405, 500, 150, 46 };
    drawMenuButton(backButton, L"返回主菜单", mouseInRect(mouse, backButton));
}

bool ShowStartMenu() {
    musicManager.Open();
    StartMenuScreen screen = StartMenuScreen::MainMenu;

    while (true) {
        POINT mouse = getClientMouse();
        bool clicked = (GetAsyncKeyState(VK_LBUTTON) & 0x0001) != 0;

        if (screen == StartMenuScreen::MainMenu) {
            std::vector<RectF> buttons{
                { 360, 238, 240, 48 },
                { 360, 302, 240, 48 },
                { 360, 366, 240, 48 },
                { 360, 430, 240, 48 }
            };
            int hovered = getHoveredButton(buttons, mouse);
            if (clicked) {
                if (hovered == 0) return true;
                if (hovered == 1) screen = StartMenuScreen::Settings;
                if (hovered == 2) screen = StartMenuScreen::Controls;
                if (hovered == 3) return false;
            }
            drawMainMenu(mouse);
        }
        else if (screen == StartMenuScreen::Settings) {
            RectF toggleButton{ 330, 230, 300, 46 };
            RectF minusButton{ 315, 326, 54, 44 };
            RectF plusButton{ 591, 326, 54, 44 };
            RectF backButton{ 315, 448, 150, 46 };
            RectF startButton{ 495, 448, 150, 46 };

            if (GetAsyncKeyState(VK_UP) & 0x0001) musicManager.SetVolume(musicManager.GetVolume() + 5);
            if (GetAsyncKeyState(VK_DOWN) & 0x0001) musicManager.SetVolume(musicManager.GetVolume() - 5);

            if (clicked) {
                if (mouseInRect(mouse, toggleButton)) musicManager.SetEnabled(!musicManager.IsEnabled());
                else if (mouseInRect(mouse, minusButton)) musicManager.SetVolume(musicManager.GetVolume() - 5);
                else if (mouseInRect(mouse, plusButton)) musicManager.SetVolume(musicManager.GetVolume() + 5);
                else if (mouseInRect(mouse, backButton)) screen = StartMenuScreen::MainMenu;
                else if (mouseInRect(mouse, startButton)) return true;
            }
            drawSettingsMenu(mouse);
        }
        else if (screen == StartMenuScreen::Controls) {
            RectF backButton{ 405, 500, 150, 46 };
            if (clicked && mouseInRect(mouse, backButton)) screen = StartMenuScreen::MainMenu;
            drawControlsMenu(mouse);
        }

        FlushBatchDraw();
        Sleep(16);
    }
}

void drawPauseMenu() {
    POINT mouse;
    GetCursorPos(&mouse);
    ScreenToClient(GetHWnd(), &mouse);

    RectF continueButton{ 315, 315, 150, 48 };
    RectF menuButton{ 495, 315, 150, 48 };

    setfillcolor(RGB(16, 19, 23));
    solidrectangle(250, 170, 710, 430);
    setlinecolor(RGB(94, 110, 121));
    roundrect(250, 170, 710, 430, 10, 10);

    drawCenteredText(220, L"游戏暂停", 36, RGB(240, 245, 247));
    drawCenteredText(270, L"请选择下一步", 20, RGB(205, 214, 220));
    drawMenuButton(continueButton, L"继续游戏", mouseInRect(mouse, continueButton));
    drawMenuButton(menuButton, L"返回主菜单", mouseInRect(mouse, menuButton));
}

void updatePauseMenu() {
    if ((GetAsyncKeyState(VK_LBUTTON) & 0x0001) == 0) return;

    POINT mouse;
    GetCursorPos(&mouse);
    ScreenToClient(GetHWnd(), &mouse);

    RectF continueButton{ 315, 315, 150, 48 };
    RectF menuButton{ 495, 315, 150, 48 };

    if (mouseInRect(mouse, continueButton)) {
        state = GameState::Playing;
    }
    else if (mouseInRect(mouse, menuButton)) {
        returnToStartMenuRequested = true;
    }
}

void drawUpgrade() {
    drawWorld();
    drawOverlay(L"关卡完成", L"选择一项升级：1 生命  2 攻击  3 速度");
    settextstyle(20, 0, L"Microsoft YaHei");
    settextcolor(RGB(215, 224, 230));
    outtextxy(365, 360, L"1. 最大生命 +2，并恢复");
    outtextxy(365, 390, L"2. 攻击力 +1");
    outtextxy(365, 420, L"3. 移动速度 +8%");
}

void applyUpgrade(int choice) {
    if (choice == 1) {
        player.maxHp += 2;
        player.hp = player.maxHp;
    }
    else if (choice == 2) {
        player.attack += 1;
    }
    else if (choice == 3) {
        player.speed *= 1.08f;
    }

    player.level++;
    if (currentLevel >= 3) {
        state = GameState::Victory;
    }
    else {
        loadLevel(currentLevel + 1);
        state = GameState::Playing;
    }
}

void playerAttack(Vec2 dir) {
    if (player.attackCooldown > 0) return;
    player.attackCooldown = 22;
    slashes.push_back(Slash{ player.pos, dir, 8 });

    for (auto& m : monsters) {
        Vec2 toMonster{ m.pos.x - player.pos.x, m.pos.y - player.pos.y };
        float d = sqrtf(toMonster.x * toMonster.x + toMonster.y * toMonster.y);
        Vec2 toward = normalize(toMonster);
        if (d <= 78 && dot(toward, dir) > 0.35f) {
            m.hp -= player.attack;
            Vec2 knock = normalize({ m.pos.x - player.pos.x, m.pos.y - player.pos.y });
            moveWithCollision(m.pos, m.radius, { knock.x * 14, knock.y * 14 });
        }
    }
}

void updatePlaying() {
    if (GetAsyncKeyState(VK_ESCAPE) & 0x0001) {
        previousState = state;
        state = GameState::Paused;
        return;
    }

    POINT mouse;
    GetCursorPos(&mouse);
    ScreenToClient(GetHWnd(), &mouse);
    Vec2 aim = normalize({ (float)mouse.x - player.pos.x, (float)mouse.y - player.pos.y });
    if (aim.x == 0 && aim.y == 0) aim = { 1, 0 };

    Vec2 move{ 0, 0 };
    if (GetAsyncKeyState('W') & 0x8000) move.y -= 1;
    if (GetAsyncKeyState('S') & 0x8000) move.y += 1;
    if (GetAsyncKeyState('A') & 0x8000) move.x -= 1;
    if (GetAsyncKeyState('D') & 0x8000) move.x += 1;
    move = normalize(move);

    float speed = player.speed;
    if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && player.dashEnergy >= 20 && (move.x != 0 || move.y != 0)) {
        speed *= 2.2f;
        player.dashEnergy -= 1.3f;
        player.dashInvincible = 8;
    }
    else {
        player.dashEnergy = std::min(100.0f, player.dashEnergy + 0.35f);
    }
    moveWithCollision(player.pos, player.radius, { move.x * speed, move.y * speed });

    bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    if (mouseDown && !mouseDownLast) {
        playerAttack(aim);
    }
    mouseDownLast = mouseDown;

    if (player.attackCooldown > 0) player.attackCooldown -= 1;
    if (player.hurtCooldown > 0) player.hurtCooldown -= 1;
    if (player.dashInvincible > 0) player.dashInvincible -= 1;
    if (comboTimer > 0) comboTimer -= 1;
    else combo = 0;

    for (auto& slash : slashes) slash.life -= 1;
    slashes.erase(std::remove_if(slashes.begin(), slashes.end(), [](const Slash& s) { return s.life <= 0; }), slashes.end());

    for (auto& m : monsters) {
        Vec2 dir = normalize({ player.pos.x - m.pos.x, player.pos.y - m.pos.y });
        if (m.type == MonsterType::Chaser) {
            moveWithCollision(m.pos, m.radius, { dir.x * m.speed, dir.y * m.speed });
        }
        else if (m.type == MonsterType::Shooter) {
            float d = dist(m.pos, player.pos);
            if (d < 180) moveWithCollision(m.pos, m.radius, { -dir.x * m.speed, -dir.y * m.speed });
            else if (d > 300) moveWithCollision(m.pos, m.radius, { dir.x * m.speed * 0.6f, dir.y * m.speed * 0.6f });

            m.shootCooldown -= 1;
            if (m.shootCooldown <= 0) {
                bullets.push_back(Bullet{ m.pos, { dir.x * 3.0f, dir.y * 3.0f }, 5, true });
                m.shootCooldown = (float)(95 + rand() % 45);
            }
        }
        else {
            float d = dist(m.pos, player.pos);
            if (d > 150) moveWithCollision(m.pos, m.radius, { dir.x * m.speed, dir.y * m.speed });
            else moveWithCollision(m.pos, m.radius, { -dir.y * m.speed * 0.7f, dir.x * m.speed * 0.7f });

            m.shootCooldown -= 1;
            if (m.shootCooldown <= 0) {
                Vec2 side{ -dir.y, dir.x };
                bullets.push_back(Bullet{ m.pos, { dir.x * 3.4f, dir.y * 3.4f }, 6, true });
                bullets.push_back(Bullet{ m.pos, { (dir.x + side.x * 0.32f) * 3.1f, (dir.y + side.y * 0.32f) * 3.1f }, 5, true });
                bullets.push_back(Bullet{ m.pos, { (dir.x - side.x * 0.32f) * 3.1f, (dir.y - side.y * 0.32f) * 3.1f }, 5, true });
                m.shootCooldown = (float)(70 + rand() % 35);
            }
        }

        if (dist(m.pos, player.pos) < m.radius + player.radius && player.hurtCooldown <= 0 && player.dashInvincible <= 0) {
            player.hp -= (m.type == MonsterType::Boss) ? 2 : 1;
            player.hurtCooldown = 45;
            Vec2 push = normalize({ player.pos.x - m.pos.x, player.pos.y - m.pos.y });
            moveWithCollision(player.pos, player.radius, { push.x * 22, push.y * 22 });
        }
    }

    for (auto& b : bullets) {
        if (!b.alive) continue;
        b.pos.x += b.vel.x;
        b.pos.y += b.vel.y;
        if (blocked(b.pos, b.radius)) b.alive = false;
        if (dist(b.pos, player.pos) < b.radius + player.radius && player.hurtCooldown <= 0 && player.dashInvincible <= 0) {
            b.alive = false;
            player.hp -= 1;
            player.hurtCooldown = 40;
        }
    }

    for (auto& s : supplies) {
        if (!s.alive) continue;
        if (dist(s.pos, player.pos) < s.radius + player.radius) {
            s.alive = false;
            if (s.type == 0) player.hp = std::min(player.maxHp, player.hp + 2);
            else player.dashEnergy = 100;
        }
    }

    for (const auto& m : monsters) {
        if (m.hp <= 0) {
            int baseScore = 120;
            if (m.type == MonsterType::Shooter) baseScore = 180;
            if (m.type == MonsterType::Boss) baseScore = 600;
            combo += 1;
            comboTimer = 150;
            score += baseScore * currentLevel + std::max(0, combo - 1) * 25;
            defeatedCount += 1;
        }
    }
    monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](const Monster& m) { return m.hp <= 0; }), monsters.end());
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) { return !b.alive; }), bullets.end());

    if (player.hp <= 0) state = GameState::GameOver;
    if (monsters.empty()) state = GameState::LevelClear;
}

void drawFrame() {
    if (state == GameState::Menu) {
        drawMenu();
    }
    else if (state == GameState::Playing) {
        drawWorld();
    }
    else if (state == GameState::LevelClear) {
        drawUpgrade();
    }
    else if (state == GameState::Paused) {
        drawWorld();
        drawPauseMenu();
    }
    else if (state == GameState::GameOver) {
        drawWorld();
        drawOverlay(L"游戏失败", L"按 Esc 返回主菜单，按 R 重新开始");
    }
    else if (state == GameState::Victory) {
        drawWorld();
        drawOverlay(L"胜利！", L"按 Esc 返回主菜单，按 R 再来一局");
    }
}

void updateState() {
    if (state == GameState::Menu) {
        if (GetAsyncKeyState(VK_RETURN) & 0x0001) startGame();
    }
    else if (state == GameState::Playing) {
        updatePlaying();
    }
    else if (state == GameState::LevelClear) {
        if (GetAsyncKeyState('1') & 0x0001) applyUpgrade(1);
        if (GetAsyncKeyState('2') & 0x0001) applyUpgrade(2);
        if (GetAsyncKeyState('3') & 0x0001) applyUpgrade(3);
    }
    else if (state == GameState::Paused) {
        updatePauseMenu();
    }
    else if (state == GameState::GameOver || state == GameState::Victory) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x0001) returnToStartMenuRequested = true;
        if (GetAsyncKeyState('R') & 0x0001) startGame();
    }
}

