#include "Game.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
Player player;
MusicManager musicManager;
GameState state = GameState::Menu;
int currentLevel = 1;
bool mouseDownLast = false;
bool spaceDownLast = false;
bool returnToStartMenuRequested = false;
GameDifficulty gameDifficulty = GameDifficulty::Easy;

std::vector<RectF> walls;
std::vector<Monster> monsters;
std::vector<Supply> supplies;
std::vector<Bullet> bullets;
std::vector<Slash> slashes;

constexpr float MAX_SKILL_ENERGY = 100.0f;
constexpr int SUPPLY_RECOVERY = 20;
constexpr int MONSTER_DAMAGE = 10;
constexpr int BOSS_DAMAGE = 20;
constexpr int MELEE_HIT_INVINCIBLE_FRAMES = 10;
constexpr float MELEE_KNOCKBACK = 22.0f;
constexpr float DASH_ENERGY_COST = 20.0f;
constexpr float DASH_ENERGY_RECOVERY = 0.35f;
constexpr float DASH_SPEED_MULTIPLIER = 2.2f;
constexpr float DASH_DURATION_UPGRADE_FRAMES = 4.0f;
constexpr int PLAYER_SHOOT_COOLDOWN_MIN = 30;
constexpr int PLAYER_SHOOT_COOLDOWN_RANGE = 11;
constexpr int UPGRADED_PLAYER_SHOOT_COOLDOWN_MIN = 20;
constexpr int UPGRADED_PLAYER_SHOOT_COOLDOWN_RANGE = 6;
constexpr int ENEMY_SHOOT_COOLDOWN_FRAMES = 25;
constexpr int BOSS_SHOOT_COOLDOWN_FRAMES = 25;
constexpr float BOSS_CHASE_DISTANCE = 110.0f;
constexpr float BOSS_BULLET_SPEED = 3.4f;
constexpr float BOSS_SPREAD_ANGLE = 30.0f;
constexpr float BOSS_BULLET_RADIUS = 6.0f;
constexpr float PLAYER_BULLET_SPEED = 7.0f;
constexpr float PLAYER_BULLET_DAMAGE = 0.5f;
constexpr float UPGRADED_PLAYER_BULLET_DAMAGE = 1.0f;
constexpr float SHOOTER_BULLET_SPEED = 2.0f;
constexpr int PLAYER_SPRITE_SIZE = 48;
constexpr int PATH_CELL = 32;
constexpr int PATH_COLS = SCREEN_W / PATH_CELL;
constexpr int PATH_ROWS = SCREEN_H / PATH_CELL;

IMAGE playerSprite;
bool playerSpriteLoaded = false;
bool playerSpriteLoadTried = false;

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

Vec2 rotate(Vec2 v, float degrees) {
    float radians = degrees * PI / 180.0f;
    float c = cosf(radians);
    float s = sinf(radians);
    return { v.x * c - v.y * s, v.x * s + v.y * c };
}

bool monsterStepClear(Vec2 pos, float radius, Vec2 dir, float speed) {
    float bufferRadius = radius + 8.0f;
    float lookAhead = std::max(speed + 8.0f, 12.0f);
    return !blocked({ pos.x + dir.x * lookAhead, pos.y + dir.y * lookAhead }, bufferRadius);
}

RectF expandedRect(RectF rect, float amount) {
    return { rect.x - amount, rect.y - amount, rect.w + amount * 2.0f, rect.h + amount * 2.0f };
}

bool segmentHitsRect(Vec2 from, Vec2 to, RectF rect) {
    float length = dist(from, to);
    int steps = std::max(1, (int)(length / 8.0f));
    for (int i = 0; i <= steps; ++i) {
        float t = (float)i / (float)steps;
        Vec2 p{ from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t };
        if (pointInRect(p, rect)) return true;
    }
    return false;
}

const RectF* findBlockingWall(Vec2 from, Vec2 to, float radius) {
    const RectF* bestWall = nullptr;
    float bestDistance = 999999.0f;
    float expand = radius + 6.0f;

    for (const auto& wall : walls) {
        RectF expanded = expandedRect(wall, expand);
        if (!segmentHitsRect(from, to, expanded)) continue;

        Vec2 center{ wall.x + wall.w * 0.5f, wall.y + wall.h * 0.5f };
        float d = dist(from, center);
        if (d < bestDistance) {
            bestDistance = d;
            bestWall = &wall;
        }
    }
    return bestWall;
}

bool hasLineOfSight(Vec2 from, Vec2 to, float radius) {
    return findBlockingWall(from, to, radius) == nullptr;
}

int clampInt(int value, int minValue, int maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

int gridIndex(int col, int row) {
    return row * PATH_COLS + col;
}

Vec2 gridCenter(int col, int row) {
    return { col * (float)PATH_CELL + PATH_CELL * 0.5f, row * (float)PATH_CELL + PATH_CELL * 0.5f };
}

int gridCol(float x) {
    return clampInt((int)(x / PATH_CELL), 0, PATH_COLS - 1);
}

int gridRow(float y) {
    return clampInt((int)(y / PATH_CELL), 0, PATH_ROWS - 1);
}

bool pathCellBlocked(int col, int row, float radius) {
    return blocked(gridCenter(col, row), radius + 6.0f);
}

bool findNearestOpenCell(Vec2 pos, float radius, int& outCol, int& outRow) {
    int baseCol = gridCol(pos.x);
    int baseRow = gridRow(pos.y);
    float bestDistance = 999999.0f;
    bool found = false;

    for (int range = 0; range < std::max(PATH_COLS, PATH_ROWS); ++range) {
        for (int row = baseRow - range; row <= baseRow + range; ++row) {
            for (int col = baseCol - range; col <= baseCol + range; ++col) {
                if (col < 0 || col >= PATH_COLS || row < 0 || row >= PATH_ROWS) continue;
                if (pathCellBlocked(col, row, radius)) continue;

                float d = dist(pos, gridCenter(col, row));
                if (d < bestDistance) {
                    bestDistance = d;
                    outCol = col;
                    outRow = row;
                    found = true;
                }
            }
        }
        if (found) return true;
    }
    return false;
}

Vec2 chooseMonsterPathTarget(Vec2 pos, float radius, Vec2 target) {
    const RectF* blockingWall = findBlockingWall(pos, target, radius);
    if (blockingWall == nullptr) return target;

    int startCol = 0;
    int startRow = 0;
    int goalCol = 0;
    int goalRow = 0;
    if (!findNearestOpenCell(pos, radius, startCol, startRow)) return target;
    if (!findNearestOpenCell(target, radius, goalCol, goalRow)) return target;

    int start = gridIndex(startCol, startRow);
    int goal = gridIndex(goalCol, goalRow);
    if (start == goal) return target;

    const int total = PATH_COLS * PATH_ROWS;
    std::vector<int> cameFrom(total, -1);
    std::vector<int> queue;
    queue.reserve(total);
    queue.push_back(start);
    cameFrom[start] = start;

    const int dc[] = { 1, -1, 0, 0 };
    const int dr[] = { 0, 0, 1, -1 };
    for (int head = 0; head < (int)queue.size(); ++head) {
        int current = queue[head];
        if (current == goal) break;

        int col = current % PATH_COLS;
        int row = current / PATH_COLS;
        for (int i = 0; i < 4; ++i) {
            int nextCol = col + dc[i];
            int nextRow = row + dr[i];
            if (nextCol < 0 || nextCol >= PATH_COLS || nextRow < 0 || nextRow >= PATH_ROWS) continue;
            if (pathCellBlocked(nextCol, nextRow, radius)) continue;

            int next = gridIndex(nextCol, nextRow);
            if (cameFrom[next] != -1) continue;
            cameFrom[next] = current;
            queue.push_back(next);
        }
    }

    if (cameFrom[goal] == -1) return target;

    int next = goal;
    while (cameFrom[next] != start && cameFrom[next] != next) {
        next = cameFrom[next];
    }

    int nextCol = next % PATH_COLS;
    int nextRow = next / PATH_COLS;
    return gridCenter(nextCol, nextRow);
}

void moveMonsterAvoidingWalls(Vec2& pos, float radius, Vec2 desiredDir, float speed) {
    desiredDir = normalize(desiredDir);
    if (speed <= 0 || (desiredDir.x == 0 && desiredDir.y == 0)) return;

    const float angles[] = {
        0.0f, -25.0f, 25.0f, -45.0f, 45.0f, -70.0f, 70.0f,
        -90.0f, 90.0f, -120.0f, 120.0f, -150.0f, 150.0f, 180.0f
    };
    Vec2 bestDir = desiredDir;
    float bestScore = -999.0f;

    for (float angle : angles) {
        Vec2 candidate = normalize(rotate(desiredDir, angle));
        Vec2 next{ pos.x + candidate.x * speed, pos.y + candidate.y * speed };
        if (blocked(next, radius)) continue;

        bool hasLookAheadClearance = monsterStepClear(pos, radius, candidate, speed);
        float clearanceScore = hasLookAheadClearance ? 0.45f : -0.35f;
        float wallHugPenalty = blocked(next, radius + 4.0f) ? -0.45f : 0.0f;
        float score = dot(candidate, desiredDir) + clearanceScore + wallHugPenalty;
        if (score > bestScore) {
            bestScore = score;
            bestDir = candidate;
        }
    }

    if (bestScore < -100.0f) {
        bestDir = desiredDir;
    }

    moveWithCollision(pos, radius, { bestDir.x * speed, bestDir.y * speed });
}

void resetPlayer() {
    player = Player();
}

void addSupply(float x, float y) {
    supplies.push_back(Supply{ { x, y }, 12, true });
}

float randFloat(float minValue, float maxValue) {
    return minValue + (maxValue - minValue) * ((float)rand() / (float)RAND_MAX);
}

float playerShootCooldown() {
    if (player.bulletUpgraded) {
        return (float)(UPGRADED_PLAYER_SHOOT_COOLDOWN_MIN + rand() % UPGRADED_PLAYER_SHOOT_COOLDOWN_RANGE);
    }
    return (float)(PLAYER_SHOOT_COOLDOWN_MIN + rand() % PLAYER_SHOOT_COOLDOWN_RANGE);
}

float enemyShootCooldown() {
    return (float)ENEMY_SHOOT_COOLDOWN_FRAMES;
}

float bossShootCooldown() {
    return (float)BOSS_SHOOT_COOLDOWN_FRAMES;
}

float playerBulletDamage() {
    return player.bulletUpgraded ? UPGRADED_PLAYER_BULLET_DAMAGE : PLAYER_BULLET_DAMAGE;
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

void addRandomSupply() {
    Vec2 pos = randomOpenPosition(12.0f);
    addSupply(pos.x, pos.y);
}

void addLevelSupplies(int level) {
    if (gameDifficulty == GameDifficulty::Easy) {
        int count = (level == 1) ? 2 : 1;
        for (int i = 0; i < count; ++i) {
            addRandomSupply();
        }
    }
    else if (level <= 2) {
        addRandomSupply();
    }
}

void addMonster(MonsterType type, float x, float y, float hp, float speed) {
    Monster m;
    m.type = type;
    m.pos = { x, y };
    m.maxHp = hp;
    m.hp = hp;
    m.speed = speed;
    m.touchDamage = (type == MonsterType::Boss) ? BOSS_DAMAGE : MONSTER_DAMAGE;
    if (type == MonsterType::Boss) m.radius = 26.0f;
    else m.radius = (type == MonsterType::Shooter) ? 16.0f : 15.0f;
    m.shootCooldown = enemyShootCooldown();
    monsters.push_back(m);
}

void addHardLevel3Chaser(float x, float y, float speed) {
    Monster m;
    m.type = MonsterType::Chaser;
    m.pos = { x, y };
    m.maxHp = 1.5f;
    m.hp = 1.5f;
    m.speed = speed;
    m.touchDamage = 100;
    m.radius = 15.0f;
    m.shootCooldown = enemyShootCooldown();
    monsters.push_back(m);
}

void loadLevel(int level) {
    currentLevel = level;
    walls.clear();
    monsters.clear();
    supplies.clear();
    bullets.clear();
    slashes.clear();

    player.pos = { 90, 90 };
    player.skillEnergy = MAX_SKILL_ENERGY;
    player.attackCooldown = 0;
    player.shootCooldown = 0;
    player.hurtCooldown = 0;
    player.dashInvincible = 0;
    player.dashDir = { 0, 0 };
    spaceDownLast = false;

    if (level == 1) {
        walls.push_back({ 280, 120, 80, 260 });
        walls.push_back({ 560, 250, 90, 240 });
        walls.push_back({ 410, 500, 180, 40 });
        addMonster(MonsterType::Chaser, 760, 130, 2, 1.40f);
        addMonster(MonsterType::Chaser, 780, 470, 2, 1.45f);
        addMonster(MonsterType::Chaser, 470, 330, 2, 1.35f);
        addMonster(MonsterType::Shooter, 700, 315, 2, 0.95f);
        addLevelSupplies(level);
    }
    else if (level == 2) {
        walls.push_back({ 190, 180, 140, 55 });
        walls.push_back({ 190, 390, 140, 55 });
        walls.push_back({ 460, 100, 60, 210 });
        walls.push_back({ 460, 360, 60, 210 });
        walls.push_back({ 690, 225, 110, 170 });
        addMonster(MonsterType::Chaser, 785, 95, 3, 1.55f);
        addMonster(MonsterType::Chaser, 820, 525, 3, 1.60f);
        addMonster(MonsterType::Chaser, 550, 310, 3, 1.50f);
        addMonster(MonsterType::Shooter, 650, 310, 2, 0.98f);
        addMonster(MonsterType::Shooter, 820, 210, 2, 0.98f);
        addMonster(MonsterType::Shooter, 820, 430, 2, 0.98f);
        addLevelSupplies(level);
    }
    else {
        walls.push_back({ 155, 145, 72, 340 });
        walls.push_back({ 350, 92, 260, 52 });
        walls.push_back({ 350, 496, 260, 52 });
        walls.push_back({ 733, 145, 72, 340 });
        walls.push_back({ 365, 260, 55, 120 });
        walls.push_back({ 535, 260, 55, 120 });
        if (gameDifficulty == GameDifficulty::Hard) {
            addHardLevel3Chaser(825, 100, 1.70f);
            addHardLevel3Chaser(835, 535, 1.70f);
            addHardLevel3Chaser(280, 520, 1.65f);
        }
        else {
            addMonster(MonsterType::Chaser, 825, 100, 4, 1.70f);
            addMonster(MonsterType::Chaser, 835, 535, 4, 1.70f);
            addMonster(MonsterType::Chaser, 280, 520, 4, 1.65f);
        }
        addMonster(MonsterType::Shooter, 700, 320, 3, 1.05f);
        addMonster(MonsterType::Shooter, 300, 120, 3, 1.05f);
        addMonster(MonsterType::Shooter, 300, 520, 3, 1.05f);
        addMonster(MonsterType::Boss, 840, 320, 10, 0.80f);
        addLevelSupplies(level);
    }
}

void startGame() {
    resetPlayer();
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

bool localFileExists(const std::wstring& path) {
    DWORD attributes = GetFileAttributes(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring directoryOf(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return L"";
    return path.substr(0, slash);
}

std::wstring parentDirectoryOf(const std::wstring& directory) {
    if (directory.empty()) return L"";
    std::wstring trimmed = directory;
    while (!trimmed.empty() && (trimmed.back() == L'\\' || trimmed.back() == L'/')) trimmed.pop_back();
    return directoryOf(trimmed);
}

std::wstring combinePath(const std::wstring& directory, const std::wstring& fileName) {
    if (directory.empty()) return fileName;
    if (directory.back() == L'\\' || directory.back() == L'/') return directory + fileName;
    return directory + L"\\" + fileName;
}

void addPlayerSpriteCandidate(std::vector<std::wstring>& candidates, const std::wstring& directory) {
    if (directory.empty()) return;
    candidates.push_back(combinePath(combinePath(directory, L"assets"), L"player_apple.png"));
}

bool loadPlayerSprite() {
    if (playerSpriteLoadTried) return playerSpriteLoaded;
    playerSpriteLoadTried = true;

    wchar_t exePath[MAX_PATH]{};
    GetModuleFileName(nullptr, exePath, MAX_PATH);

    wchar_t currentPath[MAX_PATH]{};
    GetCurrentDirectory(MAX_PATH, currentPath);

    std::wstring exeDir = directoryOf(exePath);
    std::wstring currentDir = currentPath;
    std::wstring exeParent = parentDirectoryOf(exeDir);
    std::wstring currentMonsterDodgeDir = combinePath(currentDir, L"MonsterDodge");

    std::vector<std::wstring> candidates;
    addPlayerSpriteCandidate(candidates, currentDir);
    addPlayerSpriteCandidate(candidates, currentMonsterDodgeDir);
    addPlayerSpriteCandidate(candidates, exeDir);
    addPlayerSpriteCandidate(candidates, exeParent);

    for (const auto& candidate : candidates) {
        if (!localFileExists(candidate)) continue;
        loadimage(&playerSprite, candidate.c_str(), PLAYER_SPRITE_SIZE, PLAYER_SPRITE_SIZE, true);
        playerSpriteLoaded = playerSprite.getwidth() > 0 && playerSprite.getheight() > 0;
        if (playerSpriteLoaded) break;
    }

    return playerSpriteLoaded;
}

void drawAlphaImage(int x, int y, IMAGE* image) {
    DWORD* src = GetImageBuffer(image);
    DWORD* dst = GetImageBuffer();
    int width = image->getwidth();
    int height = image->getheight();

    for (int sy = 0; sy < height; ++sy) {
        int dy = y + sy;
        if (dy < 0 || dy >= SCREEN_H) continue;

        for (int sx = 0; sx < width; ++sx) {
            int dx = x + sx;
            if (dx < 0 || dx >= SCREEN_W) continue;

            DWORD source = src[sy * width + sx];
            int alpha = (source >> 24) & 0xff;
            if (alpha <= 0) continue;

            DWORD& target = dst[dy * SCREEN_W + dx];
            if (alpha >= 255) {
                target = RGB(GetRValue(source), GetGValue(source), GetBValue(source));
                continue;
            }

            int invAlpha = 255 - alpha;
            int r = (GetRValue(source) * alpha + GetRValue(target) * invAlpha) / 255;
            int g = (GetGValue(source) * alpha + GetGValue(target) * invAlpha) / 255;
            int b = (GetBValue(source) * alpha + GetBValue(target) * invAlpha) / 255;
            target = RGB(r, g, b);
        }
    }
}

void drawPlayerSprite(Vec2 aim) {
    setlinecolor(RGB(183, 244, 207));
    setlinestyle(PS_SOLID, 2);
    line((int)player.pos.x, (int)player.pos.y, (int)(player.pos.x + aim.x * 34), (int)(player.pos.y + aim.y * 34));
    setlinestyle(PS_SOLID, 1);

    if (player.dashInvincible > 0) {
        setlinecolor(RGB(91, 214, 210));
        setlinestyle(PS_SOLID, 3);
        circle((int)player.pos.x, (int)player.pos.y, 29);
        setlinestyle(PS_SOLID, 1);
    }

    if (loadPlayerSprite()) {
        int x = (int)(player.pos.x - PLAYER_SPRITE_SIZE / 2);
        int y = (int)(player.pos.y - PLAYER_SPRITE_SIZE / 2);
        drawAlphaImage(x, y, &playerSprite);
    }
    else {
        setfillcolor(player.dashInvincible > 0 ? RGB(91, 214, 210) : RGB(82, 184, 132));
        solidcircle((int)player.pos.x, (int)player.pos.y, (int)player.radius);
    }
}

void drawMechanicalFloor() {
    setbkcolor(RGB(18, 23, 28));
    cleardevice();

    for (int y = 0; y < SCREEN_H; y += 80) {
        for (int x = 0; x < SCREEN_W; x += 80) {
            COLORREF fill = ((x / 80 + y / 80) % 2 == 0) ? RGB(27, 34, 41) : RGB(23, 29, 35);
            setfillcolor(fill);
            solidrectangle(x, y, x + 80, y + 80);
            setlinecolor(RGB(42, 52, 60));
            rectangle(x, y, x + 80, y + 80);

            setfillcolor(RGB(68, 80, 90));
            solidcircle(x + 10, y + 10, 2);
            solidcircle(x + 70, y + 10, 2);
            solidcircle(x + 10, y + 70, 2);
            solidcircle(x + 70, y + 70, 2);
        }
    }

    setlinecolor(RGB(33, 40, 47));
    for (int x = -120; x < SCREEN_W; x += 160) {
        line(x, SCREEN_H, x + 220, 0);
    }

    setlinestyle(PS_SOLID, 5);
    setlinecolor(RGB(34, 111, 126));
    roundrect(28, 28, SCREEN_W - 28, SCREEN_H - 28, 10, 10);
    setlinestyle(PS_SOLID, 2);
    setlinecolor(RGB(74, 214, 228));
    roundrect(36, 36, SCREEN_W - 36, SCREEN_H - 36, 8, 8);

    setlinestyle(PS_SOLID, 8);
    setlinecolor(RGB(26, 74, 83));
    line(95, 300, 230, 300);
    line(230, 300, 230, 92);
    line(230, 92, 420, 92);
    line(540, 92, 710, 92);
    line(710, 92, 710, 300);
    line(710, 300, 865, 300);
    line(95, 430, 330, 430);
    line(330, 430, 330, 560);
    line(330, 560, 440, 560);
    line(520, 560, 640, 560);
    line(640, 560, 640, 430);
    line(640, 430, 865, 430);

    setlinestyle(PS_SOLID, 2);
    setlinecolor(RGB(73, 202, 215));
    line(95, 300, 230, 300);
    line(230, 300, 230, 92);
    line(230, 92, 420, 92);
    line(540, 92, 710, 92);
    line(710, 92, 710, 300);
    line(710, 300, 865, 300);
    line(95, 430, 330, 430);
    line(330, 430, 330, 560);
    line(330, 560, 440, 560);
    line(520, 560, 640, 560);
    line(640, 560, 640, 430);
    line(640, 430, 865, 430);

    setlinestyle(PS_SOLID, 1);
}

void drawHazardBand(int x1, int y1, int x2, int y2, bool horizontal) {
    setfillcolor(RGB(34, 32, 27));
    solidrectangle(x1, y1, x2, y2);

    setlinestyle(PS_SOLID, 4);
    setlinecolor(RGB(220, 176, 48));
    if (horizontal) {
        for (int x = x1 - 8; x < x2; x += 22) {
            line(x, y2, x + 12, y1);
        }
    }
    else {
        for (int y = y1 - 8; y < y2; y += 22) {
            line(x1, y + 12, x2, y);
        }
    }
    setlinestyle(PS_SOLID, 1);
}

void drawMechanicalWall(const RectF& wall) {
    int x1 = (int)wall.x;
    int y1 = (int)wall.y;
    int x2 = (int)(wall.x + wall.w);
    int y2 = (int)(wall.y + wall.h);

    setfillcolor(RGB(10, 13, 16));
    solidroundrect(x1 + 5, y1 + 7, x2 + 5, y2 + 7, 8, 8);

    setfillcolor(RGB(78, 87, 96));
    setlinecolor(RGB(142, 155, 165));
    solidroundrect(x1, y1, x2, y2, 8, 8);
    roundrect(x1, y1, x2, y2, 8, 8);

    setfillcolor(RGB(60, 70, 79));
    setlinecolor(RGB(100, 114, 125));
    solidroundrect(x1 + 8, y1 + 8, x2 - 8, y2 - 8, 5, 5);
    roundrect(x1 + 8, y1 + 8, x2 - 8, y2 - 8, 5, 5);

    if (wall.w >= wall.h) {
        drawHazardBand(x1 + 8, y1 + 6, x2 - 8, y1 + 14, true);
        drawHazardBand(x1 + 8, y2 - 14, x2 - 8, y2 - 6, true);
    }
    else {
        drawHazardBand(x1 + 6, y1 + 8, x1 + 14, y2 - 8, false);
        drawHazardBand(x2 - 14, y1 + 8, x2 - 6, y2 - 8, false);
    }

    setfillcolor(RGB(168, 180, 188));
    setlinecolor(RGB(38, 44, 50));
    solidcircle(x1 + 14, y1 + 14, 3);
    solidcircle(x2 - 14, y1 + 14, 3);
    solidcircle(x1 + 14, y2 - 14, 3);
    solidcircle(x2 - 14, y2 - 14, 3);
}

void drawHealthSupply(const Supply& supply) {
    int x = (int)supply.pos.x;
    int y = (int)supply.pos.y;

    setfillcolor(RGB(13, 16, 19));
    solidroundrect(x - 20, y - 16, x + 22, y + 18, 7, 7);
    setfillcolor(RGB(83, 45, 48));
    setlinecolor(RGB(170, 72, 74));
    solidroundrect(x - 22, y - 18, x + 20, y + 16, 7, 7);
    roundrect(x - 22, y - 18, x + 20, y + 16, 7, 7);

    setfillcolor(RGB(220, 75, 75));
    setlinecolor(RGB(255, 190, 190));
    solidroundrect(x - 13, y - 11, x + 11, y + 11, 4, 4);
    roundrect(x - 13, y - 11, x + 11, y + 11, 4, 4);

    setfillcolor(RGB(248, 248, 248));
    solidrectangle(x - 4, y - 9, x + 2, y + 9);
    solidrectangle(x - 10, y - 3, x + 8, y + 3);
}

void drawWorld() {
    drawMechanicalFloor();

    for (const auto& wall : walls) {
        drawMechanicalWall(wall);
    }

    for (const auto& s : supplies) {
        if (!s.alive) continue;
        drawHealthSupply(s);
    }

    for (const auto& b : bullets) {
        if (!b.alive) continue;
        setfillcolor(b.fromPlayer ? RGB(97, 202, 255) : RGB(169, 103, 235));
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
    drawPlayerSprite(aim);

    drawBar(18, 16, 180, 14, (float)player.hp, (float)player.maxHp, RGB(222, 78, 78));
    drawBar(18, 38, 180, 10, player.skillEnergy, MAX_SKILL_ENERGY, RGB(238, 190, 65));

    settextstyle(20, 0, L"Microsoft YaHei");
    settextcolor(RGB(230, 236, 240));
    setbkmode(TRANSPARENT);
    wchar_t info[128];
    swprintf_s(info, L"关卡 %d / 3    等级 %d    攻击 %d    剩余怪物 %d", currentLevel, player.level, player.attack, (int)monsters.size());
    outtextxy(220, 16, info);

    settextstyle(14, 0, L"Microsoft YaHei");
    settextcolor(RGB(230, 236, 240));
    wchar_t hpInfo[64];
    swprintf_s(hpInfo, L"血量 %d / %d", player.hp, player.maxHp);
    outtextxy(20, 17, hpInfo);
    wchar_t skillInfo[64];
    swprintf_s(skillInfo, L"技力 %.0f / %.0f", player.skillEnergy, MAX_SKILL_ENERGY);
    outtextxy(20, 33, skillInfo);
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
    drawStartBackground(L"游戏设置", L"调整背景音乐、音量和难度");
    RectF toggleButton{ 330, 212, 300, 46 };
    RectF minusButton{ 315, 296, 54, 44 };
    RectF plusButton{ 591, 296, 54, 44 };
    RectF easyButton{ 315, 392, 150, 46 };
    RectF hardButton{ 495, 392, 150, 46 };
    RectF backButton{ 315, 494, 150, 46 };
    RectF startButton{ 495, 494, 150, 46 };

    wchar_t musicLine[64];
    swprintf_s(musicLine, L"背景音乐：%s", musicManager.IsEnabled() ? L"开启" : L"关闭");
    drawMenuButton(toggleButton, musicLine, mouseInRect(mouse, toggleButton));

    wchar_t volumeLine[64];
    swprintf_s(volumeLine, L"音量：%d", musicManager.GetVolume());
    drawCenteredText(268, volumeLine, 22, RGB(230, 236, 240));
    drawMenuButton(minusButton, L"-", mouseInRect(mouse, minusButton));
    drawBar(385, 310, 190, 16, (float)musicManager.GetVolume(), 100, RGB(103, 211, 151));
    drawMenuButton(plusButton, L"+", mouseInRect(mouse, plusButton));

    drawCenteredText(362, L"难度", 22, RGB(230, 236, 240));
    drawMenuButton(easyButton, gameDifficulty == GameDifficulty::Easy ? L"简单：当前" : L"简单模式", mouseInRect(mouse, easyButton));
    drawMenuButton(hardButton, gameDifficulty == GameDifficulty::Hard ? L"困难：当前" : L"困难模式", mouseInRect(mouse, hardButton));

    if (musicManager.IsOpened()) {
        drawCenteredText(560, L"音乐文件已加载", 16, RGB(160, 220, 185));
    }
    else if (!musicManager.GetLastError().empty()) {
        std::wstring errorLine = L"音乐加载失败：" + musicManager.GetLastError();
        drawCenteredText(560, errorLine.c_str(), 16, RGB(235, 125, 116));
    }

    drawMenuButton(backButton, L"返回主菜单", mouseInRect(mouse, backButton));
    drawMenuButton(startButton, L"开始游戏", mouseInRect(mouse, startButton));
}

void drawControlsMenu(POINT mouse) {
    drawStartBackground(L"操作说明", L"熟悉按键后返回主菜单开始游戏");
    settextstyle(22, 0, L"Microsoft YaHei");
    settextcolor(RGB(230, 236, 240));
    setbkmode(TRANSPARENT);

    const wchar_t* lines[] = {
        L"WASD：移动",
        L"鼠标移动：瞄准",
        L"鼠标左键：近战攻击",
        L"鼠标右键：发射子弹",
        L"Space：冲刺 / 短暂无敌",
        L"Esc：游戏中打开暂停菜单",
        L"R：失败或胜利后重新开始"
    };
    for (int i = 0; i < 7; ++i) outtextxy(330, 225 + i * 35, lines[i]);

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
            RectF toggleButton{ 330, 212, 300, 46 };
            RectF minusButton{ 315, 296, 54, 44 };
            RectF plusButton{ 591, 296, 54, 44 };
            RectF easyButton{ 315, 392, 150, 46 };
            RectF hardButton{ 495, 392, 150, 46 };
            RectF backButton{ 315, 494, 150, 46 };
            RectF startButton{ 495, 494, 150, 46 };

            if (GetAsyncKeyState(VK_UP) & 0x0001) musicManager.SetVolume(musicManager.GetVolume() + 5);
            if (GetAsyncKeyState(VK_DOWN) & 0x0001) musicManager.SetVolume(musicManager.GetVolume() - 5);

            if (clicked) {
                if (mouseInRect(mouse, toggleButton)) musicManager.SetEnabled(!musicManager.IsEnabled());
                else if (mouseInRect(mouse, minusButton)) musicManager.SetVolume(musicManager.GetVolume() - 5);
                else if (mouseInRect(mouse, plusButton)) musicManager.SetVolume(musicManager.GetVolume() + 5);
                else if (mouseInRect(mouse, easyButton)) gameDifficulty = GameDifficulty::Easy;
                else if (mouseInRect(mouse, hardButton)) gameDifficulty = GameDifficulty::Hard;
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
    drawOverlay(L"关卡完成", currentLevel == 2 ? L"选择一项升级：1 攻击  2 延长闪避时间  3 子弹强化" : L"选择一项升级：1 攻击  2 延长闪避时间");
    settextstyle(20, 0, L"Microsoft YaHei");
    settextcolor(RGB(215, 224, 230));
    outtextxy(365, 370, L"1. 攻击力 +1");
    outtextxy(365, 405, L"2. 延长闪避时间 +4 帧");
    if (currentLevel == 2) {
        outtextxy(365, 440, L"3. 子弹伤害 1，间隔 20-25 帧");
    }
}

void applyUpgrade(int choice) {
    if (choice == 1) {
        player.attack += 1;
    }
    else if (choice == 2) {
        player.dashDuration += DASH_DURATION_UPGRADE_FRAMES;
    }
    else if (choice == 3 && currentLevel == 2) {
        player.bulletUpgraded = true;
        player.shootCooldown = 0;
    }

    player.level++;
    if (gameDifficulty == GameDifficulty::Easy) {
        player.hp = player.maxHp;
    }
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

    bool hitMonster = false;
    for (auto& m : monsters) {
        Vec2 toMonster{ m.pos.x - player.pos.x, m.pos.y - player.pos.y };
        float d = sqrtf(toMonster.x * toMonster.x + toMonster.y * toMonster.y);
        Vec2 toward = normalize(toMonster);
        if (d <= 78 && dot(toward, dir) > 0.35f) {
            hitMonster = true;
            m.hp -= player.attack;
            Vec2 knock = normalize({ m.pos.x - player.pos.x, m.pos.y - player.pos.y });
            moveWithCollision(m.pos, m.radius, { knock.x * MELEE_KNOCKBACK, knock.y * MELEE_KNOCKBACK });
        }
    }

    if (hitMonster) {
        player.hurtCooldown = std::max(player.hurtCooldown, (float)MELEE_HIT_INVINCIBLE_FRAMES);
    }
}

void playerShoot(Vec2 dir) {
    if (player.shootCooldown > 0) return;
    Vec2 spawn{ player.pos.x + dir.x * (player.radius + 8.0f), player.pos.y + dir.y * (player.radius + 8.0f) };
    bullets.push_back(Bullet{ spawn, { dir.x * PLAYER_BULLET_SPEED, dir.y * PLAYER_BULLET_SPEED }, 4, true, playerBulletDamage(), true });
    player.shootCooldown = playerShootCooldown();
}

void updatePlaying() {
    if (GetAsyncKeyState(VK_ESCAPE) & 0x0001) {
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

    bool spaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    if (spaceDown && !spaceDownLast && player.dashInvincible <= 0 && player.skillEnergy >= DASH_ENERGY_COST && (move.x != 0 || move.y != 0)) {
        player.skillEnergy -= DASH_ENERGY_COST;
        player.dashInvincible = player.dashDuration;
        player.dashDir = move;
    }
    spaceDownLast = spaceDown;

    float speed = player.speed;
    if (player.dashInvincible > 0) {
        speed *= DASH_SPEED_MULTIPLIER;
        move = player.dashDir;
    }
    else {
        player.skillEnergy = std::min(MAX_SKILL_ENERGY, player.skillEnergy + DASH_ENERGY_RECOVERY);
    }
    moveWithCollision(player.pos, player.radius, { move.x * speed, move.y * speed });

    bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    if (mouseDown && !mouseDownLast) {
        playerAttack(aim);
    }
    mouseDownLast = mouseDown;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
        playerShoot(aim);
    }

    if (player.attackCooldown > 0) player.attackCooldown -= 1;
    if (player.shootCooldown > 0) player.shootCooldown -= 1;
    if (player.hurtCooldown > 0) player.hurtCooldown -= 1;
    if (player.dashInvincible > 0) player.dashInvincible -= 1;

    for (auto& slash : slashes) slash.life -= 1;
    slashes.erase(std::remove_if(slashes.begin(), slashes.end(), [](const Slash& s) { return s.life <= 0; }), slashes.end());

    for (auto& m : monsters) {
        Vec2 dir = normalize({ player.pos.x - m.pos.x, player.pos.y - m.pos.y });
        if (m.type == MonsterType::Chaser) {
            Vec2 waypoint = chooseMonsterPathTarget(m.pos, m.radius, player.pos);
            Vec2 moveDir = normalize({ waypoint.x - m.pos.x, waypoint.y - m.pos.y });
            moveMonsterAvoidingWalls(m.pos, m.radius, moveDir, m.speed);
        }
        else if (m.type == MonsterType::Shooter) {
            float d = dist(m.pos, player.pos);
            if (!hasLineOfSight(m.pos, player.pos, m.radius)) {
                Vec2 waypoint = chooseMonsterPathTarget(m.pos, m.radius, player.pos);
                Vec2 moveDir = normalize({ waypoint.x - m.pos.x, waypoint.y - m.pos.y });
                moveMonsterAvoidingWalls(m.pos, m.radius, moveDir, m.speed * 0.6f);
            }
            else if (d < 180) {
                moveMonsterAvoidingWalls(m.pos, m.radius, { -dir.x, -dir.y }, m.speed);
            }

            m.shootCooldown -= 1;
            if (m.shootCooldown <= 0) {
                bullets.push_back(Bullet{ m.pos, { dir.x * SHOOTER_BULLET_SPEED, dir.y * SHOOTER_BULLET_SPEED }, 5, true, MONSTER_DAMAGE });
                m.shootCooldown = enemyShootCooldown();
            }
        }
        else {
            float d = dist(m.pos, player.pos);
            if (d > BOSS_CHASE_DISTANCE) {
                Vec2 waypoint = chooseMonsterPathTarget(m.pos, m.radius, player.pos);
                Vec2 moveDir = normalize({ waypoint.x - m.pos.x, waypoint.y - m.pos.y });
                moveMonsterAvoidingWalls(m.pos, m.radius, moveDir, m.speed);
            }
            else moveMonsterAvoidingWalls(m.pos, m.radius, { -dir.y, dir.x }, m.speed * 0.7f);

            m.shootCooldown -= 1;
            if (m.shootCooldown <= 0) {
                Vec2 shotDir = dir;
                if (shotDir.x == 0 && shotDir.y == 0) shotDir = { 1, 0 };
                Vec2 left = normalize(rotate(shotDir, -BOSS_SPREAD_ANGLE));
                Vec2 right = normalize(rotate(shotDir, BOSS_SPREAD_ANGLE));
                bullets.push_back(Bullet{ m.pos, { shotDir.x * BOSS_BULLET_SPEED, shotDir.y * BOSS_BULLET_SPEED }, BOSS_BULLET_RADIUS, true, BOSS_DAMAGE });
                bullets.push_back(Bullet{ m.pos, { left.x * BOSS_BULLET_SPEED, left.y * BOSS_BULLET_SPEED }, BOSS_BULLET_RADIUS, true, BOSS_DAMAGE });
                bullets.push_back(Bullet{ m.pos, { right.x * BOSS_BULLET_SPEED, right.y * BOSS_BULLET_SPEED }, BOSS_BULLET_RADIUS, true, BOSS_DAMAGE });
                m.shootCooldown = bossShootCooldown();
            }
        }

        if (dist(m.pos, player.pos) < m.radius + player.radius && player.hurtCooldown <= 0 && player.dashInvincible <= 0) {
            player.hp -= m.touchDamage;
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
    }

    for (size_t i = 0; i < bullets.size(); ++i) {
        if (!bullets[i].alive || !bullets[i].fromPlayer) continue;

        for (size_t j = 0; j < bullets.size(); ++j) {
            if (i == j || !bullets[j].alive || bullets[j].fromPlayer) continue;
            if (dist(bullets[i].pos, bullets[j].pos) < bullets[i].radius + bullets[j].radius) {
                bullets[i].alive = false;
                bullets[j].alive = false;
                break;
            }
        }
    }

    for (auto& b : bullets) {
        if (!b.alive) continue;

        if (b.fromPlayer) {
            for (auto& m : monsters) {
                if (m.hp <= 0) continue;
                if (dist(b.pos, m.pos) < b.radius + m.radius) {
                    b.alive = false;
                    m.hp -= b.damage;
                    break;
                }
            }
        }
        else if (dist(b.pos, player.pos) < b.radius + player.radius && player.hurtCooldown <= 0 && player.dashInvincible <= 0) {
            b.alive = false;
            player.hp -= (int)b.damage;
            player.hurtCooldown = 40;
        }
    }

    for (auto& s : supplies) {
        if (!s.alive) continue;
        if (dist(s.pos, player.pos) < s.radius + player.radius) {
            s.alive = false;
            player.hp = std::min(player.maxHp, player.hp + SUPPLY_RECOVERY);
        }
    }

    monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](const Monster& m) { return m.hp <= 0; }), monsters.end());
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) { return !b.alive; }), bullets.end());

    if (player.hp <= 0) state = GameState::GameOver;
    if (monsters.empty()) state = (currentLevel >= 3) ? GameState::Victory : GameState::LevelClear;
}

void drawFrame() {
    if (state == GameState::Playing) {
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
    if (state == GameState::Playing) {
        updatePlaying();
    }
    else if (state == GameState::LevelClear) {
        if (GetAsyncKeyState('1') & 0x0001) applyUpgrade(1);
        if (GetAsyncKeyState('2') & 0x0001) applyUpgrade(2);
        if (currentLevel == 2 && (GetAsyncKeyState('3') & 0x0001)) applyUpgrade(3);
    }
    else if (state == GameState::Paused) {
        updatePauseMenu();
    }
    else if (state == GameState::GameOver || state == GameState::Victory) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x0001) returnToStartMenuRequested = true;
        if (GetAsyncKeyState('R') & 0x0001) startGame();
    }
}

