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

struct Player {
    Vec2 pos{ 100, 100 };
    float radius = 15;
    int maxHp = 6;
    int hp = 6;
    int level = 1;
    int attack = 1;
    float speed = 3.3f;
    float dashEnergy = 100;
    float attackCooldown = 0;
    float hurtCooldown = 0;
    float dashInvincible = 0;
};

struct Monster {
    MonsterType type = MonsterType::Chaser;
    Vec2 pos{};
    float radius = 15;
    int maxHp = 2;
    int hp = 2;
    float speed = 1.2f;
    float shootCooldown = 0;
};

struct Supply {
    int type = 0; // 0 hp, 1 dash
    Vec2 pos{};
    float radius = 12;
    bool alive = true;
};

struct Bullet {
    Vec2 pos{};
    Vec2 vel{};
    float radius = 5;
    bool alive = true;
};

struct Slash {
    Vec2 pos{};
    Vec2 dir{};
    float life = 8;
};
