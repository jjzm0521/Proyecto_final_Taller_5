/**
 * @file racing_game.h
 * @brief Space Racing Game - Galaga style
 *
 * Vertical scrolling game: dodge obstacles falling from top
 * Controls:
 *   - Accelerometer X: Move left/right
 *   - Joystick Y (ADC): Move up/down
 */

#ifndef INC_RACING_GAME_H_
#define INC_RACING_GAME_H_

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
#define SHIP_START_X (SCREEN_WIDTH / 2)
#define SHIP_START_Y (SCREEN_HEIGHT - 60)
#define SHIP_MIN_Y 100
#define SHIP_MAX_Y (SCREEN_HEIGHT - 30)
#define SHIP_MIN_X 15
#define SHIP_MAX_X (SCREEN_WIDTH - 15)

// Movement speeds
#define SHIP_SPEED_X 4
#define SHIP_SPEED_Y 3

// Obstacles
#define RACING_MAX_OBSTACLES 8
#define RACING_OBSTACLE_WIDTH 16
#define RACING_OBSTACLE_HEIGHT 14
#define OBSTACLE_BASE_SPEED 4
#define OBSTACLE_MAX_SPEED 12

// Stars background
#define RACING_MAX_STARS 30

// Game timing
#define RACING_GAME_DELAY 16 // ~60 FPS
#define RACING_SPEED_INCREMENT_SCORE 200
#define RACING_SPAWN_INTERVAL_MIN 20
#define RACING_SPAWN_INTERVAL_MAX 60

// Input thresholds
#define RACING_ACCEL_DEADZONE 2000 // Ignore small tilts
#define RACING_ACCEL_MAX 12000     // Max tilt value for full speed
#define RACING_ADC_CENTER 2048
#define RACING_ADC_DEADZONE 400

// ============================================================================
// DATA STRUCTURES
// ============================================================================

typedef struct {
  int16_t x;
  int16_t y;
} RacingPlayerShip;

typedef enum {
  RACING_OBS_ENEMY_BEE = 0,
  RACING_OBS_ENEMY_BUTTERFLY,
  RACING_OBS_ENEMY_BOSS,
  RACING_OBS_ASTEROID
} RacingObstacleType;

typedef struct {
  int16_t x;
  int16_t y;
  RacingObstacleType type;
  uint8_t active;
  int8_t speed;
} RacingObstacle;

typedef struct {
  int16_t x;
  int16_t y;
  uint8_t brightness; // 0-2
} RacingStar;

typedef enum {
  RACING_GAME_PLAYING = 0,
  RACING_GAME_OVER,
  RACING_GAME_PAUSED
} RacingGameStateEnum;

typedef struct {
  uint32_t score;
  uint32_t hi_score;
  uint8_t speed;
  RacingGameStateEnum state;
  uint32_t frame_count;
  uint32_t next_spawn_frame;
} RacingGame;

// ============================================================================
// COLORS (RGB565) - Galaga style
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
#define COL_DARKGRAY 0x4208
#define COL_GRAY 0x8410

// Star colors (dim)
#define RACING_COL_STAR_DIM 0x4208
#define RACING_COL_STAR_MED 0x8410
#define RACING_COL_STAR_BRIGHT 0xFFFF

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

/**
 * @brief Initialize the Racing Game
 * @param hadc Pointer to ADC handle for joystick Y
 * @param hi2c Pointer to I2C handle for accelerometer
 */
void Racing_Init(ADC_HandleTypeDef *hadc, I2C_HandleTypeDef *hi2c);

/**
 * @brief Main game loop - call repeatedly from main while loop
 */
void Racing_Loop(void);

#endif /* INC_RACING_GAME_H_ */
