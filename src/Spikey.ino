/*
 * SPIKEY - A Spinosaurus Virtual Pet
 * By Benjamin Klein
 * For Arduino Uno R4 WiFi + ST7789 320x240 TFT
 * License: Spikey Source-Available Community License v1.0
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <EEPROM.h>
#include <math.h>

// Project identity metadata. Keep these notices intact; see LICENSE.md.
#define SPIKEY_VERSION          "2.1"
#define SPIKEY_LICENSE_NAME     "Spikey Source-Available Community License v1.0"
#define SPIKEY_COPYRIGHT_NOTICE "Copyright (c) 2026 Benjamin Klein and contributors"

// ============== HARDWARE ==============
#define TFT_CS    10
#define TFT_DC    9
#define TFT_RST   8
#define BTN_LEFT  2
#define BTN_SEL   3
#define BTN_RIGHT 4
#define BUZZER    5

#define SCREEN_W  320
#define SCREEN_H  240
#define STRIP_H   30
#define NUM_STRIPS (SCREEN_H / STRIP_H)
#define GROUND_Y  196

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 strip(SCREEN_W, STRIP_H);
int stripOffsetY = 0;

// ============== COLORS (RGB565) ==============
#define SKY_DAY_TOP     0x019F
#define SKY_DAY_MID     0x5D9F
#define SKY_DAY_HORIZON 0xEF7D
#define SKY_NIGHT_TOP   0x0820
#define SKY_NIGHT_MID   0x1041
#define SKY_NIGHT_HRZ   0x2882
#define SKY_SUNSET      0xFA40
#define SKY_SUNSET_DEEP 0xA980

#define SPINO_OUTLINE   0x2264
#define SPINO_DARK      0x4BA8
#define SPINO_MID       0x7D0D
#define SPINO_BODY      0x9DF3
#define SPINO_LIGHT     0xC6F8
#define SPINO_BELLY     0xEF7A
#define SPINO_SPIKE     0x6C6A

#define SAIL_ORANGE     0xFC80
#define SAIL_ORANGE_D   0xE300
#define SAIL_ORANGE_H   0xFEA0

#define EYE_WHITE       0xFFFF
#define EYE_PUPIL       0x0841
#define MOUTH           0x5100
#define TEETH           0xF7BE
#define TONGUE          0xFB0F

#define CLAW_LIGHT      0xF7BE
#define CLAW_SHADOW     0x8C51
#define CLAW_OUTLINE    0x31A6

#define MOUNTAIN_DARK   0x18C3
#define MOUNTAIN_MID    0x2965
#define MOUNTAIN_LIGHT  0x4228
#define MOUNTAIN_SNOW   0xF79E
#define TREE_TRUNK      0x4A22
#define TREE_D          0x1B05
#define TREE_M          0x3466
#define TREE_L          0x5DE8
#define GRASS_DARK      0x2484
#define GRASS_MID       0x3DA7
#define GRASS_LIGHT     0x56EA
#define GRASS_HIGH      0x7F6F
#define CLOUD_CORE      0xFFFF
#define CLOUD_EDGE      0xE73C
#define CLOUD_SHADOW    0xBDD7
#define SUN_COLOR       0xFFE0
#define SUN_GLOW        0xFD80
#define MOON_COLOR      0xEF7D
#define STAR_COLOR      0xFFFF

#define APPLE_RED       0xE0A0
#define APPLE_HIGH      0xFFD0
#define APPLE_SHADOW    0x9000
#define APPLE_LEAF      0x3C66
#define APPLE_STEM      0x6A44
#define WATER_BLUE      0x4C1F
#define WATER_LIGHT     0x7E7F
#define WATER_FOAM      0xFFFF

#define UI_BG           0x10C3
#define UI_BORDER       0xFFFF
#define UI_TEXT         0xFFFF
#define UI_HIGHLIGHT    0xFFE0
#define UI_DANGER       0xF800
#define UI_SUCCESS      0x07E0

#define POOP_DARK       0x2924
#define POOP_MID        0x4A42
#define POOP_LIGHT      0x6B43

#define STAT_HUNGER     0xFD60
#define STAT_JOY        0xFC1F
#define STAT_HEALTH     0x07E0
#define STAT_DISC       0x07FF
#define STAT_BG         0x18C3

// Pose constants (plain ints to avoid PlatformIO prototype issues)
#define POSE_WALK0  0
#define POSE_WALK1  1
#define POSE_WALK2  2
#define POSE_WALK3  3
#define POSE_WALK4  4
#define POSE_EAT1   5
#define POSE_EAT2   6
#define POSE_EAT3   7
#define POSE_EAT4   8
#define POSE_DRINK1 9
#define POSE_DRINK2 10
#define POSE_DRINK3 11
#define POSE_DRINK4 12
#define POSE_JUMP   13
#define POSE_SLEEP1 14
#define POSE_SLEEP2 15
#define POSE_DEAD   16

#define FOOD_SPLASH_MS 1800
#define MAX_POOPS 6

const unsigned long LIFE_CYCLE_MS = 12UL * 60UL * 60UL * 1000UL;
const unsigned long DAY_PHASE_MS  = LIFE_CYCLE_MS / 2UL;

const float APPLE_HUNGER_GAIN          = 20.0f;
const float FISH_HUNGER_GAIN           = 50.0f;
const float WATER_HEALTH_GAIN          = 10.0f;
const float PLAY_HAPPINESS_GAIN        = 20.0f;
const float TRAIN_DISCIPLINE_GAIN      = 10.0f;
const float DAILY_WATER_HEALTH_LOSS    = 50.0f;
const float DAILY_PLAY_HAPPINESS_LOSS  = 20.0f;
const float DAILY_TRAIN_DISCIPLINE_LOSS = 30.0f;
const float FOOD_PER_POOP              = 50.0f;
const unsigned long POOP_MIN_INTERVAL_MS = 60000UL;
const float WEIGHT_PER_FOOD_PERCENT    = 0.01f;
const float WEIGHT_LOSS_PER_POOP       = 0.2f;
const float WAKE_WEIGHT_LOSS           = 0.2f;
const float WEIGHT_SWEET_MIN           = 7.0f;
const float WEIGHT_SWEET_MAX           = 9.0f;
const float WEIGHT_FATAL_MAX           = 15.0f;
const float HUNGER_DECAY_PER_DAY_MS    = 100.0f / (3.0f * (float)DAY_PHASE_MS);

enum EndReason : uint8_t {
    END_REASON_NONE = 0,
    END_REASON_STARVATION,
    END_REASON_DEHYDRATION,
    END_REASON_LONELINESS,
    END_REASON_FILTH,
    END_REASON_WEIGHT,
    END_REASON_OLD_AGE,
    END_REASON_INJURY
};

// ============== STATE STRUCTS ==============
struct PetState {
    float hunger = 100, happiness = 100, health = 100, discipline = 100;
    float weight = 1, age = 0, poopometer = 0;
    int poops[MAX_POOPS] = {0, 0, 0, 0, 0, 0};
    bool sleeping = false, dead = false, victory = false, soundEnabled = true;
    bool wateredThisCycle = false, playedThisCycle = false, trainedThisCycle = false;
    bool warnedHunger = false, warnedJoy = false, warnedHealth = false;
    uint8_t deathReason = END_REASON_NONE;
    uint16_t completedCycles = 0;
    uint32_t cycleProgressMs = 0;
};

struct AnimState {
    int walkPos = 0;
    float walkX = 100, prevWalkX = 100;
    bool walkReverse = false, walkRight = true;
};

struct SceneState {
    float grassX = 0, prevGrassX = 0;
    float treesX = 0, prevTreesX = 0;
    float cloud1X = 300, prevCloud1X = 300;
    float cloud2X = 100, prevCloud2X = 100;
    float sunX = -20, prevSunX = -20;
    bool night = false;
    int stars[14][2];
    float sunsetBlend = 0;
};

enum RunnerObstacleType : uint8_t {
    RUNNER_OBS_NONE = 0,
    RUNNER_OBS_SMALL_CACTUS,
    RUNNER_OBS_LARGE_CACTUS,
    RUNNER_OBS_PTERA
};

struct RunnerObstacleState {
    bool active = false;
    uint8_t type = RUNNER_OBS_NONE;
    uint8_t variant = 0;
    float x = 0, prevX = 0, y = 0;
    int w = 0, h = 0;
};

struct RunnerPondState {
    bool active = false;
    float x = 0, prevX = 0;
    int width = 0, depth = 0;
};

struct GameState {
    bool active = false, paused = false, gameOver = false, newHiScore = false;
    bool jumping = false, ducking = false, inPond = false;
    int score = 0, hiScore = 0, level = 0, runStartHiScore = 0;
    int nextPondScore = 0;
    float jumpPos = 0, prevJumpPos = 0, jumpVel = 0;
    float runnerAccumMs = 0;
    RunnerObstacleState obstacle;
    RunnerPondState pond;
};

struct UIState {
    bool menuOpened = false, menuDepth = false;
    int menu = 0, subMenu = 1;
    int action = 0, setting = 0;
    bool notification = false;
    int animType = 0;
    int animItem = 0;
    unsigned long animStart = 0;
};

struct Button {
    bool curr = false, prev = false, pressed = false;
    unsigned long lastPress = 0;
    void update(bool raw) {
        prev = curr; curr = raw;
        pressed = curr && !prev && (millis() - lastPress >= 80);
        if (pressed) lastPress = millis();
    }
};

PetState   pet;
AnimState  anim;
SceneState scene;
GameState  game;
UIState    ui;
Button     btn1, btn2, btn3;

unsigned long lastUpdate = 0, lastMotionUpdate = 0, lastFrame = 0, lastSave = 0;
bool foodSplashBaseDrawn = false;
int foodSplashBaseItem = 0;
int foodSplashOverlayStep = -1;
bool foodSplashPrevRectValid = false;
int foodSplashPrevX = 0, foodSplashPrevY = 0, foodSplashPrevW = 0, foodSplashPrevH = 0;
bool deathScreenDrawn = false;
bool runnerNeedsFullRedraw = true;
unsigned long lastPoopMs = 0;

// ============== MENU ==============
#define MENU_COUNT  8
#define STR_SZ      10
#define FRAME_MS 20
#define RUNNER_TICK_MS 20.0f
#define RUNNER_FAST_TOP 84
#define RUNNER_FAST_BOTTOM 220
#define RUNNER_REF_START_SPEED 20.0f
#define RUNNER_REF_SCORE_STEP 100
#define RUNNER_SPEED_PIXEL_SCALE ((float)SCREEN_W / 1800.0f)
#define RUNNER_JUMP_VEL 3.6f
#define RUNNER_JUMP_GRAVITY 0.144f
#define RUNNER_JUMP_PIXELS_PER_VEL ((float)SCREEN_H / 600.0f * 2.0f)
#define RUNNER_POND_SLOW_FACTOR 0.80f
#define RUNNER_PLAYER_X 60
#define RUNNER_PLAYER_STAND_Y (GROUND_Y - 55)
#define RUNNER_PLAYER_DUCK_Y  (GROUND_Y - 43)
#define RUNNER_OBS_SPAWN_X    (SCREEN_W + 48)
#define RUNNER_POND_SAFE_GAP  96
#define RUNNER_POND_MIN_W     56
#define RUNNER_POND_MAX_W     108
#define RUNNER_POND_MIN_SCORE_GAP 180
#define RUNNER_POND_MAX_SCORE_GAP 320

const char menuItems[MENU_COUNT][8][STR_SZ] PROGMEM = {
    {"Food",   "Apple", "Fish", "Water", "", "", "", ""},
    {"Play",   "", "", "", "", "", "", ""},
    {"Sleep",  "", "", "", "", "", "", ""},
    {"Clean",  "", "", "", "", "", "", ""},
    {"Doctor", "", "", "", "", "", "", ""},
    {"Train",  "", "", "", "", "", "", ""},
    {"Stats",  "Hunger", "Joy", "Health", "Discip", "Weight", "Age", ""},
    {"Sound",  "Toggle", "", "", "", "", "", ""}
};

// ============== EEPROM ==============
#define SAVE_MAGIC 0xB3A5C4D3
#define LEGACY_SAVE_MAGIC 0xB3A5C4D2
#define SAVE_ADDR  0

struct LegacySaveData {
    uint32_t magic;
    float hunger, happiness, health, discipline, weight, age;
    int hiScore;
    bool soundEnabled;
    int poops[3];
    uint32_t checksum;
};

struct SaveData {
    uint32_t magic;
    float hunger, happiness, health, discipline, weight, age, poopometer;
    int hiScore;
    uint16_t completedCycles;
    uint32_t cycleProgressMs;
    uint8_t soundEnabled, sleeping, dead, victory;
    uint8_t deathReason;
    uint8_t wateredThisCycle, playedThisCycle, trainedThisCycle;
    int poops[MAX_POOPS];
    uint32_t checksum;
};

// ============== IMPLEMENTATIONS ==============

void resetPetProgress();
void renderFoodSplashAnimation(int item, unsigned long elapsed);
void fillFoodSplashBgRect(int x, int y, int w, int h, uint16_t top, uint16_t mid, uint16_t bot);
float getRunnerSpeedUnits();
float getRunnerPondMultiplier();
float getRunnerPixelsPerTick();
float getRunnerScrollSpeedPxPerMs();
void resetRunnerObstacle(RunnerObstacleState& obs);
void resetRunnerPond(RunnerPondState& pond);
float getRunnerObstacleRight(const RunnerObstacleState& obs);
float getRunnerPondRight(const RunnerPondState& pond);
void spawnRunnerObstacle();
void spawnRunnerPond();
void advanceRunnerWalkPose();
bool runnerRectsOverlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh);
void getRunnerPlayerRect(int& x, int& y, int& w, int& h);
void getRunnerObstacleHitRect(const RunnerObstacleState& obs, int& x, int& y, int& w, int& h);
bool runnerObstacleHitsPlayer();
void updateRunnerPondContact();
void crashRunnerGame();
void processRunnerTick();

uint32_t calcChecksumBytes(const void* data, size_t len) {
    uint32_t sum = 0;
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < len; i++) sum = (sum * 31) + p[i];
    return sum;
}

uint32_t calcChecksum(const SaveData* d) {
    return calcChecksumBytes(d, sizeof(SaveData) - sizeof(uint32_t));
}

uint32_t calcLegacyChecksum(const LegacySaveData* d) {
    return calcChecksumBytes(d, sizeof(LegacySaveData) - sizeof(uint32_t));
}

void saveState() {
    SaveData d = {};
    d.magic = SAVE_MAGIC;
    d.hunger = pet.hunger; d.happiness = pet.happiness;
    d.health = pet.health; d.discipline = pet.discipline;
    d.weight = pet.weight; d.age = pet.age; d.poopometer = pet.poopometer;
    d.hiScore = game.hiScore;
    d.completedCycles = pet.completedCycles;
    d.cycleProgressMs = pet.cycleProgressMs;
    d.soundEnabled = pet.soundEnabled ? 1 : 0;
    d.sleeping = pet.sleeping ? 1 : 0;
    d.dead = pet.dead ? 1 : 0;
    d.victory = pet.victory ? 1 : 0;
    d.deathReason = pet.deathReason;
    d.wateredThisCycle = pet.wateredThisCycle ? 1 : 0;
    d.playedThisCycle = pet.playedThisCycle ? 1 : 0;
    d.trainedThisCycle = pet.trainedThisCycle ? 1 : 0;
    for (int i = 0; i < MAX_POOPS; i++) d.poops[i] = pet.poops[i];
    d.checksum = calcChecksum(&d);
    EEPROM.put(SAVE_ADDR, d);
}

bool loadLegacyState() {
    LegacySaveData d = {};
    EEPROM.get(SAVE_ADDR, d);
    if (d.magic != LEGACY_SAVE_MAGIC) return false;
    if (calcLegacyChecksum(&d) != d.checksum) return false;

    pet.hunger = d.hunger; pet.happiness = d.happiness;
    pet.health = d.health; pet.discipline = d.discipline;
    pet.weight = d.weight;
    pet.age = d.age < 0 ? 0 : floorf(d.age);
    pet.poopometer = 0;
    pet.sleeping = false;
    pet.dead = false;
    pet.victory = false;
    pet.deathReason = END_REASON_NONE;
    pet.wateredThisCycle = false;
    pet.playedThisCycle = false;
    pet.trainedThisCycle = false;
    pet.completedCycles = (uint16_t)(pet.age * 5.0f);
    pet.cycleProgressMs = 0;
    game.hiScore = d.hiScore;
    pet.soundEnabled = d.soundEnabled;
    for (int i = 0; i < MAX_POOPS; i++) {
        pet.poops[i] = (i < 3) ? d.poops[i] : 0;
    }
    return true;
}

bool loadState() {
    SaveData d = {};
    EEPROM.get(SAVE_ADDR, d);
    if (d.magic == SAVE_MAGIC && calcChecksum(&d) == d.checksum) {
        game.hiScore = d.hiScore;
        pet.soundEnabled = d.soundEnabled != 0;
        if (d.dead != 0 || d.victory != 0) {
            resetPetProgress();
            saveState();
            return true;
        }

        pet.hunger = d.hunger; pet.happiness = d.happiness;
        pet.health = d.health; pet.discipline = d.discipline;
        pet.weight = d.weight; pet.age = d.age; pet.poopometer = d.poopometer;
        pet.completedCycles = d.completedCycles;
        pet.cycleProgressMs = d.cycleProgressMs % LIFE_CYCLE_MS;
        pet.sleeping = d.sleeping != 0;
        pet.dead = false;
        pet.victory = false;
        pet.deathReason = d.deathReason;
        pet.wateredThisCycle = d.wateredThisCycle != 0;
        pet.playedThisCycle = d.playedThisCycle != 0;
        pet.trainedThisCycle = d.trainedThisCycle != 0;
        for (int i = 0; i < MAX_POOPS; i++) pet.poops[i] = d.poops[i];
        return true;
    }
    if (!loadLegacyState()) return false;
    saveState();
    return true;
}

float getRunnerSpeedUnits() {
    return RUNNER_REF_START_SPEED + (float)(game.score / RUNNER_REF_SCORE_STEP);
}

float getRunnerPondMultiplier() {
    return game.inPond ? RUNNER_POND_SLOW_FACTOR : 1.0f;
}

float getRunnerPixelsPerTick() {
    return getRunnerSpeedUnits() * RUNNER_SPEED_PIXEL_SCALE * getRunnerPondMultiplier();
}

float getRunnerScrollSpeedPxPerMs() {
    return getRunnerPixelsPerTick() / RUNNER_TICK_MS;
}

void resetRunnerObstacle(RunnerObstacleState& obs) {
    obs = RunnerObstacleState();
}

void resetRunnerPond(RunnerPondState& pond) {
    pond = RunnerPondState();
}

float getRunnerObstacleRight(const RunnerObstacleState& obs) {
    return obs.x + obs.w;
}

float getRunnerPondRight(const RunnerPondState& pond) {
    return pond.x + pond.width;
}

void spawnRunnerObstacle() {
    RunnerObstacleState& obs = game.obstacle;
    resetRunnerObstacle(obs);

    int choice = random(0, 3);
    obs.active = true;
    obs.x = RUNNER_OBS_SPAWN_X;
    obs.prevX = obs.x;

    if (choice == 0) {
        obs.type = RUNNER_OBS_SMALL_CACTUS;
        obs.variant = (uint8_t)random(0, 3);
        obs.y = GROUND_Y - 18;
        obs.w = 24;
        obs.h = 18;
    } else if (choice == 1) {
        obs.type = RUNNER_OBS_LARGE_CACTUS;
        obs.variant = (uint8_t)random(0, 3);
        obs.y = GROUND_Y - 34;
        obs.w = 26;
        obs.h = 34;
    } else {
        static const int pteraHeights[3] = {124, 128, 132};
        obs.type = RUNNER_OBS_PTERA;
        obs.variant = (uint8_t)random(0, 3);
        obs.y = pteraHeights[obs.variant];
        obs.w = 34;
        obs.h = 18;
    }

    if (game.pond.active) {
        float minSafeSpawn = getRunnerPondRight(game.pond) + RUNNER_POND_SAFE_GAP;
        if (obs.x < minSafeSpawn) {
            obs.x = minSafeSpawn;
            obs.prevX = obs.x;
        }
    }
}

void spawnRunnerPond() {
    RunnerPondState& pond = game.pond;
    resetRunnerPond(pond);
    pond.active = true;
    pond.width = random(RUNNER_POND_MIN_W, RUNNER_POND_MAX_W + 1);
    pond.depth = random(7, 12);
    pond.x = SCREEN_W + random(36, 90);
    pond.prevX = pond.x;
    game.nextPondScore = game.score + random(RUNNER_POND_MIN_SCORE_GAP, RUNNER_POND_MAX_SCORE_GAP + 1);
}

void advanceRunnerWalkPose() {
    if (anim.walkReverse) {
        anim.walkPos--;
        if (anim.walkPos <= 0) { anim.walkPos = 0; anim.walkReverse = false; }
    } else {
        anim.walkPos++;
        if (anim.walkPos >= 4) { anim.walkPos = 4; anim.walkReverse = true; }
    }
}

bool runnerRectsOverlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

void getRunnerPlayerRect(int& x, int& y, int& w, int& h) {
    if (game.ducking && !game.jumping) {
        x = RUNNER_PLAYER_X + 34;
        y = GROUND_Y - 18 + (game.inPond ? 3 : 0);
        w = 40;
        h = 15;
    } else {
        x = RUNNER_PLAYER_X + 32;
        y = GROUND_Y - 39 - (int)game.jumpPos + ((game.inPond && !game.jumping) ? 3 : 0);
        w = 34;
        h = 34;
    }
}

void getRunnerObstacleHitRect(const RunnerObstacleState& obs, int& x, int& y, int& w, int& h) {
    x = (int)obs.x;
    y = (int)obs.y;
    w = obs.w;
    h = obs.h;

    if (obs.type == RUNNER_OBS_SMALL_CACTUS) {
        x += 3;
        y += 4;
        w -= 6;
        h -= 5;
    } else if (obs.type == RUNNER_OBS_LARGE_CACTUS) {
        x += 6;
        y += 6;
        w -= 11;
        h -= 9;
    } else if (obs.type == RUNNER_OBS_PTERA) {
        x += 5;
        y += 4;
        w -= 10;
        h -= 8;
    }
}

bool runnerObstacleHitsPlayer() {
    if (!game.obstacle.active) return false;

    int playerX, playerY, playerW, playerH;
    int obsX, obsY, obsW, obsH;
    getRunnerObstacleHitRect(game.obstacle, obsX, obsY, obsW, obsH);

    if (game.obstacle.type == RUNNER_OBS_PTERA) {
        if (game.ducking && !game.jumping) return false;
        playerX = RUNNER_PLAYER_X + 52;
        playerY = GROUND_Y - 64 - (int)game.jumpPos;
        playerW = 54;
        playerH = 38;
    } else {
        getRunnerPlayerRect(playerX, playerY, playerW, playerH);
    }

    if (!runnerRectsOverlap(playerX, playerY, playerW, playerH, obsX, obsY, obsW, obsH)) {
        return false;
    }

    if (game.obstacle.type != RUNNER_OBS_PTERA && game.jumping) {
        int footY = GROUND_Y - (int)game.jumpPos;
        if (footY <= obsY + 4) return false;
    }

    return true;
}

void updateRunnerPondContact() {
    game.inPond = false;
    if (!game.pond.active || game.jumping) return;

    int footLeft = RUNNER_PLAYER_X + 28;
    int footRight = RUNNER_PLAYER_X + 58;
    if (game.pond.x < footRight && getRunnerPondRight(game.pond) > footLeft) {
        game.inPond = true;
    }
}

uint16_t lerp565(uint16_t c1, uint16_t c2, float t) {
    if (t <= 0) return c1;
    if (t >= 1) return c2;
    int r1 = (c1 >> 11) & 0x1F, g1 = (c1 >> 5) & 0x3F, b1 = c1 & 0x1F;
    int r2 = (c2 >> 11) & 0x1F, g2 = (c2 >> 5) & 0x3F, b2 = c2 & 0x1F;
    int r = r1 + (int)((r2 - r1) * t);
    int g = g1 + (int)((g2 - g1) * t);
    int b = b1 + (int)((b2 - b1) * t);
    return (r << 11) | (g << 5) | b;
}

int W2L(int wy) { return wy - stripOffsetY; }
bool inStrip(int wy, int h) { return (wy + h > stripOffsetY) && (wy < stripOffsetY + STRIP_H); }

void drawPixW(int x, int wy, uint16_t c) {
    int ly = W2L(wy);
    if (ly >= 0 && ly < STRIP_H && x >= 0 && x < SCREEN_W) strip.drawPixel(x, ly, c);
}

void fillRectW(int x, int wy, int w, int h, uint16_t c) { strip.fillRect(x, W2L(wy), w, h, c); }
void drawRectW(int x, int wy, int w, int h, uint16_t c) { strip.drawRect(x, W2L(wy), w, h, c); }
void drawLineW(int x0, int wy0, int x1, int wy1, uint16_t c) { strip.drawLine(x0, W2L(wy0), x1, W2L(wy1), c); }
void fillTriW(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) { strip.fillTriangle(x0, W2L(y0), x1, W2L(y1), x2, W2L(y2), c); }
void fillCircW(int cx, int cy, int r, uint16_t c) { strip.fillCircle(cx, W2L(cy), r, c); }
void drawCircW(int cx, int cy, int r, uint16_t c) { strip.drawCircle(cx, W2L(cy), r, c); }
void setCursorW(int x, int wy) { strip.setCursor(x, W2L(wy)); }
float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

void fillEllipseW(int cx, int cy, int rx, int ry, uint16_t color) {
    if (rx <= 0 || ry <= 0) return;
    if (!inStrip(cy - ry, 2 * ry + 1)) return;
    float ry2 = (float)ry * ry;
    for (int dy = -ry; dy <= ry; dy++) {
        int ly = W2L(cy + dy);
        if (ly < 0 || ly >= STRIP_H) continue;
        float f = 1.0f - (float)(dy * dy) / ry2;
        if (f < 0) f = 0;
        int w = (int)(rx * sqrtf(f));
        if (w < 0) w = 0;
        strip.drawFastHLine(cx - w, ly, 2 * w + 1, color);
    }
}

void drawEllipseW(int cx, int cy, int rx, int ry, uint16_t color) {
    if (rx <= 0 || ry <= 0) return;
    float ry2 = (float)ry * ry;
    float rx2 = (float)rx * rx;
    for (int dy = -ry; dy <= ry; dy++) {
        float f = 1.0f - (float)(dy * dy) / ry2;
        if (f < 0) f = 0;
        int w = (int)(rx * sqrtf(f));
        drawPixW(cx + w, cy + dy, color);
        if (w > 0) drawPixW(cx - w, cy + dy, color);
    }
    for (int dx = -rx; dx <= rx; dx++) {
        float f = 1.0f - (float)(dx * dx) / rx2;
        if (f < 0) f = 0;
        int h = (int)(ry * sqrtf(f));
        drawPixW(cx + dx, cy + h, color);
        if (h > 0) drawPixW(cx + dx, cy - h, color);
    }
}

void tftFillEllipse(int cx, int cy, int rx, int ry, uint16_t color) {
    if (rx <= 0 || ry <= 0) return;
    float ry2 = (float)ry * ry;
    for (int dy = -ry; dy <= ry; dy++) {
        float f = 1.0f - (float)(dy * dy) / ry2;
        if (f < 0) f = 0;
        int w = (int)(rx * sqrtf(f));
        tft.drawFastHLine(cx - w, cy + dy, 2 * w + 1, color);
    }
}

uint16_t computeSkyColor(int y) {
    float t = y / 239.0f;
    uint16_t base;
    if (scene.night) {
        base = (t < 0.5f) ? lerp565(SKY_NIGHT_TOP, SKY_NIGHT_MID, t * 2.0f)
                          : lerp565(SKY_NIGHT_MID, SKY_NIGHT_HRZ, (t - 0.5f) * 2.0f);
    } else {
        base = (t < 0.5f) ? lerp565(SKY_DAY_TOP, SKY_DAY_MID, t * 2.0f)
                          : lerp565(SKY_DAY_MID, SKY_DAY_HORIZON, (t - 0.5f) * 2.0f);
    }
    if (scene.sunsetBlend > 0.01f && !scene.night) {
        uint16_t sunset = (t < 0.5f) ? lerp565(SKY_SUNSET_DEEP, SKY_SUNSET, t * 2.0f)
                                      : lerp565(SKY_SUNSET, 0xFFE0, (t - 0.5f) * 2.0f);
        base = lerp565(base, sunset, scene.sunsetBlend);
    }
    return base;
}

void renderSky() {
    uint16_t* buf = strip.getBuffer();
    for (int row = 0; row < STRIP_H; row++) {
        uint16_t color = computeSkyColor(stripOffsetY + row);
        for (int x = 0; x < SCREEN_W; x++) buf[row * SCREEN_W + x] = color;
    }
}

void renderSun(float sunX) {
    int cx = (int)sunX;
    int cy = 35 + (int)(18 * sinf(((cx + 20.0f) / (SCREEN_W + 40.0f)) * 3.14159f));
    if (!scene.night) {
        if (inStrip(cy - 18, 36)) {
            fillCircW(cx, cy, 15, lerp565(SUN_GLOW, computeSkyColor(cy), 0.5f));
            fillCircW(cx, cy, 12, SUN_GLOW);
            fillCircW(cx, cy, 9, SUN_COLOR);
            fillCircW(cx - 2, cy - 2, 4, 0xFFFC);
        }
    } else {
        if (inStrip(cy - 12, 24)) {
            fillCircW(cx, cy, 10, MOON_COLOR);
            fillCircW(cx - 4, cy - 2, 9, computeSkyColor(cy));
            drawPixW(cx + 2, cy + 3, 0xCE79);
            drawPixW(cx - 1, cy + 5, 0xCE79);
            drawPixW(cx + 4, cy - 1, 0xCE79);
        }
    }
}

void renderStars() {
    if (!scene.night) return;
    unsigned long tm = millis();
    for (int i = 0; i < 14; i++) {
        int sx = scene.stars[i][0];
        int sy = scene.stars[i][1];
        if (inStrip(sy - 1, 3)) {
            drawPixW(sx, sy, STAR_COLOR);
            int twinkle = (tm / 200 + i * 7) % 4;
            if (twinkle == 0) {
                drawPixW(sx + 1, sy, 0xBDF7);
                drawPixW(sx - 1, sy, 0xBDF7);
                drawPixW(sx, sy + 1, 0xBDF7);
                drawPixW(sx, sy - 1, 0xBDF7);
            }
        }
    }
}

void renderCloud(float cloudX, int cloudY, int size) {
    int cx = (int)cloudX;
    if (cx < -size - 20 || cx > SCREEN_W + size + 20) return;
    if (!inStrip(cloudY - size / 2 - 3, size + 10)) return;
    fillCircW(cx - size / 2, cloudY + 2, size / 3, CLOUD_SHADOW);
    fillCircW(cx + size / 2, cloudY + 2, size / 3, CLOUD_SHADOW);
    fillCircW(cx, cloudY + 4, size / 2, CLOUD_SHADOW);
    fillCircW(cx - size / 2, cloudY, size / 3, CLOUD_EDGE);
    fillCircW(cx + size / 2, cloudY, size / 3, CLOUD_EDGE);
    fillCircW(cx, cloudY - 2, size / 2, CLOUD_EDGE);
    fillCircW(cx - size / 2 + 1, cloudY - 1, size / 3 - 2, CLOUD_CORE);
    fillCircW(cx + size / 2 - 1, cloudY - 1, size / 3 - 2, CLOUD_CORE);
    fillCircW(cx, cloudY - 3, size / 2 - 2, CLOUD_CORE);
}

void renderClouds(float c1, float c2) {
    if (game.active && !game.gameOver) return;
    renderCloud(c1, 35, 45);
    renderCloud(c2, 60, 32);
}

void renderMountains() {
    static const int peaks[4][3] = {
        {50, 95, 120}, {150, 75, 140}, {240, 90, 130}, {310, 105, 100}
    };
    int groundY = 170;
    if (!inStrip(70, groundY - 70)) return;
    for (int i = 0; i < 4; i++) {
        int px = peaks[i][0], py = peaks[i][1], pw = peaks[i][2];
        fillTriW(px - pw / 2, groundY, px - 3, py + 2, px + 2, groundY, MOUNTAIN_DARK);
        fillTriW(px - 2, groundY, px, py, px + pw / 2, groundY, MOUNTAIN_MID);
        fillTriW(px, py, px + pw / 5, groundY, px + pw / 2, groundY, MOUNTAIN_LIGHT);
        fillTriW(px - 10, py + 12, px, py, px + 12, py + 15, MOUNTAIN_SNOW);
    }
}

void renderTrees(float offsetX) {
    int treeY = 175;
    if (!inStrip(treeY - 30, 50)) return;
    for (int i = 0; i < 12; i++) {
        int bx = i * 40;
        int tx = ((bx + (int)offsetX) % (SCREEN_W + 80)) - 40;
        if (tx < -20 || tx > SCREEN_W + 20) continue;
        fillRectW(tx - 2, treeY + 3, 5, 12, TREE_TRUNK);
        fillRectW(tx - 1, treeY + 3, 3, 12, lerp565(TREE_TRUNK, 0, 0.3f));
        fillCircW(tx, treeY - 3, 13, TREE_D);
        fillCircW(tx - 3, treeY - 7, 10, TREE_M);
        fillCircW(tx + 3, treeY - 5, 8, TREE_M);
        fillCircW(tx - 1, treeY - 11, 7, TREE_L);
        fillCircW(tx + 4, treeY - 8, 5, TREE_L);
    }
}

void renderGrass(float offsetX) {
    if (!inStrip(GROUND_Y, SCREEN_H - GROUND_Y)) return;
    fillRectW(0, GROUND_Y,      SCREEN_W, 2,  GRASS_DARK);
    fillRectW(0, GROUND_Y + 2,  SCREEN_W, 3,  GRASS_MID);
    fillRectW(0, GROUND_Y + 5,  SCREEN_W, 5,  GRASS_LIGHT);
    fillRectW(0, GROUND_Y + 10, SCREEN_W, SCREEN_H - GROUND_Y - 10, GRASS_HIGH);
    int ox = (int)offsetX;
    for (int i = 0; i < 32; i++) {
        int bx = ((i * 11) + ox) % SCREEN_W;
        if (bx < 0) bx += SCREEN_W;
        int by = GROUND_Y + ((i * 3) & 3);
        drawLineW(bx, by, bx, by + 5, GRASS_DARK);
    }
    for (int i = 0; i < 24; i++) {
        int bx = ((i * 15) + ox / 2 + 7) % SCREEN_W;
        if (bx < 0) bx += SCREEN_W;
        int by = GROUND_Y + 3 + ((i * 5) & 2);
        drawLineW(bx, by, bx, by + 4, GRASS_HIGH);
    }
}

void renderRunnerGround(float offsetX) {
    (void)offsetX;
    if (!inStrip(GROUND_Y, 24)) return;

    fillRectW(0, GROUND_Y,      SCREEN_W, 2,  GRASS_DARK);
    fillRectW(0, GROUND_Y + 2,  SCREEN_W, 3,  GRASS_MID);
    fillRectW(0, GROUND_Y + 5,  SCREEN_W, 5,  GRASS_LIGHT);
    fillRectW(0, GROUND_Y + 10, SCREEN_W, 14, GRASS_HIGH);
}

void renderPoops() {
    int py = GROUND_Y + 8;
    for (int i = 0; i < MAX_POOPS; i++) {
        if (pet.poops[i] > 0) {
            int px = (pet.poops[i] * SCREEN_W) / 160;
            if (inStrip(py - 8, 12)) {
                fillCircW(px, py, 7, POOP_DARK);
                fillCircW(px - 1, py - 2, 6, POOP_MID);
                fillCircW(px - 1, py - 5, 4, POOP_MID);
                fillCircW(px - 2, py - 4, 2, POOP_LIGHT);
            }
        }
    }
}

// ============== SPINOSAURUS (rasterized from reference) ==============
void drawSpikey(int wx, int wy, int pose, bool facingRight) {
    if (!inStrip(wy - 24, 98)) return;

    int dir = facingRight ? 1 : -1;
    int bodyCx = wx + 40;
    int bodyCy = wy + 28;

    uint16_t cOutline = SPINO_OUTLINE;
    uint16_t cDark = SPINO_DARK;
    uint16_t cMid = SPINO_MID;
    uint16_t cBody = SPINO_BODY;
    uint16_t cLight = SPINO_LIGHT;
    uint16_t cBelly = SPINO_BELLY;
    uint16_t cSailO = SAIL_ORANGE;
    uint16_t cSailOD = SAIL_ORANGE_D;
    uint16_t cSailOH = SAIL_ORANGE_H;
    uint16_t cEyeBlue = 0x04BF;
    uint16_t cClaw = 0x1082;
    uint16_t cJaw = 0xB5B6;

    bool sleeping = (pose == POSE_SLEEP1 || pose == POSE_SLEEP2);
    if (sleeping) {
        cOutline = lerp565(cOutline, 0x001F, 0.3f);
        cDark = lerp565(cDark, 0x001F, 0.3f);
        cMid = lerp565(cMid, 0x001F, 0.3f);
        cBody = lerp565(cBody, 0x001F, 0.3f);
        cLight = lerp565(cLight, 0x001F, 0.3f);
        cBelly = lerp565(cBelly, 0x001F, 0.3f);
        cSailO = lerp565(cSailO, 0x001F, 0.3f);
        cSailOD = lerp565(cSailOD, 0x001F, 0.3f);
        cSailOH = lerp565(cSailOH, 0x001F, 0.3f);
    }

    int leg1Off = 0, leg2Off = 0, headOffY = 0, jumpLift = 0, mouthOpen = 0;
    bool eatingPose = (pose == POSE_EAT1 || pose == POSE_EAT2 || pose == POSE_EAT3 || pose == POSE_EAT4);
    switch (pose) {
        case POSE_WALK1: leg1Off = 4;  leg2Off = -4; break;
        case POSE_WALK2: leg1Off = 7;  leg2Off = -6; break;
        case POSE_WALK3: break;
        case POSE_WALK4: leg1Off = -6; leg2Off = 7;  break;
        case POSE_EAT1:  headOffY = 8;  mouthOpen = 3; break;
        case POSE_EAT2:  headOffY = 18; mouthOpen = 5; break;
        case POSE_EAT3:  headOffY = 24; mouthOpen = 6; break;
        case POSE_EAT4:  headOffY = 12; mouthOpen = 2; break;
        case POSE_DRINK1: case POSE_DRINK2:
        case POSE_DRINK3: case POSE_DRINK4:
            headOffY = 22; mouthOpen = 3; break;
        case POSE_JUMP:  jumpLift = 6;  leg1Off = -8; leg2Off = -6; break;
        default: break;
    }

    int curBodyCy = bodyCy - jumpLift;

    // DEAD
    if (pose == POSE_DEAD) {
        int y = GROUND_Y - 5;
        fillEllipseW(bodyCx, y, 35, 8, cOutline);
        fillEllipseW(bodyCx, y - 1, 33, 7, cDark);
        fillEllipseW(bodyCx, y - 2, 30, 5, cBody);
        fillRectW(bodyCx - 12, y - 15, 4, 14, cDark);
        fillRectW(bodyCx + 8, y - 17, 4, 16, cDark);
        strip.setTextSize(2); strip.setTextColor(UI_DANGER);
        setCursorW(bodyCx + dir*25, y - 5); strip.print("X");
        strip.setTextSize(1);
        return;
    }

    // SLEEPING on ground
    if (sleeping) {
        int sCy = GROUND_Y - 8;
        fillEllipseW(bodyCx, sCy + 1, 38, 7, cOutline);
        fillEllipseW(bodyCx, sCy, 36, 6, cDark);
        fillEllipseW(bodyCx + dir*2, sCy - 1, 32, 5, cBody);
        fillEllipseW(bodyCx + dir*4, sCy + 1, 26, 3, cBelly);
        int sleepSailBase = sCy - 4;
        fillTriW(bodyCx - 18, sleepSailBase, bodyCx + 18, sleepSailBase, bodyCx, sleepSailBase - 16, cSailOH);
        fillTriW(bodyCx - 15, sleepSailBase, bodyCx + 15, sleepSailBase, bodyCx, sleepSailBase - 13, cSailO);
        for (int i = -2; i <= 2; i++) {
            int px = bodyCx + i * 7;
            int py = sleepSailBase - 10 - (2 - abs(i));
            fillCircW(px, py + 3, 4, cSailO);
            drawLineW(px, sleepSailBase, px, py + 2, cSailOD);
        }
        fillEllipseW(bodyCx - dir*18, sCy - 1, 12, 5, cMid);
        fillEllipseW(bodyCx - dir*18, sCy - 2, 11, 4, cLight);
        fillEllipseW(bodyCx - dir*26, sCy + 1, 9, 3, cMid);
        drawLineW(bodyCx - dir*22, sCy, bodyCx - dir*28, sCy, cDark);
        drawLineW(bodyCx - dir*23, sCy + 2, bodyCx - dir*29, sCy + 2, cDark);
        fillEllipseW(bodyCx + dir*26, sCy + 2, 16, 4, cMid);
        fillEllipseW(bodyCx + dir*32, sCy + 3, 10, 3, cDark);
        fillEllipseW(bodyCx - 4, sCy + 5, 6, 2, cDark);
        fillEllipseW(bodyCx + 8, sCy + 5, 6, 2, cDark);
        strip.setTextSize(2); strip.setTextColor(UI_TEXT);
        int zX = facingRight ? bodyCx + 24 : bodyCx - 44;
        int zY = sCy - 22 + (pose == POSE_SLEEP2 ? -2 : 0);
        setCursorW(zX, zY); strip.print("Z");
        strip.setTextSize(1);
        setCursorW(zX + 14, zY + 4); strip.print("z");
        return;
    }

    // ======= STANDING / WALKING / EATING =======

    // ---- SAIL (large scalloped orange fan from reference) ----
    int sailBaseY = curBodyCy - 6;
    int sailCX = bodyCx - dir * 3;
    int sailLeft = sailCX - 26;
    int sailRight = sailCX + 26;
    int sailTop = sailBaseY - 30;

    fillTriW(sailLeft - 1, sailBaseY + 1, sailRight + 1, sailBaseY + 1, sailCX, sailTop - 2, cSailOH);
    fillTriW(sailLeft + 2, sailBaseY, sailRight - 2, sailBaseY, sailCX, sailTop + 2, cSailO);
    fillTriW(sailLeft + 8, sailBaseY, sailRight - 8, sailBaseY, sailCX, sailTop + 6, cSailOD);

    const int sailPeaks[6] = {18, 25, 30, 28, 22, 15};
    for (int i = 0; i < 6; i++) {
        int px = sailLeft + 5 + i * 9;
        int py = sailBaseY - sailPeaks[i];
        fillCircW(px, py + 5, 7, cSailOH);
        fillCircW(px, py + 6, 5, cSailO);
        fillTriW(px - 8, sailBaseY, px + 8, sailBaseY, px, py, cSailO);
        fillTriW(px - 3, sailBaseY - 1, px + 3, sailBaseY - 1, px, py + 6, cSailOD);
        drawLineW(px, sailBaseY - 1, px, py + 5, cSailOD);
        drawPixW(px - 2, py + 4, cSailOH);
    }
    drawLineW(sailLeft + 3, sailBaseY, sailRight - 3, sailBaseY, cSailOD);

    // ---- TAIL (long smooth taper rising slightly like the reference) ----
    fillTriW(bodyCx - dir * 24, curBodyCy - 3,
             bodyCx - dir * 78, curBodyCy - 10,
             bodyCx - dir * 26, curBodyCy + 9, cOutline);
    fillTriW(bodyCx - dir * 25, curBodyCy - 4,
             bodyCx - dir * 73, curBodyCy - 8,
             bodyCx - dir * 27, curBodyCy + 7, cMid);
    fillTriW(bodyCx - dir * 28, curBodyCy - 5,
             bodyCx - dir * 64, curBodyCy - 7,
             bodyCx - dir * 31, curBodyCy + 1, cLight);
    fillTriW(bodyCx - dir * 30, curBodyCy + 4,
             bodyCx - dir * 67, curBodyCy - 4,
             bodyCx - dir * 33, curBodyCy + 7, cBelly);

    // ---- BODY ----
    fillEllipseW(bodyCx, curBodyCy + 4, 36, 14, cOutline);
    fillEllipseW(bodyCx, curBodyCy + 3, 34, 13, cDark);
    fillEllipseW(bodyCx, curBodyCy + 1, 32, 12, cBody);
    fillEllipseW(bodyCx + dir * 2, curBodyCy + 8, 27, 5, cBelly);
    fillEllipseW(bodyCx - dir * 4, curBodyCy - 5, 23, 4, cLight);
    drawLineW(bodyCx - dir * 19, curBodyCy - 5, bodyCx + dir * 17, curBodyCy - 9, lerp565(cLight, 0xFFFF, 0.3f));

    // ---- NECK AND CREAM THROAT ----
    int headX = bodyCx + dir * 41;
    int headY = curBodyCy - 9 + headOffY;
    int neck1X = bodyCx + dir * 22;
    int neck1Y = curBodyCy - 4 + headOffY / 3;
    int neck2X = bodyCx + dir * 31;
    int neck2Y = curBodyCy - 8 + headOffY / 2;

    fillEllipseW(neck1X, neck1Y + 1, 13, 8, cOutline);
    fillEllipseW(neck1X, neck1Y, 12, 7, cMid);
    fillEllipseW(neck1X, neck1Y - 2, 10, 4, cLight);
    fillEllipseW(neck1X + dir * 2, neck1Y + 5, 8, 3, cBelly);
    fillEllipseW(neck2X, neck2Y + 1, 12, 8, cOutline);
    fillEllipseW(neck2X, neck2Y, 11, 7, cMid);
    fillEllipseW(neck2X - dir * 1, neck2Y - 2, 9, 4, cLight);
    fillEllipseW(neck2X + dir * 3, neck2Y + 5, 7, 3, cBelly);

    // ---- HEAD AND LONG CROCODILE SNOUT ----
    fillEllipseW(headX - dir * 4, headY - 3, 13, 9, cOutline);
    fillEllipseW(headX - dir * 4, headY - 4, 12, 8, cMid);
    fillEllipseW(headX - dir * 7, headY - 6, 8, 3, cLight);
    fillEllipseW(headX - dir * 4, headY + 3, 8, 3, cBelly);

    for (int i = 0; i < 24; i++) {
        int sx = headX + dir * (4 + i);
        int sy = headY - 3 + i / 12;
        int sh = 7 - i / 6;
        if (sh < 3) sh = 3;
        strip.drawFastVLine(sx, W2L(sy), sh, cMid);
        drawPixW(sx, sy, cLight);
        drawPixW(sx, sy + sh - 1, cOutline);
    }
    fillEllipseW(headX + dir * 28, headY + 1, 4, 3, cDark);
    drawPixW(headX + dir * 27, headY, 0x0000);

    int jawDrop = mouthOpen + 4;
    if (jawDrop > 10) jawDrop = 10;
    int mouthY = headY + 4;
    fillTriW(headX + dir * 3, mouthY,
             headX + dir * 28, mouthY + 1,
             headX + dir * 9, mouthY + jawDrop, MOUTH);
    fillEllipseW(headX + dir * 12, mouthY + jawDrop - 1, 8, 2, TONGUE);
    fillEllipseW(headX + dir * 12, mouthY + jawDrop - 2, 6, 1, 0xFD5F);
    for (int i = 0; i < 8; i++) {
        int tx = headX + dir * (5 + i * 3);
        fillTriW(tx - 1, mouthY, tx + 1, mouthY, tx, mouthY + 2, TEETH);
    }
    for (int i = 0; i < 19; i++) {
        int sx = headX + dir * (6 + i);
        int sy = mouthY + jawDrop;
        strip.drawFastVLine(sx, W2L(sy), 3, cJaw);
        drawPixW(sx, sy + 2, cOutline);
    }
    if (eatingPose) {
        for (int i = 0; i < 6; i++) {
            int tx = headX + dir * (6 + i * 3);
            int ty = mouthY + jawDrop + 1;
            fillTriW(tx - 1, ty + 1, tx + 1, ty + 1, tx, ty - 1, TEETH);
        }
    }

    // ---- EYE ----
    fillCircW(headX - dir * 4, headY - 7, 4, EYE_WHITE);
    fillCircW(headX - dir * 4, headY - 7, 3, cEyeBlue);
    fillCircW(headX - dir * 3, headY - 7, 1, EYE_PUPIL);
    drawPixW(headX - dir * 5, headY - 8, 0xFFFF);
    drawPixW(headX - dir * 1, headY - 7, cOutline);

    // ---- LEGS with prominent white claws ----
    int leg1Base = bodyCx + dir * (-6 + leg1Off);
    int leg2Base = bodyCx + dir * (8 + leg2Off);
    int legTop = curBodyCy + 10;
    int legMid = curBodyCy + 19;
    int legFoot = curBodyCy + 27;

    // Back leg
    fillEllipseW(leg1Base, legTop + 2, 8, 10, cDark);
    fillEllipseW(leg1Base, legTop + 1, 7, 9, cMid);
    fillRectW(leg1Base - 3, legMid - 2, 7, 12, cDark);
    fillRectW(leg1Base - 2, legMid - 2, 5, 11, cMid);
    fillEllipseW(leg1Base + dir * 3, legFoot, 12, 4, cDark);
    fillEllipseW(leg1Base + dir * 4, legFoot - 1, 10, 3, cMid);
    fillTriW(leg1Base + dir * 8, legFoot + 3, leg1Base + dir * 13, legFoot, leg1Base + dir * 12, legFoot + 4, cClaw);
    fillTriW(leg1Base + dir * 3, legFoot + 4, leg1Base + dir * 8, legFoot, leg1Base + dir * 8, legFoot + 4, cClaw);
    fillTriW(leg1Base - dir * 2, legFoot + 4, leg1Base + dir * 3, legFoot + 1, leg1Base + dir * 3, legFoot + 5, cClaw);

    // Front leg
    fillEllipseW(leg2Base, legTop + 1, 8, 11, cOutline);
    fillEllipseW(leg2Base, legTop, 7, 10, cBody);
    fillEllipseW(leg2Base - dir * 2, legTop - 2, 5, 6, cLight);
    fillRectW(leg2Base - 3, legMid - 1, 7, 13, cMid);
    fillRectW(leg2Base - 2, legMid - 1, 5, 12, cLight);
    fillEllipseW(leg2Base + dir * 4, legFoot + 1, 13, 4, cDark);
    fillEllipseW(leg2Base + dir * 5, legFoot, 11, 3, cMid);
    fillTriW(leg2Base + dir * 9, legFoot + 4, leg2Base + dir * 15, legFoot, leg2Base + dir * 14, legFoot + 5, cClaw);
    fillTriW(leg2Base + dir * 4, legFoot + 5, leg2Base + dir * 10, legFoot, leg2Base + dir * 10, legFoot + 5, cClaw);
    fillTriW(leg2Base - dir * 1, legFoot + 5, leg2Base + dir * 5, legFoot + 1, leg2Base + dir * 5, legFoot + 5, cClaw);

    // ---- SMALL ARMS with subtle green claws ----
    int shoulderX = bodyCx + dir * 15;
    int shoulderY = curBodyCy + 5;
    int elbowX = shoulderX + dir * 6;
    int elbowY = shoulderY + 7;
    int wristX = elbowX + dir * 4;
    int wristY = elbowY + 4;

    drawLineW(shoulderX, shoulderY, elbowX, elbowY, cOutline);
    drawLineW(shoulderX + dir, shoulderY, elbowX + dir, elbowY, cMid);
    drawLineW(shoulderX + dir, shoulderY + 1, elbowX + dir, elbowY + 1, cLight);
    fillEllipseW(elbowX, elbowY, 4, 3, cMid);
    fillEllipseW(elbowX - dir, elbowY - 1, 3, 2, cLight);

    drawLineW(elbowX, elbowY, wristX, wristY, cOutline);
    drawLineW(elbowX + dir, elbowY, wristX + dir, wristY, cMid);
    drawLineW(elbowX + dir, elbowY + 1, wristX + dir, wristY + 1, cLight);
    fillEllipseW(wristX, wristY, 3, 2, cMid);

    drawLineW(wristX + dir * 1, wristY + 1, wristX + dir * 5, wristY + 2, cMid);
    drawLineW(wristX, wristY + 2, wristX + dir * 4, wristY + 4, cMid);
    drawLineW(wristX - dir * 1, wristY + 3, wristX + dir * 3, wristY + 6, cDark);
}

void drawApple(int wx, int wy) {
    if (!inStrip(wy, 32)) return;
    int cx = wx + 14, cy = wy + 18;
    fillEllipseW(cx, cy + 13, 11, 2, 0x2104);
    fillCircW(cx, cy, 12, APPLE_SHADOW);
    fillCircW(cx, cy, 11, APPLE_RED);
    fillCircW(cx - 3, cy - 3, 5, APPLE_HIGH);
    fillCircW(cx - 4, cy - 4, 2, 0xFFFF);
    drawLineW(cx + 1, cy - 11, cx + 2, cy - 14, APPLE_STEM);
    drawLineW(cx + 2, cy - 11, cx + 3, cy - 14, APPLE_STEM);
    fillTriW(cx + 2, cy - 12, cx + 9, cy - 14, cx + 5, cy - 9, APPLE_LEAF);
    drawLineW(cx + 3, cy - 12, cx + 7, cy - 14, 0x2442);
}

void drawFish(int wx, int wy) {
    if (!inStrip(wy, 25)) return;
    int cx = wx + 16, cy = wy + 12;
    fillEllipseW(cx, cy + 10, 16, 2, 0x2945);
    fillTriW(cx - 13, cy, cx - 22, cy - 6, cx - 22, cy + 6, 0x3A5F);
    fillTriW(cx - 13, cy, cx - 19, cy - 3, cx - 19, cy + 3, 0x6DBE);
    fillEllipseW(cx, cy, 14, 7, 0x3A5F);
    fillEllipseW(cx, cy - 1, 13, 6, 0x5CFE);
    fillEllipseW(cx + 1, cy - 2, 10, 4, 0xAF7F);
    fillEllipseW(cx + 2, cy - 3, 7, 2, 0xDFFF);
    fillTriW(cx - 4, cy - 6, cx + 1, cy - 10, cx + 4, cy - 6, 0x3A5F);
    drawLineW(cx - 2, cy - 8, cx + 2, cy - 8, 0x6DBE);
    fillTriW(cx - 1, cy + 3, cx + 3, cy + 6, cx + 5, cy + 3, 0x5CFE);
    drawLineW(cx + 5, cy - 2, cx + 5, cy + 2, 0x3A5F);
    drawLineW(cx + 4, cy - 1, cx + 4, cy + 1, 0x2945);
    fillCircW(cx + 9, cy - 1, 2, 0xFFFF);
    drawPixW(cx + 10, cy - 1, 0x0000);
    drawPixW(cx + 9, cy - 2, 0xFFFF);
    drawPixW(cx - 3, cy, 0xDFFF);
    drawPixW(cx - 1, cy + 1, 0xDFFF);
    drawPixW(cx + 3, cy, 0xDFFF);
}

void drawWater(int wx, int wy, int animFrame) {
    if (!inStrip(wy, 22)) return;
    int cx = wx + 22, cy = wy + 12;
    fillEllipseW(cx, cy + 6, 22, 5, 0x31A6);
    fillEllipseW(cx, cy + 4, 20, 4, 0x6B4D);
    fillEllipseW(cx, cy + 1, 19, 4, WATER_BLUE);
    fillEllipseW(cx, cy, 17, 3, WATER_LIGHT);
    int rip = animFrame % 4;
    if (rip == 0) drawEllipseW(cx, cy, 11, 2, WATER_FOAM);
    else if (rip == 1) { drawEllipseW(cx, cy, 8, 2, WATER_FOAM); drawEllipseW(cx, cy, 15, 3, WATER_LIGHT); }
    else if (rip == 2) { drawEllipseW(cx, cy, 5, 1, WATER_FOAM); drawEllipseW(cx, cy, 12, 2, WATER_LIGHT); }
    else drawEllipseW(cx, cy, 7, 2, WATER_FOAM);
}

void drawObstacle1(int wx, int wy) {
    if (!inStrip(wy - 3, 26)) return;

    uint16_t shadow = 0x18C3;
    uint16_t outline = 0x2945;
    uint16_t dark = 0x4A69;
    uint16_t mid = 0x7BEF;
    uint16_t light = 0xBDF7;
    uint16_t chip = 0x6B4D;

    fillEllipseW(wx + 12, wy + 19, 13, 3, shadow);
    fillEllipseW(wx + 12, wy + 12, 12, 9, outline);
    fillEllipseW(wx + 11, wy + 11, 10, 8, dark);
    fillEllipseW(wx + 9, wy + 8, 8, 5, mid);
    fillEllipseW(wx + 6, wy + 6, 4, 2, light);
    fillEllipseW(wx + 16, wy + 13, 5, 4, chip);
    drawLineW(wx + 3, wy + 13, wx + 8, wy + 17, outline);
    drawLineW(wx + 13, wy + 5, wx + 20, wy + 11, outline);
    drawPixW(wx + 7, wy + 8, 0xFFFF);
    drawPixW(wx + 8, wy + 8, 0xFFFF);
}

void drawObstacle2(int wx, int wy) {
    if (!inStrip(wy - 4, 46)) return;
    int baseY = wy + 38;
    uint16_t cactusDark = 0x14A3;
    uint16_t cactusMid = 0x1E05;
    uint16_t cactusLight = 0x2F49;

    fillEllipseW(wx + 16, baseY + 2, 18, 3, 0x18C3);

    fillRectW(wx + 11, wy + 6, 10, 31, cactusDark);
    fillRectW(wx + 12, wy + 5, 8, 31, cactusMid);
    fillRectW(wx + 14, wy + 7, 2, 26, cactusLight);

    fillRectW(wx + 2, wy + 13, 7, 12, cactusDark);
    fillRectW(wx + 3, wy + 12, 5, 12, cactusMid);
    fillRectW(wx + 20, wy + 16, 7, 11, cactusDark);
    fillRectW(wx + 21, wy + 15, 5, 11, cactusMid);

    fillRectW(wx + 7, wy + 10, 4, 5, cactusMid);
    fillRectW(wx + 18, wy + 13, 4, 5, cactusMid);

    fillEllipseW(wx + 16, wy + 5, 5, 4, cactusMid);
    fillEllipseW(wx + 6, wy + 12, 4, 3, cactusMid);
    fillEllipseW(wx + 24, wy + 15, 4, 3, cactusMid);

    drawLineW(wx + 13, wy + 9, wx + 13, wy + 34, cactusDark);
    drawLineW(wx + 17, wy + 8, wx + 17, wy + 34, cactusDark);
    drawLineW(wx + 6, wy + 14, wx + 6, wy + 24, cactusDark);
    drawLineW(wx + 23, wy + 17, wx + 23, wy + 26, cactusDark);

    for (int i = 0; i < 6; i++) {
        int sx = wx + 9 + i * 3;
        drawPixW(sx, wy + 11 + (i & 1), 0xFFFF);
        drawPixW(sx - 4, wy + 19 + (i % 3), 0xFFFF);
        drawPixW(sx + 5, wy + 21 + ((i + 1) % 3), 0xFFFF);
    }
}

void drawPteranodon(int wx, int wy, bool wingsUp) {
    if (!inStrip(wy - 6, 34)) return;

    uint16_t wingDark = 0x1082;
    uint16_t wingMid = 0x2965;
    uint16_t wingLight = 0x6B4D;
    int bodyX = wx + 18;
    int bodyY = wy + 10;
    int wingTipY = wingsUp ? bodyY - 10 : bodyY - 3;
    int wingBackY = wingsUp ? bodyY + 1 : bodyY + 7;

    fillTriW(bodyX - 1, bodyY, bodyX - 24, wingTipY, bodyX - 10, wingBackY, wingDark);
    fillTriW(bodyX + 1, bodyY, bodyX + 22, wingTipY + 2, bodyX + 9, wingBackY + 2, wingDark);
    fillTriW(bodyX, bodyY - 1, bodyX - 18, wingTipY + 2, bodyX - 9, bodyY + 2, wingMid);
    fillTriW(bodyX, bodyY - 1, bodyX + 17, wingTipY + 4, bodyX + 7, bodyY + 3, wingMid);

    fillEllipseW(bodyX, bodyY + 1, 7, 3, wingMid);
    fillEllipseW(bodyX + 1, bodyY, 5, 2, wingLight);
    fillTriW(bodyX + 4, bodyY, bodyX + 14, bodyY - 2, bodyX + 11, bodyY + 4, wingMid);
    drawLineW(bodyX + 10, bodyY, bodyX + 18, bodyY - 4, wingDark);
    drawLineW(bodyX - 2, bodyY + 2, bodyX - 6, bodyY + 7, wingDark);
    drawLineW(bodyX + 2, bodyY + 2, bodyX - 1, bodyY + 8, wingDark);
    drawPixW(bodyX + 5, bodyY - 1, 0xFFFF);
    drawPixW(bodyX + 6, bodyY - 1, 0x0000);
}

void drawRunnerPond(int wx, int width, int depth) {
    if (!inStrip(GROUND_Y - 2, 28)) return;

    int cx = wx + width / 2;
    int waterTop = GROUND_Y - 1;
    fillEllipseW(cx, waterTop + 8, width / 2 + 4, depth + 5, 0x18C3);
    fillEllipseW(cx, waterTop + 6, width / 2 + 2, depth + 3, 0x2965);
    fillRectW(wx + 4, waterTop + 1, width - 8, depth + 6, WATER_BLUE);
    fillEllipseW(cx, waterTop + 3, width / 2 - 2, depth + 1, WATER_LIGHT);
    drawLineW(wx + 10, waterTop + 2, wx + width - 10, waterTop + 2, WATER_FOAM);

    int rippleShift = (millis() / 120) % 12;
    for (int i = 0; i < 3; i++) {
        int rippleX = wx + 16 + i * (width / 3);
        drawEllipseW(rippleX + (rippleShift % 5), waterTop + 5 + (i & 1), 8, 2, WATER_FOAM);
    }
}

void drawStatBar(int x, int wy, float val, uint16_t color) {
    int y = W2L(wy);
    int w = 130, h = 16;
    int fillW = (int)(val * (w - 4) / 100.0f);
    strip.fillRect(x + 1, y + 1, w - 2, h - 2, STAT_BG);
    strip.drawRect(x, y, w, h, UI_BORDER);
    for (int px = 0; px < fillW; px++) {
        float t = (float)px / (w - 4);
        uint16_t barColor = lerp565(lerp565(color, 0, 0.5f), color, t);
        strip.drawFastVLine(x + 2 + px, y + 2, h - 4, barColor);
    }
    strip.drawFastHLine(x + 2, y + 2, fillW, lerp565(color, 0xFFFF, 0.3f));
}

void renderUI() {
    if (ui.menuOpened && !game.active) {
        int menuY = 8, menuH = 56;
        if (inStrip(menuY, menuH)) {
            for (int y = menuY; y < menuY + menuH; y++) {
                int ly = W2L(y);
                if (ly < 0 || ly >= STRIP_H) continue;
                uint16_t* buf = strip.getBuffer();
                for (int x = 10; x < SCREEN_W - 10; x++) {
                    buf[ly * SCREEN_W + x] = lerp565(buf[ly * SCREEN_W + x], UI_BG, 0.75f);
                }
            }
            drawRectW(10, menuY, SCREEN_W - 20, menuH, UI_BORDER);
            drawRectW(11, menuY + 1, SCREEN_W - 22, menuH - 2, UI_HIGHLIGHT);
            char title[STR_SZ];
            memcpy_P(title, &menuItems[ui.menu][0], STR_SZ);
            strip.setTextSize(2);
            strip.setTextColor(UI_BORDER);
            setCursorW(30, menuY + 6);
            strip.print(title);
            if (ui.menuDepth) {
                char sub[STR_SZ];
                memcpy_P(sub, &menuItems[ui.menu][ui.subMenu], STR_SZ);
                strip.setTextSize(1);
                strip.setTextColor(UI_HIGHLIGHT);
                setCursorW(30, menuY + 32);
                strip.print("> ");
                strip.print(sub);
            }
            int arrowY = menuY + (ui.menuDepth ? 32 : 10);
            fillTriW(18, arrowY, 18, arrowY + 10, 24, arrowY + 5, UI_HIGHLIGHT);
        }
    }

    if (ui.setting > 0 && !game.active && ui.menuOpened && ui.menuDepth) {
        int statsY = 75;
        if (inStrip(statsY, 24)) {
            switch (ui.setting) {
                case 701: drawStatBar(150, statsY, pet.hunger,     STAT_HUNGER); break;
                case 702: drawStatBar(150, statsY, pet.happiness,  STAT_JOY); break;
                case 703: drawStatBar(150, statsY, pet.health,     STAT_HEALTH); break;
                case 704: drawStatBar(150, statsY, pet.discipline, STAT_DISC); break;
                case 705:
                    strip.setTextSize(2);
                    strip.setTextColor((pet.weight >= WEIGHT_SWEET_MIN && pet.weight <= WEIGHT_SWEET_MAX) ? UI_SUCCESS : UI_TEXT);
                    setCursorW(150, statsY + 2); strip.print(pet.weight, 1); strip.print(" t");
                    break;
                case 706:
                    strip.setTextSize(2); strip.setTextColor(UI_TEXT);
                    setCursorW(150, statsY + 2); strip.print((int)pet.age); strip.print(" y");
                    break;
                case 801:
                    strip.setTextSize(2);
                    strip.setTextColor(pet.soundEnabled ? UI_SUCCESS : UI_DANGER);
                    setCursorW(150, statsY + 2);
                    strip.print(pet.soundEnabled ? "ON " : "OFF");
                    break;
            }
        }
    }

    if (ui.notification) {
        int nX = 290, nY = 10;
        if (inStrip(nY, 22)) {
            float pulse = 0.5f + 0.5f * sinf(millis() * 0.006f);
            uint16_t col = lerp565(0x7800, 0xF800, pulse);
            fillCircW(nX + 10, nY + 10, 11, col);
            drawCircW(nX + 10, nY + 10, 11, UI_BORDER);
            strip.setTextSize(2);
            strip.setTextColor(UI_BORDER);
            setCursorW(nX + 7, nY + 4);
            strip.print("!");
        }
    }

    if (game.active && !game.gameOver) {
        int sY = 220;
        if (inStrip(sY, 20)) {
            fillRectW(0, sY, SCREEN_W, 20, 0x0000);
            strip.setTextSize(1);
            strip.setTextColor(UI_HIGHLIGHT, 0x0000);
            setCursorW(8, sY + 6);
            strip.print("HI:");
            strip.print(game.hiScore);

            strip.setTextColor(UI_TEXT, 0x0000);
            setCursorW(130, sY + 6);
            strip.print("PTS:");
            strip.print(game.score);
        }
    }

    if (game.gameOver) {
        int goY = 62, goH = 112;
        if (inStrip(goY, goH)) {
            for (int y = goY; y < goY + goH; y++) {
                int ly = W2L(y);
                if (ly < 0 || ly >= STRIP_H) continue;
                uint16_t* buf = strip.getBuffer();
                for (int x = 30; x < SCREEN_W - 30; x++) {
                    buf[ly * SCREEN_W + x] = lerp565(buf[ly * SCREEN_W + x], UI_BG, 0.88f);
                }
            }
            drawRectW(30, goY, SCREEN_W - 60, goH, UI_DANGER);
            drawRectW(31, goY + 1, SCREEN_W - 62, goH - 2, UI_HIGHLIGHT);
            strip.setTextSize(4);
            strip.setTextColor(UI_DANGER);
            setCursorW(70, goY + 10);
            strip.print("GAME");
            setCursorW(180, goY + 10);
            strip.print("OVER");
            if (game.newHiScore) {
                strip.setTextSize(2);
                strip.setTextColor(UI_HIGHLIGHT);
                setCursorW(70, goY + 58);
                strip.print("NEW HI-SCORE!");
                strip.setTextSize(3);
                setCursorW(120, goY + 74);
                strip.print(game.score);
            } else {
                strip.setTextSize(2);
                strip.setTextColor(UI_TEXT);
                setCursorW(90, goY + 58);
                strip.print("SCORE: ");
                strip.print(game.score);
                strip.setTextSize(1);
                setCursorW(90, goY + 78);
                strip.print("HI: ");
                strip.print(game.hiScore);
            }
            strip.setTextSize(1);
            strip.setTextColor(UI_HIGHLIGHT);
            setCursorW(74, goY + 96);
            strip.print("L/M retry  R exit");
        }
    }

    if (ui.animType == 3) {
        int markIdx = ((millis() - ui.animStart) / 150) % 5;
        for (int i = 0; i <= markIdx && i < 5; i++) {
            int mx = 190 + i * 22;
            int my = 100;
            if (inStrip(my, 26)) {
                strip.setTextSize(3);
                strip.setTextColor(UI_HIGHLIGHT);
                setCursorW(mx, my);
                strip.print("!");
            }
        }
    }

    if (ui.animType == 4) {
        int phase = ((millis() - ui.animStart) / 200) % 6;
        if (phase % 2 == 1) {
            int cx = 160, cy = 120;
            if (inStrip(cy - 22, 44)) {
                fillRectW(cx - 22, cy - 6, 44, 12, UI_BORDER);
                fillRectW(cx - 6, cy - 22, 12, 44, UI_BORDER);
                fillRectW(cx - 20, cy - 4, 40, 8, UI_DANGER);
                fillRectW(cx - 4, cy - 20, 8, 40, UI_DANGER);
            }
        }
    }
}

void renderRunnerHudDirect() {
    const int hudY = 220;
    const uint16_t hudBg = 0x0000;
    tft.setTextSize(1);
    tft.setTextColor(UI_HIGHLIGHT, hudBg);
    tft.setCursor(8, hudY + 6);
    tft.print("HI:");
    tft.print(game.hiScore);
    tft.print("     ");
    tft.setTextColor(UI_TEXT, hudBg);
    tft.setCursor(130, hudY + 6);
    tft.print("PTS:");
    tft.print(game.score);
    tft.print("      ");
}

void renderFrame(float t) {
    float walkX   = anim.prevWalkX   + (anim.walkX - anim.prevWalkX) * t;
    float grassX  = scene.prevGrassX + (scene.grassX - scene.prevGrassX) * t;
    float treesX  = scene.prevTreesX + (scene.treesX - scene.prevTreesX) * t;
    float cloud1X = scene.prevCloud1X + (scene.cloud1X - scene.prevCloud1X) * t;
    float cloud2X = scene.prevCloud2X + (scene.cloud2X - scene.prevCloud2X) * t;
    float sunX    = scene.prevSunX + (scene.sunX - scene.prevSunX) * t;
    float pondX   = game.pond.prevX + (game.pond.x - game.pond.prevX) * t;
    float obsX    = game.obstacle.prevX + (game.obstacle.x - game.obstacle.prevX) * t;
    float jumpPos = game.prevJumpPos + (game.jumpPos - game.prevJumpPos) * t;
    bool visualJumping = game.active && (game.jumping || jumpPos > 0.5f);

    unsigned long animElapsed = millis() - ui.animStart;
    int pose;
    if (pet.dead) pose = POSE_DEAD;
    else if (ui.animType == 1) {
        int chewStep = (animElapsed / 200) % 3; // 3 jaw cycles over 1800ms
        pose = (chewStep == 1) ? POSE_EAT3 : POSE_EAT4;
    }
    else if (ui.animType == 2) {
        int chewStep = (animElapsed / 200) % 3;
        pose = (chewStep == 1) ? POSE_DRINK3 : POSE_DRINK2;
    }
    else if (pet.sleeping) pose = ((millis() / 700) % 2 == 0) ? POSE_SLEEP1 : POSE_SLEEP2;
    else if (visualJumping) pose = POSE_JUMP;
    else if (game.active && game.ducking) pose = ((millis() / 120) % 2 == 0) ? POSE_WALK1 : POSE_WALK2;
    else pose = POSE_WALK0 + (anim.walkPos % 5);

    int dinoX = game.active ? RUNNER_PLAYER_X : (int)walkX;
    int runnerBaseY = (game.ducking && !visualJumping) ? RUNNER_PLAYER_DUCK_Y : RUNNER_PLAYER_STAND_Y;
    if (game.inPond && !visualJumping) runnerBaseY += 3;
    int dinoY = game.active ? (runnerBaseY - (int)(jumpPos + 0.5f)) : (GROUND_Y - 55);
    bool dinoRight = game.active ? true : anim.walkRight;
    int dir = dinoRight ? 1 : -1;

    int headOffYRef = 0;
    int jumpLiftRef = 0;
    switch (pose) {
        case POSE_EAT1: headOffYRef = 8; break;
        case POSE_EAT2: headOffYRef = 18; break;
        case POSE_EAT3: headOffYRef = 24; break;
        case POSE_EAT4: headOffYRef = 12; break;
        case POSE_DRINK1: case POSE_DRINK2:
        case POSE_DRINK3: case POSE_DRINK4:
            headOffYRef = 22; break;
        case POSE_JUMP:
            jumpLiftRef = 6; break;
        default: break;
    }
    int bodyCxRef = dinoX + 40;
    int curBodyCyRef = (dinoY + 28) - jumpLiftRef;
    int headXRef = bodyCxRef + dir * 41;
    int headYRef = curBodyCyRef - 9 + headOffYRef;
    int mouthXRef = headXRef + dir * 24;
    int mouthYRef = headYRef + 6;

    bool runnerFastFrame = game.active && !game.gameOver && !runnerNeedsFullRedraw && ui.animType == 0;
    int firstStripY = runnerFastFrame ? RUNNER_FAST_TOP : 0;
    int lastStripY = runnerFastFrame ? RUNNER_FAST_BOTTOM : SCREEN_H;

    tft.startWrite();
    for (stripOffsetY = firstStripY; stripOffsetY < lastStripY; stripOffsetY += STRIP_H) {
        int writeH = lastStripY - stripOffsetY;
        if (writeH > STRIP_H) writeH = STRIP_H;

        renderSky();
        renderSun(sunX);
        renderStars();
        renderClouds(cloud1X, cloud2X);
        renderMountains();
        renderTrees(game.active ? 0 : treesX);

        if (ui.animType == 1 || ui.animType == 2) {
            int item = ui.animItem;
            int biteStage = (int)(animElapsed / 600);
            if (biteStage > 3) biteStage = 3;
            int foodCenterX = mouthXRef + dir * 16;
            int foodCenterY = mouthYRef + 8;
            if (foodCenterY > GROUND_Y - 5) foodCenterY = GROUND_Y - 5;

            int foodX = (item == 101) ? (foodCenterX - 14) : ((item == 102) ? (foodCenterX - 16) : (foodCenterX - 22));
            int foodY = (item == 101) ? (foodCenterY - 18) : (foodCenterY - 12);
            uint16_t biteBg = (foodCenterY > GROUND_Y - 16) ? GRASS_HIGH : computeSkyColor(foodY + 8);
            int biteSign = dinoRight ? -1 : 1;

            if (item == 101) {
                drawApple(foodX, foodY);
                if (biteStage >= 1) fillCircW(foodX + 14 + biteSign * 6, foodY + 10, 5, biteBg);
                if (biteStage >= 2) fillCircW(foodX + 14 + biteSign * 10, foodY + 4, 6, biteBg);
                if (biteStage >= 3) fillCircW(foodX + 14 + biteSign * 14, foodY + 12, 6, biteBg);
            } else if (item == 102) {
                drawFish(foodX, foodY);
                if (biteStage >= 1) fillCircW(foodX + 16 + biteSign * 8, foodY + 8, 6, biteBg);
                if (biteStage >= 2) fillCircW(foodX + 16 + biteSign * 12, foodY + 3, 6, biteBg);
                if (biteStage >= 3) fillCircW(foodX + 16 + biteSign * 16, foodY + 10, 7, biteBg);
            } else if (item == 103) {
                int f = (animElapsed / 180) % 4;
                drawWater(foodX, foodY, f);
                if (biteStage >= 1) fillEllipseW(foodX + 22, foodY + 10, 12, 3, biteBg);
                if (biteStage >= 2) fillEllipseW(foodX + 22, foodY + 9, 15, 4, biteBg);
                if (biteStage >= 3) fillEllipseW(foodX + 22, foodY + 8, 18, 5, biteBg);
            }
        }

        if (game.active && !game.gameOver) renderRunnerGround(grassX);
        else renderGrass(grassX);
        renderPoops();

        if (game.active && game.pond.active) {
            drawRunnerPond((int)pondX, game.pond.width, game.pond.depth);
        }

        if (game.active && game.obstacle.active) {
            if (game.obstacle.type == RUNNER_OBS_SMALL_CACTUS) drawObstacle1((int)obsX, (int)game.obstacle.y);
            else if (game.obstacle.type == RUNNER_OBS_LARGE_CACTUS) drawObstacle2((int)obsX, (int)game.obstacle.y);
            else drawPteranodon((int)obsX, (int)game.obstacle.y, ((millis() / 100) & 1) == 0);
        }

        drawSpikey(dinoX, dinoY, pose, dinoRight);
        if (game.active && game.pond.active && game.inPond && !visualJumping) {
            drawRunnerPond((int)pondX, game.pond.width, game.pond.depth);
        }

        if (!runnerFastFrame) renderUI();

        tft.setAddrWindow(0, stripOffsetY, SCREEN_W, writeH);
        tft.writePixels(strip.getBuffer(), SCREEN_W * writeH);
    }
    tft.endWrite();

    if (runnerFastFrame) {
        renderRunnerHudDirect();
    } else if (game.active && !game.gameOver) {
        runnerNeedsFullRedraw = false;
    }
}

int countPoops() {
    int count = 0;
    for (int i = 0; i < MAX_POOPS; i++) if (pet.poops[i] > 0) count++;
    return count;
}

void clearPoops() {
    for (int i = 0; i < MAX_POOPS; i++) pet.poops[i] = 0;
}

void resetMiniGame() {
    runnerNeedsFullRedraw = true;
    game.active = false; game.paused = false; game.gameOver = false;
    game.newHiScore = false; game.jumping = false; game.ducking = false; game.inPond = false;
    game.score = 0; game.level = 0; game.jumpPos = 0; game.prevJumpPos = 0;
    game.jumpVel = RUNNER_JUMP_VEL;
    game.runStartHiScore = game.hiScore;
    game.nextPondScore = random(RUNNER_POND_MIN_SCORE_GAP, RUNNER_POND_MAX_SCORE_GAP + 1);
    game.runnerAccumMs = 0;
    resetRunnerObstacle(game.obstacle);
    resetRunnerPond(game.pond);
    anim.walkX = 120; anim.prevWalkX = 120;
    anim.walkPos = 0; anim.walkReverse = false; anim.walkRight = true;
}

void syncSceneClock() {
    bool wasNight = scene.night;
    if (pet.sleeping) {
        if (!wasNight) {
            for (int i = 0; i < 14; i++) {
                scene.stars[i][0] = random(10, SCREEN_W - 10);
                scene.stars[i][1] = random(8, 80);
            }
        }
        scene.night = true;
        scene.sunX = SCREEN_W * 0.68f;
        scene.sunsetBlend = 0;
        return;
    }

    float cycleT = (float)pet.cycleProgressMs / (float)LIFE_CYCLE_MS;
    scene.sunX = -20.0f + cycleT * (SCREEN_W + 40.0f);
    scene.night = pet.cycleProgressMs >= DAY_PHASE_MS;

    if (scene.night && !wasNight) {
        for (int i = 0; i < 14; i++) {
            scene.stars[i][0] = random(10, SCREEN_W - 10);
            scene.stars[i][1] = random(8, 80);
        }
    }

    if (!scene.night) {
        float sn = (scene.sunX + 20.0f) / (SCREEN_W + 40.0f);
        if (sn < 0.18f) scene.sunsetBlend = 1.0f - sn / 0.18f;
        else if (sn > 0.82f) scene.sunsetBlend = (sn - 0.82f) / 0.18f;
        else scene.sunsetBlend = 0;
    } else {
        scene.sunsetBlend = 0;
    }
}

void normalizeLoadedState() {
    pet.hunger = isfinite(pet.hunger) ? clampf(pet.hunger, 0, 100) : 100;
    pet.happiness = isfinite(pet.happiness) ? clampf(pet.happiness, 0, 100) : 100;
    pet.health = isfinite(pet.health) ? clampf(pet.health, 0, 100) : 100;
    pet.discipline = isfinite(pet.discipline) ? clampf(pet.discipline, 0, 100) : 100;
    pet.weight = isfinite(pet.weight) ? pet.weight : 1;
    if (pet.weight < 0) pet.weight = 0;
    pet.age = isfinite(pet.age) ? pet.age : 0;
    if (pet.age < 0) pet.age = 0;
    if (pet.age > 100) pet.age = 100;
    pet.poopometer = isfinite(pet.poopometer) ? pet.poopometer : 0;
    if (pet.poopometer < 0) pet.poopometer = 0;
    if (pet.poopometer >= FOOD_PER_POOP) pet.poopometer = fmodf(pet.poopometer, FOOD_PER_POOP);
    pet.cycleProgressMs %= LIFE_CYCLE_MS;
    if (pet.completedCycles == 0 && pet.age > 0) {
        pet.completedCycles = (uint16_t)(pet.age * 5.0f);
    }
    for (int i = 0; i < MAX_POOPS; i++) {
        if (pet.poops[i] <= 0) pet.poops[i] = 0;
        else if (pet.poops[i] < 20) pet.poops[i] = 20;
        else if (pet.poops[i] > 140) pet.poops[i] = 140;
    }
    if (!pet.dead) {
        pet.victory = false;
        if (pet.deathReason == END_REASON_OLD_AGE && pet.age < 100) pet.deathReason = END_REASON_NONE;
    } else if (pet.deathReason == END_REASON_NONE) {
        pet.deathReason = pet.victory ? END_REASON_OLD_AGE : END_REASON_INJURY;
    }
    pet.warnedHunger = pet.warnedJoy = pet.warnedHealth = false;
}

void resetPetProgress() {
    bool keepSound = pet.soundEnabled;
    pet = PetState();
    pet.soundEnabled = keepSound;

    ui.menuOpened = false; ui.menuDepth = false;
    ui.menu = 0; ui.subMenu = 1;
    ui.action = 0; ui.setting = 0; ui.notification = false;
    ui.animType = 0; ui.animItem = 0; ui.animStart = 0;

    foodSplashBaseDrawn = false;
    foodSplashBaseItem = 0;
    foodSplashOverlayStep = -1;
    foodSplashPrevRectValid = false;
    deathScreenDrawn = false;
    lastPoopMs = 0;

    resetMiniGame();
    syncSceneClock();
    scene.prevGrassX = scene.grassX;
    scene.prevTreesX = scene.treesX;
    scene.prevCloud1X = scene.cloud1X;
    scene.prevCloud2X = scene.cloud2X;
    scene.prevSunX = scene.sunX;
}

void killPet(uint8_t reason, bool victoryScreen) {
    if (pet.dead) return;

    pet.hunger = clampf(pet.hunger, 0, 100);
    pet.happiness = clampf(pet.happiness, 0, 100);
    pet.health = clampf(pet.health, 0, 100);
    pet.discipline = clampf(pet.discipline, 0, 100);
    if (pet.weight < 0) pet.weight = 0;

    pet.dead = true;
    pet.victory = victoryScreen;
    pet.deathReason = reason;
    pet.sleeping = false;

    ui.menuOpened = false;
    ui.menuDepth = false;
    ui.action = 0;
    ui.setting = 0;
    ui.subMenu = 1;
    ui.animType = 0;
    ui.animItem = 0;

    foodSplashBaseDrawn = false;
    foodSplashBaseItem = 0;
    foodSplashOverlayStep = -1;
    foodSplashPrevRectValid = false;
    deathScreenDrawn = false;

    resetMiniGame();
    syncSceneClock();
    saveState();

    if (!pet.soundEnabled) return;
    if (victoryScreen) {
        tone(BUZZER, 523, 120); delay(130);
        tone(BUZZER, 659, 120); delay(130);
        tone(BUZZER, 784, 120); delay(130);
        tone(BUZZER, 1047, 250);
    } else {
        tone(BUZZER, 500, 400); delay(450);
        tone(BUZZER, 400, 400); delay(450);
        tone(BUZZER, 300, 500);
    }
}

void enforceEndConditions(uint8_t healthReason) {
    if (pet.dead) return;

    pet.hunger = clampf(pet.hunger, 0, 100);
    pet.happiness = clampf(pet.happiness, 0, 100);
    pet.health = clampf(pet.health, 0, 100);
    pet.discipline = clampf(pet.discipline, 0, 100);
    if (pet.weight < 0) pet.weight = 0;

    if (pet.age >= 100.0f) {
        pet.age = 100.0f;
        killPet(END_REASON_OLD_AGE, true);
        return;
    }
    if (pet.weight > WEIGHT_FATAL_MAX) {
        killPet(END_REASON_WEIGHT, false);
        return;
    }
    if (pet.hunger <= 0.0f) {
        pet.hunger = 0;
        killPet(END_REASON_STARVATION, false);
        return;
    }
    if (pet.health <= 0.0f) {
        pet.health = 0;
        killPet(healthReason != END_REASON_NONE ? healthReason : END_REASON_INJURY, false);
        return;
    }
    if (pet.happiness <= 0.0f) {
        pet.happiness = 0;
        killPet(END_REASON_LONELINESS, false);
    }
}

void refreshWarningsAndNotification() {
    if (pet.dead) {
        ui.notification = false;
        return;
    }

    if (pet.hunger <= 20 && !pet.warnedHunger) { pet.warnedHunger = true; if (pet.soundEnabled) tone(BUZZER, 200, 80); }
    if (pet.hunger > 20) pet.warnedHunger = false;

    if (pet.happiness <= 20 && !pet.warnedJoy) { pet.warnedJoy = true; if (pet.soundEnabled) tone(BUZZER, 220, 80); }
    if (pet.happiness > 20) pet.warnedJoy = false;

    if (pet.health <= 20 && !pet.warnedHealth) { pet.warnedHealth = true; if (pet.soundEnabled) tone(BUZZER, 180, 80); }
    if (pet.health > 20) pet.warnedHealth = false;

    ui.notification = (pet.hunger <= 20 || countPoops() > 0 || pet.happiness <= 20 || pet.health <= 20);
}

bool spawnPoop() {
    unsigned long now = millis();
    if (lastPoopMs != 0 && now - lastPoopMs < POOP_MIN_INTERVAL_MS) {
        return false;
    }

    int dirtyCount = countPoops();
    if (dirtyCount >= MAX_POOPS) {
        pet.health = 0;
        killPet(END_REASON_FILTH, false);
        return true;
    }

    for (int i = 0; i < MAX_POOPS; i++) {
        if (pet.poops[i] == 0) {
            pet.poops[i] = random(20, 140);
            break;
        }
    }

    lastPoopMs = now;
    pet.weight -= WEIGHT_LOSS_PER_POOP;
    if (pet.weight < 0) pet.weight = 0;
    dirtyCount++;

    if (pet.soundEnabled) tone(BUZZER, 200, 50);
    if ((dirtyCount % 2) == 0) {
        pet.health -= 20.0f;
    }

    enforceEndConditions(END_REASON_FILTH);
    return true;
}

void applyFoodGain(float hungerGain, bool playSound) {
    if (pet.dead) return;

    pet.hunger = clampf(pet.hunger + hungerGain, 0, 100);
    pet.weight += hungerGain * WEIGHT_PER_FOOD_PERCENT;
    pet.poopometer += hungerGain;

    if (pet.poopometer >= FOOD_PER_POOP && !pet.dead) {
        pet.poopometer -= FOOD_PER_POOP;
        if (!spawnPoop()) {
            pet.poopometer = FOOD_PER_POOP;
        }
    }

    if (playSound && pet.soundEnabled && !pet.dead) tone(BUZZER, 500, 200);
    enforceEndConditions(END_REASON_NONE);
}

void giveWater(bool playSound) {
    if (pet.dead) return;
    pet.health = clampf(pet.health + WATER_HEALTH_GAIN, 0, 100);
    pet.wateredThisCycle = true;
    if (playSound && pet.soundEnabled) tone(BUZZER, 450, 160);
}

void autoFeedSelf() {
    if (pet.hunger <= 30.0f) applyFoodGain(FISH_HUNGER_GAIN, false);
    else if (pet.hunger <= 60.0f) applyFoodGain(APPLE_HUNGER_GAIN, false);
}

void applyDisciplineAutocare() {
    if (pet.dead) return;

    if (pet.discipline > 90.0f) {
        if (countPoops() > 0) clearPoops();
        if (pet.hunger <= 60.0f) autoFeedSelf();
        if (!pet.wateredThisCycle || pet.health <= 80.0f) giveWater(false);
        if (countPoops() > 0) clearPoops();
    } else if (pet.discipline > 60.0f) {
        if (pet.hunger <= 60.0f) autoFeedSelf();
        if (!pet.wateredThisCycle || pet.health <= 80.0f) giveWater(false);
    } else if (pet.discipline > 40.0f) {
        if (!pet.wateredThisCycle || pet.health <= 80.0f) giveWater(false);
    }
}

void processCompletedCycle() {
    pet.completedCycles++;
    if ((pet.completedCycles % 5) == 0) {
        pet.age += 1.0f;
        if (pet.age >= 100.0f) {
            pet.age = 100.0f;
            killPet(END_REASON_OLD_AGE, true);
            return;
        }
    }

    applyDisciplineAutocare();
    if (pet.dead) return;

    bool missedWater = !pet.wateredThisCycle;
    bool missedPlay = !pet.playedThisCycle;
    bool missedTrain = !pet.trainedThisCycle;

    if (missedWater) pet.health -= DAILY_WATER_HEALTH_LOSS;
    if (missedPlay) pet.happiness -= DAILY_PLAY_HAPPINESS_LOSS;
    if (missedTrain) pet.discipline -= DAILY_TRAIN_DISCIPLINE_LOSS;

    pet.wateredThisCycle = false;
    pet.playedThisCycle = false;
    pet.trainedThisCycle = false;

    enforceEndConditions(missedWater ? END_REASON_DEHYDRATION : END_REASON_INJURY);
}

void updatePetStats(unsigned long elapsedMs) {
    if (pet.dead) {
        refreshWarningsAndNotification();
        return;
    }

    if (pet.sleeping) {
        refreshWarningsAndNotification();
        return;
    }

    unsigned long remaining = elapsedMs;
    while (remaining > 0 && !pet.dead) {
        unsigned long cycleRemaining = LIFE_CYCLE_MS - pet.cycleProgressMs;
        unsigned long chunk = (remaining < cycleRemaining) ? remaining : cycleRemaining;

        if (pet.cycleProgressMs < DAY_PHASE_MS) {
            unsigned long dayRemaining = DAY_PHASE_MS - pet.cycleProgressMs;
            unsigned long dayChunk = (chunk < dayRemaining) ? chunk : dayRemaining;
            pet.hunger -= dayChunk * HUNGER_DECAY_PER_DAY_MS;
        }

        pet.cycleProgressMs += chunk;
        remaining -= chunk;

        if (pet.cycleProgressMs >= LIFE_CYCLE_MS) {
            pet.cycleProgressMs = 0;
            processCompletedCycle();
        }
    }

    enforceEndConditions(END_REASON_NONE);
    refreshWarningsAndNotification();
}

void updateWalkAnimation() {
    anim.prevWalkX = anim.walkX;
    if (anim.walkReverse) {
        anim.walkPos--;
        if (anim.walkPos <= 0) { anim.walkPos = 0; anim.walkReverse = false; }
    } else {
        anim.walkPos++;
        if (anim.walkPos >= 4) { anim.walkPos = 4; anim.walkReverse = true; }
    }
    if (!pet.sleeping && ui.animType == 0 && !ui.menuOpened) {
        if (anim.walkRight) {
            anim.walkX += 3.0f;
            if (anim.walkX > 220) anim.walkRight = false;
        } else {
            anim.walkX -= 3.0f;
            if (anim.walkX < 30) anim.walkRight = true;
        }
    }
}

void updateScenery(unsigned long elapsedMs) {
    scene.prevGrassX  = scene.grassX;
    scene.prevTreesX  = scene.treesX;
    scene.prevCloud1X = scene.cloud1X;
    scene.prevCloud2X = scene.cloud2X;
    scene.prevSunX    = scene.sunX;

    float moveScale = elapsedMs / 100.0f;
    syncSceneClock();

    float runnerScroll = getRunnerScrollSpeedPxPerMs() * elapsedMs;

    scene.cloud1X -= 0.8f * moveScale;
    if (scene.cloud1X < -50) {
        scene.cloud1X = SCREEN_W + 50;
        scene.prevCloud1X = scene.cloud1X;
    }
    scene.cloud2X -= 0.5f * moveScale;
    if (scene.cloud2X < -40) {
        scene.cloud2X = SCREEN_W + 40;
        scene.prevCloud2X = scene.cloud2X;
    }

    if (game.active && !game.gameOver) {
        scene.grassX = 0;
        scene.prevGrassX = 0;
    } else if (!pet.sleeping && ui.animType == 0) {
        if (anim.walkRight) { scene.grassX -= 2.5f * moveScale; scene.treesX -= 0.6f * moveScale; }
        else { scene.grassX += 2.5f * moveScale; scene.treesX += 0.6f * moveScale; }
        if (scene.grassX > SCREEN_W) scene.grassX = 0;
        if (scene.grassX < -SCREEN_W) scene.grassX = 0;
        if (scene.treesX > SCREEN_W) scene.treesX = 0;
        if (scene.treesX < -SCREEN_W) scene.treesX = 0;
    }
}

void crashRunnerGame() {
    game.gameOver = true;
    game.paused = false;
    game.jumping = false;
    game.ducking = false;
    game.inPond = false;
    game.jumpPos = 0;
    game.prevJumpPos = 0;
    game.jumpVel = RUNNER_JUMP_VEL;
    game.runnerAccumMs = 0;
    game.newHiScore = game.score > game.runStartHiScore;
    if (game.score > game.hiScore) game.hiScore = game.score;

    if (pet.soundEnabled) {
        tone(BUZZER, 500, 40); delay(50);
        tone(BUZZER, 350, 40); delay(50);
        tone(BUZZER, 200, 80);
    }

    pet.health -= 1;
    enforceEndConditions(END_REASON_INJURY);
    if (!pet.dead) saveState();
}

void processRunnerTick() {
    advanceRunnerWalkPose();

    if (game.jumping) {
        game.jumpPos += game.jumpVel * RUNNER_JUMP_PIXELS_PER_VEL;
        game.jumpVel -= RUNNER_JUMP_GRAVITY;
        if (game.jumpPos < 0) game.jumpPos = 0;
        if (game.jumpVel < -RUNNER_JUMP_VEL) {
            game.jumping = false;
            game.jumpPos = 0;
            game.jumpVel = RUNNER_JUMP_VEL;
        }
    }

    float tickPixels = getRunnerPixelsPerTick();

    if (game.pond.active) {
        game.pond.x -= tickPixels;
        if (getRunnerPondRight(game.pond) < -12) {
            resetRunnerPond(game.pond);
        }
    }

    if (game.obstacle.active) {
        game.obstacle.x -= tickPixels;
        if (getRunnerObstacleRight(game.obstacle) < -12) {
            resetRunnerObstacle(game.obstacle);
        }
    }

    updateRunnerPondContact();

    if (runnerObstacleHitsPlayer()) {
        crashRunnerGame();
        return;
    }

    game.score++;
    if (game.score > game.hiScore) game.hiScore = game.score;
    game.level = game.score / RUNNER_REF_SCORE_STEP;

    if (!game.pond.active && !game.obstacle.active && game.score >= game.nextPondScore) {
        spawnRunnerPond();
    } else if (!game.obstacle.active) {
        spawnRunnerObstacle();
    }
}

void updateGameLogic(unsigned long elapsedMs) {
    if (game.gameOver) {
        game.runnerAccumMs = 0;
        return;
    }

    game.prevJumpPos = game.jumpPos;
    if (game.pond.active) game.pond.prevX = game.pond.x;
    if (game.obstacle.active) game.obstacle.prevX = game.obstacle.x;
    game.runnerAccumMs += elapsedMs;
    int safety = 0;
    while (game.runnerAccumMs >= RUNNER_TICK_MS && safety < 6 && !game.gameOver) {
        processRunnerTick();
        game.runnerAccumMs -= RUNNER_TICK_MS;
        safety++;
    }
}

void handleButtons() {
    if (pet.dead) {
        if (btn1.pressed || btn2.pressed || btn3.pressed) {
            if (pet.soundEnabled) tone(BUZZER, 300, 80);
            resetPetProgress();
            saveState();
        }
        return;
    }

    if (btn1.pressed) {
        if (game.active) {
            if (game.gameOver) {
                resetMiniGame();
                game.active = true;
                spawnRunnerObstacle();
            } else if (!game.jumping) {
                if (pet.soundEnabled) tone(BUZZER, 200, 50);
                game.ducking = false;
                game.jumping = true;
                game.jumpPos = 0;
                game.prevJumpPos = 0;
                game.jumpVel = RUNNER_JUMP_VEL;
            }
        } else {
            if (pet.soundEnabled) tone(BUZZER, 300, 80);
            if (!ui.menuOpened) ui.menuOpened = true;
            else {
                if (ui.menuDepth) {
                    char nxt[STR_SZ];
                    memcpy_P(nxt, &menuItems[ui.menu][ui.subMenu + 1], STR_SZ);
                    if (nxt[0] == 0) ui.subMenu = 1;
                    else ui.subMenu++;
                    ui.setting = 100 * (ui.menu + 1) + ui.subMenu;
                } else {
                    if (ui.menu >= MENU_COUNT - 1) ui.menu = 0; else ui.menu++;
                    ui.subMenu = 1;
                    ui.setting = 100 * (ui.menu + 1) + ui.subMenu;
                }
            }
        }
    }

    if (btn2.pressed) {
        if (game.active) {
            if (game.gameOver) {
                resetMiniGame();
                game.active = true;
                spawnRunnerObstacle();
            }
        } else {
            if (pet.soundEnabled) tone(BUZZER, 600, 80);
            if (ui.menuOpened) {
                bool hasSubs = (pgm_read_byte(&menuItems[ui.menu][1][0]) != 0);
                if (hasSubs && !ui.menuDepth) {
                    ui.menuDepth = true;
                    ui.subMenu = 1;
                    ui.setting = 100 * (ui.menu + 1) + 1;
                } else {
                    ui.action = 100 * (ui.menu + 1) + ui.subMenu;
                    ui.setting = ui.action;
                }
            } else {
                ui.menuOpened = true;
                ui.menuDepth = true;
                ui.menu = 6; ui.subMenu = 1;
                ui.setting = 701;
            }
        }
    }

    if (btn3.pressed) {
        if (pet.soundEnabled) tone(BUZZER, 1000, 80);
        if (game.active || game.gameOver) {
            resetMiniGame();
            saveState();
        } else {
            if (!ui.menuDepth) {
                ui.menuOpened = false; ui.menu = 0; ui.setting = 0;
                saveState();
            } else {
                ui.menuDepth = false;
                ui.setting = 100 * (ui.menu + 1) + 1;
            }
            ui.action = 0;
            ui.subMenu = 1;
        }
    }

    if (game.active && !game.gameOver) {
        game.ducking = btn2.curr && !game.jumping;
    }
}

void executeAction() {
    if (ui.action == 0) return;

    int act = ui.action;
    if (act == 101 || act == 102 || act == 103) {
        if (!pet.sleeping) {
            ui.animType = (act == 103) ? 2 : 1;
            ui.animItem = act;
            ui.animStart = millis();
            foodSplashBaseDrawn = false;
            foodSplashBaseItem = 0;
            foodSplashOverlayStep = -1;
        }
        if (!pet.sleeping) {
            switch (act) {
                case 101:
                    applyFoodGain(APPLE_HUNGER_GAIN, true);
                    break;
                case 102:
                    applyFoodGain(FISH_HUNGER_GAIN, true);
                    break;
                case 103:
                    giveWater(true);
                    break;
            }
        }
    }
    else if (act == 201) {
        if (!pet.sleeping && pet.health > 20) {
            resetMiniGame();
            game.active = true;
            spawnRunnerObstacle();
            pet.playedThisCycle = true;
            pet.happiness = clampf(pet.happiness + PLAY_HAPPINESS_GAIN, 0, 100);
            ui.menuOpened = false;
        }
    }
    else if (act == 301) {
        if (pet.sleeping) {
            pet.sleeping = false;
            pet.weight -= WAKE_WEIGHT_LOSS;
            if (pet.weight < 0) pet.weight = 0;
        } else {
            pet.sleeping = true;
        }
        syncSceneClock();
    }
    else if (act == 401) {
        clearPoops();
    }
    else if (act == 501) {
        if (pet.health < 60) {
            pet.health = 100;
            ui.animType = 4;
            ui.animStart = millis();
        }
    }
    else if (act == 601) {
        if (!pet.sleeping) {
            pet.discipline = clampf(pet.discipline + TRAIN_DISCIPLINE_GAIN, 0, 100);
            pet.trainedThisCycle = true;
            ui.animType = 3;
            ui.animStart = millis();
            for (int i = 1; i <= 5; i++) { if (pet.soundEnabled) tone(BUZZER, 200 * i, 100); delay(50); }
        }
    }
    else if (act == 801) {
        pet.soundEnabled = !pet.soundEnabled;
    }

    ui.action = 0;
    enforceEndConditions(END_REASON_NONE);
    refreshWarningsAndNotification();
    saveState();
}

void drawSplashSpike(int cx, int cy) {
    uint16_t cOutline = SPINO_OUTLINE;
    uint16_t cDark = SPINO_DARK;
    uint16_t cMid = SPINO_MID;
    uint16_t cBody = SPINO_BODY;
    uint16_t cLight = SPINO_LIGHT;
    uint16_t cBelly = SPINO_BELLY;
    uint16_t cEyeBlue = 0x04BF;
    uint16_t cClaw = 0x1082;
    uint16_t cJaw = 0xB5B6;

    int sailBase = cy - 12;
    int sailLeft = cx - 43;
    int sailRight = cx + 42;
    int sailTop = cy - 67;
    tft.fillTriangle(sailLeft - 2, sailBase + 2, sailRight + 2, sailBase + 2, cx, sailTop, SAIL_ORANGE_H);
    tft.fillTriangle(sailLeft + 3, sailBase, sailRight - 3, sailBase, cx, sailTop + 7, SAIL_ORANGE);
    tft.fillTriangle(sailLeft + 15, sailBase, sailRight - 15, sailBase, cx, sailTop + 15, SAIL_ORANGE_D);

    const int peaks[7] = {28, 43, 55, 60, 51, 39, 24};
    for (int i = 0; i < 7; i++) {
        int px = sailLeft + 8 + i * 12;
        int py = sailBase - peaks[i];
        tft.fillCircle(px, py + 8, 11, SAIL_ORANGE_H);
        tft.fillCircle(px, py + 9, 8, SAIL_ORANGE);
        tft.fillTriangle(px - 12, sailBase, px + 12, sailBase, px, py, SAIL_ORANGE);
        tft.fillTriangle(px - 5, sailBase, px + 5, sailBase, px, py + 10, SAIL_ORANGE_D);
        tft.drawLine(px, sailBase, px, py + 10, SAIL_ORANGE_D);
        tft.drawLine(px + 1, sailBase, px + 1, py + 12, 0xFB20);
        tft.fillCircle(px - 3, py + 7, 3, SAIL_ORANGE_H);
    }
    tft.drawLine(sailLeft + 4, sailBase, sailRight - 4, sailBase, SAIL_ORANGE_D);

    // Tail behind the body: long, smooth, and slightly lifted at the tip.
    for (int i = 0; i < 86; i++) {
        int h = 18 - i / 5;
        if (h < 2) break;
        int tx = cx - 42 - i;
        int ty = cy - h / 2 - i / 16 + 4;
        for (int dy = 0; dy < h; dy++) {
            uint16_t c = (dy == 0 || dy == h - 1) ? cOutline : ((dy < h / 2) ? cLight : cMid);
            tft.drawPixel(tx, ty + dy, c);
        }
        if (i > 44 && i % 9 == 0) tft.drawFastVLine(tx, ty + h / 2, h / 2, cDark);
    }

    tftFillEllipse(cx, cy + 2, 50, 19, cOutline);
    tftFillEllipse(cx, cy + 1, 47, 17, cBody);
    tftFillEllipse(cx + 6, cy + 10, 35, 8, cBelly);
    tftFillEllipse(cx - 6, cy - 9, 34, 6, cLight);
    tft.drawLine(cx - 34, cy - 9, cx + 27, cy - 17, lerp565(cLight, 0xFFFF, 0.3f));

    int headCx = cx + 65;
    int headCy = cy - 15;
    tftFillEllipse(cx + 35, cy - 9, 18, 12, cOutline);
    tftFillEllipse(cx + 36, cy - 10, 16, 10, cMid);
    tftFillEllipse(cx + 37, cy - 15, 13, 6, cLight);
    tftFillEllipse(cx + 41, cy - 3, 12, 5, cBelly);

    tftFillEllipse(headCx - 5, headCy - 2, 18, 13, cOutline);
    tftFillEllipse(headCx - 5, headCy - 3, 16, 11, cMid);
    tftFillEllipse(headCx - 10, headCy - 8, 11, 5, cLight);
    tftFillEllipse(headCx - 3, headCy + 5, 11, 5, cBelly);

    int snoutX = headCx + 8;
    int snoutY = headCy - 4;
    for (int i = 0; i < 40; i++) {
        int h = 10 - i / 7;
        if (h < 4) h = 4;
        int sy = snoutY + i / 18;
        for (int dy = 0; dy < h; dy++) {
            uint16_t c = (dy == 0 || dy == h - 1) ? cOutline : ((dy < h / 2) ? cLight : cMid);
            tft.drawPixel(snoutX + i, sy + dy, c);
        }
    }
    tftFillEllipse(snoutX + 39, snoutY + 3, 5, 4, cDark);
    tft.drawPixel(snoutX + 37, snoutY + 1, 0x0000);
    tft.drawPixel(snoutX + 38, snoutY + 1, 0x0000);

    int mouthY = snoutY + 9;
    int lowerY = snoutY + 20;
    for (int i = 0; i < 38; i++) tft.drawFastVLine(snoutX + 1 + i, mouthY, lowerY - mouthY, MOUTH);
    tft.fillRect(snoutX + 5, lowerY - 5, 20, 4, TONGUE);
    tft.fillRect(snoutX + 5, lowerY - 6, 17, 1, 0xFD7F);
    for (int i = 0; i < 11; i++) {
        int tx = snoutX + 3 + i * 3;
        tft.fillTriangle(tx - 1, mouthY, tx + 1, mouthY, tx, mouthY + 3, TEETH);
    }
    for (int i = 0; i < 34; i++) {
        for (int dy = 0; dy < 5; dy++) {
            uint16_t c = (dy == 4) ? cOutline : cJaw;
            tft.drawPixel(snoutX + 2 + i, lowerY + dy, c);
        }
    }

    tft.fillCircle(headCx - 7, headCy - 11, 6, EYE_WHITE);
    tft.fillCircle(headCx - 7, headCy - 11, 4, cEyeBlue);
    tft.fillCircle(headCx - 6, headCy - 11, 2, EYE_PUPIL);
    tft.fillCircle(headCx - 9, headCy - 13, 1, 0xFFFF);
    tft.drawPixel(headCx + 3, headCy - 11, cOutline);

    // Back leg
    tftFillEllipse(cx - 13, cy + 18, 12, 17, cDark);
    tftFillEllipse(cx - 13, cy + 16, 10, 15, cMid);
    tft.fillRect(cx - 18, cy + 25, 10, 28, cDark);
    tft.fillRect(cx - 17, cy + 25, 8, 27, cMid);
    tftFillEllipse(cx - 6, cy + 55, 18, 6, cDark);
    tftFillEllipse(cx - 4, cy + 54, 15, 4, cMid);
    tft.fillTriangle(cx + 5, cy + 60, cx + 15, cy + 54, cx + 14, cy + 62, cClaw);
    tft.fillTriangle(cx - 4, cy + 61, cx + 6, cy + 54, cx + 6, cy + 62, cClaw);
    tft.fillTriangle(cx - 13, cy + 61, cx - 3, cy + 55, cx - 3, cy + 63, cClaw);

    // Front leg
    tftFillEllipse(cx + 18, cy + 17, 13, 18, cOutline);
    tftFillEllipse(cx + 18, cy + 15, 11, 16, cBody);
    tftFillEllipse(cx + 15, cy + 10, 7, 9, cLight);
    tft.fillRect(cx + 13, cy + 26, 11, 30, cMid);
    tft.fillRect(cx + 14, cy + 26, 8, 29, cLight);
    tftFillEllipse(cx + 28, cy + 59, 20, 6, cDark);
    tftFillEllipse(cx + 29, cy + 58, 17, 4, cMid);
    tft.fillTriangle(cx + 41, cy + 64, cx + 52, cy + 57, cx + 50, cy + 66, cClaw);
    tft.fillTriangle(cx + 30, cy + 65, cx + 42, cy + 57, cx + 41, cy + 66, cClaw);
    tft.fillTriangle(cx + 20, cy + 65, cx + 31, cy + 58, cx + 30, cy + 66, cClaw);

    // Small arms with dark claws
    int shX = cx + 23;
    int shY = cy + 3;
    int elX = shX + 9;
    int elY = shY + 13;
    int wrX = elX + 5;
    int wrY = elY + 8;
    tft.drawLine(shX, shY - 1, elX, elY - 1, cOutline);
    tft.drawLine(shX, shY, elX, elY, cMid);
    tft.drawLine(shX + 1, shY + 1, elX + 1, elY + 1, cLight);
    tft.drawLine(elX, elY - 1, wrX, wrY - 1, cOutline);
    tft.drawLine(elX, elY, wrX, wrY, cMid);
    tft.drawLine(wrX, wrY, wrX + 8, wrY + 4, cClaw);
    tft.drawLine(wrX - 1, wrY + 1, wrX + 6, wrY + 7, cClaw);
    tft.drawLine(wrX - 2, wrY + 2, wrX + 4, wrY + 10, cClaw);
}

void fillFoodSplashBgRect(int x, int y, int w, int h, uint16_t top, uint16_t mid, uint16_t bot) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;

    for (int yy = y; yy < y + h; yy++) {
        float t = yy / 239.0f;
        uint16_t c = (t < 0.55f) ? lerp565(top, mid, t / 0.55f)
                                  : lerp565(mid, bot, (t - 0.55f) / 0.45f);
        tft.drawFastHLine(x, yy, w, c);
    }
}

void renderFoodSplashAnimation(int item, unsigned long elapsed) {
    float p = elapsed / (float)FOOD_SPLASH_MS;
    if (p > 1.0f) p = 1.0f;

    uint16_t top = (item == 103) ? 0x041F : 0x7E7F;
    uint16_t mid = (item == 101) ? 0xFF9D : ((item == 102) ? 0x7E7F : 0x6DFF);
    uint16_t bot = (item == 103) ? 0x1B47 : 0xB7B0;
    if (!foodSplashBaseDrawn || foodSplashBaseItem != item) {
        fillFoodSplashBgRect(0, 0, SCREEN_W, SCREEN_H, top, mid, bot);

        tftFillEllipse(125, 218, 112, 10, 0x8C92);
        tftFillEllipse(125, 216, 96, 6, 0xBDF7);
        drawSplashSpike(104, 148);
        foodSplashBaseDrawn = true;
        foodSplashBaseItem = item;
        foodSplashOverlayStep = -1;
        foodSplashPrevRectValid = false;
    }

    int overlayStep = (p < 0.62f) ? 0 : 1;
    if (overlayStep == foodSplashOverlayStep) return;
    foodSplashOverlayStep = overlayStep;

    int mouthX = 214;
    int mouthY = 143;
    int itemX = 270;
    int itemY = (item == 103) ? 84 : 96;
    bool eaten = overlayStep == 1;

    fillFoodSplashBgRect(202, 32, 116, 120, top, mid, bot);

    if (item == 101) {
        tftFillEllipse(itemX + 3, itemY + 24, 24, 5, 0x8C51);
        tft.fillCircle(itemX - 9, itemY, 19, APPLE_SHADOW);
        tft.fillCircle(itemX + 7, itemY, 19, APPLE_RED);
        tft.fillCircle(itemX - 4, itemY - 2, 20, APPLE_RED);
        tft.fillCircle(itemX - 10, itemY - 10, 8, APPLE_HIGH);
        tft.fillCircle(itemX - 13, itemY - 13, 3, 0xFFFF);
        tft.fillRect(itemX - 2, itemY - 27, 5, 12, APPLE_STEM);
        tft.fillTriangle(itemX + 2, itemY - 25, itemX + 24, itemY - 30, itemX + 12, itemY - 15, APPLE_LEAF);
        if (eaten) {
            tft.fillCircle(itemX + 17, itemY - 7, 10, mid);
            tft.fillCircle(itemX + 20, itemY + 5, 8, mid);
        }
    } else if (item == 102) {
        tftFillEllipse(itemX + 5, itemY + 25, 34, 5, 0x2965);
        tft.fillTriangle(itemX - 32, itemY, itemX - 58, itemY - 19, itemX - 58, itemY + 19, 0x3A5F);
        tft.fillTriangle(itemX - 31, itemY, itemX - 48, itemY - 11, itemX - 48, itemY + 11, 0x7E7F);
        tftFillEllipse(itemX, itemY, 36, 18, 0x3A5F);
        tftFillEllipse(itemX + 3, itemY - 2, 32, 15, 0x5CFE);
        tftFillEllipse(itemX + 7, itemY - 7, 20, 7, 0xDFFF);
        tft.fillTriangle(itemX - 7, itemY - 15, itemX + 10, itemY - 32, itemX + 19, itemY - 14, 0x3A5F);
        tft.fillTriangle(itemX - 3, itemY + 10, itemX + 13, itemY + 24, itemX + 24, itemY + 10, 0x5CFE);
        tft.fillCircle(itemX + 25, itemY - 3, 5, 0xFFFF);
        tft.fillCircle(itemX + 27, itemY - 3, 2, 0x0000);
        if (eaten) tft.fillCircle(itemX + 26, itemY + 6, 13, mid);
    } else {
        int cupX = itemX;
        int cupY = itemY;
        tft.fillRoundRect(cupX - 18, cupY - 22, 36, 42, 5, 0xFFFF);
        tft.drawRoundRect(cupX - 18, cupY - 22, 36, 42, 5, 0x2945);
        if (!eaten) {
            tft.fillRoundRect(cupX - 14, cupY - 17, 28, 34, 4, WATER_BLUE);
            tft.drawFastHLine(cupX - 12, cupY - 8, 24, WATER_FOAM);
        } else {
            tft.fillRoundRect(cupX - 14, cupY + 6, 28, 10, 4, WATER_BLUE);
            for (int i = 0; i < 7; i++) {
                int dx = mouthX + 4 + i * 5;
                int dy = mouthY - 18 + (i % 3) * 6;
                tft.fillCircle(dx, dy, 3, (i % 2 == 0) ? WATER_FOAM : WATER_LIGHT);
            }
        }
    }

    if (eaten) {
        tft.setTextSize(3);
        tft.setTextColor(0x0000);
        tft.setCursor(24, 28);
        tft.print(item == 103 ? "GLUG!" : "CHOMP!");
        tft.setTextColor(item == 103 ? WATER_FOAM : SAIL_ORANGE_H);
        tft.setCursor(22, 26);
        tft.print(item == 103 ? "GLUG!" : "CHOMP!");
        for (int i = 0; i < 10; i++) {
            int px = mouthX + ((i * 19 + elapsed / 18) % 58) - 24;
            int py = mouthY + ((i * 13 + elapsed / 24) % 42) - 20;
            tft.fillCircle(px, py, 2, item == 103 ? WATER_FOAM : TEETH);
        }
    }
}

uint16_t computeSplashBgColor(int x, int y) {
    (void)x;

    uint16_t skyTop = lerp565(SKY_DAY_TOP, 0x7DFF, 0.52f);
    uint16_t skyMid = lerp565(SKY_DAY_MID, 0xCFFF, 0.20f);
    uint16_t skyHorizon = lerp565(SKY_DAY_HORIZON, 0xFFE0, 0.18f);
    float ty = y / (float)(SCREEN_H - 1);
    return (ty < 0.48f)
        ? lerp565(skyTop, skyMid, ty / 0.48f)
        : lerp565(skyMid, skyHorizon, (ty - 0.48f) / 0.52f);
}

bool splashButtonPressed() {
    return !digitalRead(BTN_LEFT) || !digitalRead(BTN_SEL) || !digitalRead(BTN_RIGHT);
}

void drawScaledSplashSpikey(int dstX, int dstY, float scale) {
    const int srcW = 150;
    const int srcH = 80;
    const int logicalX = 40;
    const int logicalY = 10;
    const uint16_t transparentKey = 0x0001;

    for (stripOffsetY = 0; stripOffsetY < srcH; stripOffsetY += STRIP_H) {
        strip.fillScreen(transparentKey);
        drawSpikey(logicalX, logicalY, POSE_WALK1, true);

        uint16_t* buf = strip.getBuffer();
        int rows = srcH - stripOffsetY;
        if (rows > STRIP_H) rows = STRIP_H;

        for (int row = 0; row < rows; row++) {
            int srcY = stripOffsetY + row;
            int y0 = dstY + (int)(srcY * scale);
            int y1 = dstY + (int)((srcY + 1) * scale);
            if (y1 <= y0) y1 = y0 + 1;
            int rowBase = row * SCREEN_W;
            for (int x = 0; x < srcW; x++) {
                uint16_t color = buf[rowBase + x];
                if (color == transparentKey) {
                    continue;
                }

                int x0 = dstX + (int)(x * scale);
                int x1 = dstX + (int)((x + 1) * scale);
                if (x1 <= x0) x1 = x0 + 1;
                tft.fillRect(x0, y0, x1 - x0, y1 - y0, color);
            }
        }
    }

    stripOffsetY = 0;
}

void renderSplashScreen() {
    tft.startWrite();
    for (stripOffsetY = 0; stripOffsetY < SCREEN_H; stripOffsetY += STRIP_H) {
        uint16_t* buf = strip.getBuffer();
        for (int row = 0; row < STRIP_H; row++) {
            int y = stripOffsetY + row;
            for (int x = 0; x < SCREEN_W; x++) {
                buf[row * SCREEN_W + x] = computeSplashBgColor(x, y);
            }
        }

        fillCircW(78, 54, 22, lerp565(SUN_GLOW, 0xFFFF, 0.30f));
        fillCircW(78, 54, 16, SUN_GLOW);
        fillCircW(78, 54, 11, SUN_COLOR);
        fillCircW(74, 50, 4, 0xFFFC);
        renderMountains();

        tft.setAddrWindow(0, stripOffsetY, SCREEN_W, STRIP_H);
        tft.writePixels(strip.getBuffer(), SCREEN_W * STRIP_H);
    }
    tft.endWrite();

    tftFillEllipse(160, 208, 76, 8, 0x9C52);
    tftFillEllipse(160, 205, 60, 5, 0xC658);
    drawScaledSplashSpikey(55, 94, 1.4f);

    tft.setTextSize(5);
    tft.setTextColor(0x4208);
    tft.setCursor(73, 12);
    tft.print("Spikey");
    tft.setTextColor(0xFA60);
    tft.setCursor(71, 10);
    tft.print("Spikey");
    tft.setTextColor(0xFFFF);
    tft.setCursor(70, 9);
    tft.print("Spikey");

    tft.setTextSize(1);
    tft.setTextColor(0x0000);
    tft.setCursor(113, 55);
    tft.print("by Benjamin Klein");
    tft.setTextColor(0xFFFF);
    tft.setCursor(112, 54);
    tft.print("by Benjamin Klein");

    tft.setTextSize(2);
    tft.setTextColor(0x0000);
    tft.setCursor(82, 214);
    tft.print("Press Any Key");

    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(119, 231);
    tft.print("v");
    tft.print(SPIKEY_VERSION);
}

void showSplash() {
    if (pet.soundEnabled) {
        tone(BUZZER, 523, 120); delay(130);
        tone(BUZZER, 659, 120); delay(130);
        tone(BUZZER, 784, 120); delay(130);
        tone(BUZZER, 1047, 250);
    } else {
        delay(320);
    }

    renderSplashScreen();

    while (splashButtonPressed()) {
        delay(10);
    }

    while (true) {
        if (splashButtonPressed()) break;
        delay(10);
    }

    while (!digitalRead(BTN_LEFT) || !digitalRead(BTN_SEL) || !digitalRead(BTN_RIGHT)) {
        delay(10);
    }

    btn1 = Button();
    btn2 = Button();
    btn3 = Button();
}

const char* getEndReasonText() {
    switch (pet.deathReason) {
        case END_REASON_STARVATION:  return "Starved";
        case END_REASON_DEHYDRATION: return "Dehydrated";
        case END_REASON_LONELINESS:  return "Lonely";
        case END_REASON_FILTH:       return "Too Much Poop";
        case END_REASON_WEIGHT:      return "Too Heavy";
        case END_REASON_OLD_AGE:     return "Old Age";
        case END_REASON_INJURY:      return "Injury";
        default:                     return "Unknown";
    }
}

void renderDeathScreen() {
    if (deathScreenDrawn) return;

    uint16_t topColor = pet.victory ? 0x032A : 0x2104;
    uint16_t bottomColor = pet.victory ? 0x0146 : 0x0841;
    for (int y = 0; y < SCREEN_H; y++) {
        float t = y / 239.0f;
        uint16_t color = lerp565(topColor, bottomColor, t);
        tft.drawFastHLine(0, y, SCREEN_W, color);
    }

    int cx = 160, cy = 110;
    if (pet.victory) {
        tftFillEllipse(cx, cy + 15, 55, 10, 0x1082);
        tftFillEllipse(cx, cy + 10, 50, 15, SPINO_DARK);
        tftFillEllipse(cx, cy + 5, 45, 10, SPINO_BODY);
        tft.setTextSize(2);
        tft.setTextColor(UI_HIGHLIGHT);
        tft.setCursor(118, 100);
        tft.print("100Y");
        for (int i = 0; i < 5; i++) {
            int sx = 70 + i * 44;
            tft.drawPixel(sx, 142, UI_HIGHLIGHT);
            tft.drawFastHLine(sx - 2, 142, 5, UI_HIGHLIGHT);
            tft.drawFastVLine(sx, 140, 5, UI_HIGHLIGHT);
        }
    } else {
        tftFillEllipse(cx, cy + 15, 55, 10, 0x1082);
        tftFillEllipse(cx, cy + 10, 50, 15, SPINO_DARK);
        tftFillEllipse(cx, cy + 5, 45, 10, SPINO_MID);
        tft.drawLine(cx - 20, cy - 5, cx - 10, cy + 5, UI_DANGER);
        tft.drawLine(cx - 20, cy + 5, cx - 10, cy - 5, UI_DANGER);
        tft.drawLine(cx + 10, cy - 5, cx + 20, cy + 5, UI_DANGER);
        tft.drawLine(cx + 10, cy + 5, cx + 20, cy - 5, UI_DANGER);
    }

    tft.setTextSize(4);
    tft.setTextColor(pet.victory ? UI_HIGHLIGHT : UI_DANGER);
    tft.setCursor(50, 20);
    tft.print("SPIKEY");
    tft.setCursor(pet.victory ? 52 : 85, 60);
    tft.print(pet.victory ? "WINS" : "DIED");

    tft.setTextSize(2);
    tft.setTextColor(UI_TEXT);
    if (pet.victory) {
        tft.setCursor(56, 164);
        tft.print("Reached 100 years!");
    } else {
        tft.setCursor(36, 160);
        tft.print("Reason: ");
        tft.print(getEndReasonText());
    }
    tft.setCursor(46, 190);
    tft.print("Press Any Button");
    tft.setCursor(90, 214);
    tft.print("to restart");
    deathScreenDrawn = true;
}

void setup() {
    Serial.begin(115200);
    pinMode(BTN_LEFT,  INPUT_PULLUP);
    pinMode(BTN_SEL,   INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BUZZER,    OUTPUT);

    tft.init(240, 320);
    tft.setRotation(3);
    tft.invertDisplay(false);
    tft.setSPISpeed(40000000);
    tft.fillScreen(0x0000);

    randomSeed(analogRead(A0) + micros());
    for (int i = 0; i < 14; i++) {
        scene.stars[i][0] = random(10, SCREEN_W - 10);
        scene.stars[i][1] = random(8, 80);
    }

    Serial.println(F("=== SPIKEY v" SPIKEY_VERSION " ==="));
    Serial.println(F(SPIKEY_LICENSE_NAME));
    Serial.println(F(SPIKEY_COPYRIGHT_NOTICE));
    if (loadState()) {
        Serial.println(F("Save restored"));
    } else {
        Serial.println(F("Fresh start"));
        resetPetProgress();
    }
    normalizeLoadedState();
    syncSceneClock();
    scene.prevGrassX = scene.grassX;
    scene.prevTreesX = scene.treesX;
    scene.prevCloud1X = scene.cloud1X;
    scene.prevCloud2X = scene.cloud2X;
    scene.prevSunX = scene.sunX;
    refreshWarningsAndNotification();

    showSplash();
    tft.fillScreen(0x0000);
    lastUpdate = lastMotionUpdate = lastFrame = lastSave = millis();
}

void loop() {
    unsigned long now = millis();

    btn1.update(!digitalRead(BTN_LEFT));
    btn2.update(!digitalRead(BTN_SEL));
    btn3.update(!digitalRead(BTN_RIGHT));
    handleButtons();

    if (!pet.dead && game.active && now - lastMotionUpdate >= FRAME_MS) {
        unsigned long elapsedMs = now - lastMotionUpdate;
        updateScenery(elapsedMs);
        updateGameLogic(elapsedMs);
        lastMotionUpdate = now;
    } else if (!game.active) {
        lastMotionUpdate = now;
    }

    if (now - lastUpdate >= 100) {
        unsigned long elapsedMs = now - lastUpdate;
        if (!pet.dead) {
            if (!pet.dead) executeAction();
            if (!pet.dead) updatePetStats(elapsedMs);
            if (!pet.dead && !game.active) updateScenery(elapsedMs);
            if (!pet.dead) {
                if (!game.active) updateWalkAnimation();
            }
            if (!pet.dead && ui.animType != 0) {
                unsigned long animLen = (ui.animType == 1 || ui.animType == 2) ? FOOD_SPLASH_MS : 1800;
                if (now - ui.animStart > animLen) {
                    ui.animType = 0;
                    ui.animItem = 0;
                    foodSplashBaseDrawn = false;
                    foodSplashBaseItem = 0;
                    foodSplashOverlayStep = -1;
                    foodSplashPrevRectValid = false;
                }
            }
            if (!pet.dead) deathScreenDrawn = false;
        }
        lastUpdate = now;
    }

    if (now - lastFrame >= FRAME_MS) {
        if (!pet.dead) {
            float t = game.active ? 1.0f : ((float)(now - lastUpdate) / 100.0f);
            if (t > 1.0f) t = 1.0f;
            renderFrame(t);
        } else {
            renderDeathScreen();
        }
        lastFrame = now;
    }

    if (now - lastSave >= 30000) {
        saveState();
        lastSave = now;
    }
}
