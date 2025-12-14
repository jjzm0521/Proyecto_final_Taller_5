#ifndef INC_GALAGA_GAME_H_
#define INC_GALAGA_GAME_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

// ============================================================================
// SCREEN & GAME CONSTANTS
// ============================================================================
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

// Player Ship
#define SHIP_WIDTH 24
#define SHIP_HEIGHT 20
#define SHIP_Y (SCREEN_HEIGHT - 40) // Near bottom
#define SHIP_SPEED 4

// Bullets
#define MAX_BULLETS 5
#define BULLET_WIDTH 3
#define BULLET_HEIGHT 8
#define BULLET_SPEED 10

// Enemies
#define MAX_ENEMIES 24
#define ENEMY_WIDTH 16
#define ENEMY_HEIGHT 12
#define ENEMY_SPEED_X 2
#define ENEMY_DROP_DIST 15

// Enemy formation
#define ENEMIES_PER_ROW 8
#define ENEMY_SPACING_X 36
#define ENEMY_SPACING_Y 30
#define FORMATION_START_X 30
#define FORMATION_START_Y 60

// Timing
#define SHOOT_COOLDOWN 300   // ms between shots
#define GALAGA_GAME_DELAY 16 // ~60 FPS

// ============================================================================
// DATA STRUCTURES
// ============================================================================

typedef struct {
  int16_t x;
  int16_t y;
  int8_t dx; // Movement direction (-1 left, 1 right)
} PlayerShip;

typedef struct {
  int16_t x;
  int16_t y;
  uint8_t active;
} Bullet;

typedef enum {
  ENEMY_TYPE_BEE = 0,   // Yellow - bottom rows
  ENEMY_TYPE_BUTTERFLY, // Blue/Purple - middle rows
  ENEMY_TYPE_BOSS       // Red - top row
} EnemyType;

typedef struct {
  int16_t x;
  int16_t y;
  EnemyType type;
  uint8_t active;
} Enemy;

typedef enum {
  GAME_STATE_PLAYING = 0,
  GAME_STATE_WIN,
  GAME_STATE_GAMEOVER
} GameStateEnum;

typedef struct {
  uint16_t score;
  uint8_t enemies_alive;
  GameStateEnum state;
  uint32_t last_shot_time;
  int8_t formation_dx; // Direction of enemy formation movement
} GameState;

// ============================================================================
// COLORS (RGB565)
// ============================================================================
#define COL_BLACK 0x0000
#define COL_WHITE 0xFFFF
#define COL_RED 0xF800
#define COL_BLUE 0x001F
#define COL_YELLOW 0xFFE0
#define COL_CYAN 0x07FF
#define COL_MAGENTA 0xF81F
#define COL_GREEN 0x07E0
#define COL_ORANGE 0xFD20
#define COL_PURPLE 0x780F
#define COL_DARKBLUE 0x0010

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

/**
 * @brief Initialize the Galaga game
 */
void Galaga_Init(void);

/**
 * @brief Main game loop - call repeatedly from main while loop
 */
void Galaga_Loop(void);

/**
 * @brief Set player X position (for accelerometer control)
 * @param x Position in pixels (0 to SCREEN_WIDTH)
 */
void Galaga_SetPlayerX(int16_t x);

/**
 * @brief Fire a bullet (for joystick/ADC control)
 */
void Galaga_Fire(void);

/**
 * @brief Check if game is currently active (playing)
 * @return 1 if playing, 0 if game over or won
 */
uint8_t Galaga_IsActive(void);

/**
 * @brief Enable/disable manual control mode
 * @param enable 1 for manual control, 0 for auto-play
 */
void Galaga_SetManualControl(uint8_t enable);

#endif /* INC_GALAGA_GAME_H_ */
