#pragma once

constexpr int SCREEN_W = 960;
constexpr int SCREEN_H = 640;
constexpr float PI = 3.1415926f;

struct Vec2 {
    float x;
    float y;
};

struct RectF {
    float x;
    float y;
    float w;
    float h;
};

enum class GameState {
    Menu,
    Playing,
    LevelClear,
    GameOver,
    Victory,
    Paused
};

enum class MonsterType {
    Chaser,
    Shooter,
    Boss
};

enum class StartMenuScreen {
    MainMenu,
    Settings,
    Controls
};

enum class GameDifficulty {
    Easy,
    Hard
};

struct Player {
    Vec2 pos{ 100, 100 };
    float radius = 15;
    int maxHp = 100;
    int hp = 100;
    int level = 1;
    int attack = 1;
    float speed = 3.3f;
    float skillEnergy = 100;
    float attackCooldown = 0;
    float shootCooldown = 0;
    float hurtCooldown = 0;
    float dashInvincible = 0;
    bool bulletUpgraded = false;
};

struct Monster {
    MonsterType type = MonsterType::Chaser;
    Vec2 pos{};
    float radius = 15;
    float maxHp = 2;
    float hp = 2;
    float speed = 1.2f;
    int touchDamage = 10;
    float shootCooldown = 0;
};

struct Supply {
    Vec2 pos{};
    float radius = 12;
    bool alive = true;
};

struct Bullet {
    Vec2 pos{};
    Vec2 vel{};
    float radius = 5;
    bool alive = true;
    float damage = 10;
    bool fromPlayer = false;
};

struct Slash {
    Vec2 pos{};
    Vec2 dir{};
    float life = 8;
};
