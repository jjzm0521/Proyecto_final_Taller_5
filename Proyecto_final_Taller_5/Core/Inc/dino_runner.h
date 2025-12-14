/**
 * @file dino_runner.h
 * @brief T-Rex Runner Game - Chrome Dinosaur Style
 *
 * Endless runner game optimized for ILI9486 display (320x480)
 * Control: Joystick ADC (IN0=X, IN1=Y)
 *   - Y arriba (>3000): Saltar
 *   - Y abajo (<1000): Agacharse
 */

#ifndef INC_DINO_RUNNER_H_
#define INC_DINO_RUNNER_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

// ============================================================================
// SCREEN & GAME CONSTANTS
// ============================================================================
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

// Ground level (from top)
#define GROUND_Y 400
#define GROUND_THICKNESS 2

// T-Rex dimensions
#define DINO_WIDTH 24
#define DINO_HEIGHT 26
#define DINO_DUCK_WIDTH 32
#define DINO_DUCK_HEIGHT 16
#define DINO_START_X 40

// Physics
#define JUMP_VELOCITY -18
#define GRAVITY 2
#define MAX_FALL_SPEED 15

// Obstacles
#define DINO_MAX_OBSTACLES 3
#define DINO_OBSTACLE_SPAWN_MIN 80
#define DINO_OBSTACLE_SPAWN_MAX 200

// Cactus sizes
#define CACTUS_SMALL_W 12
#define CACTUS_SMALL_H 24
#define CACTUS_LARGE_W 16
#define CACTUS_LARGE_H 36

// Bird (pterodactyl)
#define BIRD_WIDTH 24
#define BIRD_HEIGHT 16

// Game timing
#define DINO_GAME_DELAY 16 // ~60 FPS
#define DINO_SPEED_INCREMENT_SCORE 100
#define INITIAL_SPEED 6
#define MAX_SPEED 14

// ADC thresholds (12-bit: 0-4095)
#define DINO_ADC_CENTER 2048
#define DINO_ADC_DEADZONE 500
#define DINO_ADC_UP_THRESHOLD (DINO_ADC_CENTER + DINO_ADC_DEADZONE)
#define DINO_ADC_DOWN_THRESHOLD (DINO_ADC_CENTER - DINO_ADC_DEADZONE)

// ============================================================================
// DATA STRUCTURES
// ============================================================================

typedef enum { DINO_STATE_RUN = 0, DINO_STATE_JUMP, DINO_STATE_DUCK } DinoState;

typedef struct {
  int16_t x;
  int16_t y;
  int16_t vy; // Vertical velocity
  DinoState state;
  uint8_t anim_frame; // For running animation (0 or 1)
  uint8_t on_ground;
} Dino;

typedef enum {
  DINO_OBS_CACTUS_SMALL = 0,
  DINO_OBS_CACTUS_LARGE,
  DINO_OBS_BIRD_LOW,
  DINO_OBS_BIRD_HIGH
} DinoObstacleType;

typedef struct {
  int16_t x;
  int16_t y;
  DinoObstacleType type;
  uint8_t active;
  uint8_t anim_frame; // For bird wing animation
} DinoObstacle;

typedef enum {
  DINO_GAME_PLAYING = 0,
  DINO_GAME_OVER,
  DINO_GAME_RESTART
} DinoGameStateEnum;

typedef struct {
  uint32_t score;
  uint32_t hi_score;
  uint8_t speed;
  DinoGameStateEnum state;
  uint32_t last_obstacle_x;
  uint32_t frame_count;
} DinoGame;

typedef enum { INPUT_NONE = 0, INPUT_UP, INPUT_DOWN } JoystickInput;

// ============================================================================
// COLORS (RGB565) - Chrome Dino grayscale theme
// ============================================================================
#define DINO_COL_BG 0xFFFF       // White background
#define DINO_COL_DINO 0x5285     // Dark gray (#535353)
#define DINO_COL_GROUND 0x5285   // Same as dino
#define DINO_COL_OBSTACLE 0x5285 // Same gray
#define DINO_COL_TEXT 0x5285     // Score text
#define COL_BLACK 0x0000

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

/**
 * @brief Initialize the Dino Runner game
 * @param hadc Pointer to ADC handle for joystick input
 */
void Dino_Init(ADC_HandleTypeDef *hadc);

/**
 * @brief Main game loop - call repeatedly from main while loop
 */
void Dino_Loop(void);

/**
 * @brief Trigger a jump (can be called externally)
 */
void Dino_Jump(void);

/**
 * @brief Set duck state (can be called externally)
 * @param duck 1 to duck, 0 to stop ducking
 */
void Dino_Duck(uint8_t duck);

#endif /* INC_DINO_RUNNER_H_ */
