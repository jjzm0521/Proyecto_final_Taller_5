/**
 * @file racing_game.c
 * @brief Space Racing Game Implementation
 */

#include "racing_game.h"
#include "STM32_GY521.h"
#include "TFT_ILI9486.h"
// ============================================================================
// GAME STATE VARIABLES
// ============================================================================
static RacingPlayerShip player;
static RacingObstacle obstacles[RACING_MAX_OBSTACLES];
static RacingStar stars[RACING_MAX_STARS];
static RacingGame game;

static ADC_HandleTypeDef *hadc_joystick = NULL;
static I2C_HandleTypeDef *hi2c_accel = NULL;

// Sensor GY521 interno para racing game
static GY521 gy521_sensor;
static uint8_t gy521_ok = 0;

// ADC DMA buffer
static volatile uint16_t adc_buffer[2] = {2048, 2048};

// Random seed
static uint32_t rand_seed = 12345;

static uint32_t simple_rand(void) {
  rand_seed = rand_seed * 1103515245 + 12345;
  return (rand_seed >> 16) & 0x7FFF;
}

// ============================================================================
// PLAYER SHIP SPRITE (Galaga style)
// ============================================================================

static void DrawPlayerShip(int16_t cx, int16_t cy, uint8_t erase) {
  uint16_t white = erase ? COL_BLACK : COL_WHITE;
  uint16_t red = erase ? COL_BLACK : COL_RED;
  uint16_t blue = erase ? COL_BLACK : COL_BLUE;

  // Central tower (top spike)
  ILI_FillRect(cx - 1, cy - 10, 3, 4, white);

  // Main body top section
  ILI_FillRect(cx - 3, cy - 6, 7, 3, white);

  // Cockpit (red center)
  ILI_FillRect(cx - 1, cy - 4, 3, 2, red);

  // Main body middle
  ILI_FillRect(cx - 5, cy - 3, 11, 4, white);

  // Blue accents on body
  ILI_FillRect(cx - 4, cy - 2, 2, 2, blue);
  ILI_FillRect(cx + 3, cy - 2, 2, 2, blue);

  // Wings extending outward
  ILI_FillRect(cx - 9, cy, 4, 5, white);
  ILI_FillRect(cx + 6, cy, 4, 5, white);

  // Wing tips
  ILI_FillRect(cx - 11, cy - 4, 3, 9, white);
  ILI_FillRect(cx + 9, cy - 4, 3, 9, white);

  // Red accents on wings
  ILI_FillRect(cx - 10, cy - 2, 2, 4, red);
  ILI_FillRect(cx + 9, cy - 2, 2, 4, red);

  // Lower body sections
  ILI_FillRect(cx - 7, cy + 2, 3, 4, white);
  ILI_FillRect(cx + 5, cy + 2, 3, 4, white);

  // Red accents lower
  ILI_FillRect(cx - 6, cy + 3, 2, 2, red);
  ILI_FillRect(cx + 5, cy + 3, 2, 2, red);

  // Central lower section
  ILI_FillRect(cx - 3, cy + 1, 7, 5, white);

  // Bottom red accent
  ILI_FillRect(cx - 2, cy + 4, 5, 2, red);
}

static void ErasePlayerShip(int16_t cx, int16_t cy) {
  // La nave ocupa desde cx-11 hasta cx+11 (23px) y cy-10 hasta cy+6 (17px)
  // Agregar margen extra para asegurar borrado completo
  ILI_FillRect(cx - 13, cy - 12, 27, 20, COL_BLACK);
}

// ============================================================================
// OBSTACLE SPRITES
// ============================================================================

static void DrawObstacle(RacingObstacle *o, uint8_t erase) {
  uint16_t color1, color2;

  if (erase) {
    color1 = COL_BLACK;
    color2 = COL_BLACK;
  } else {
    switch (o->type) {
    case RACING_OBS_ENEMY_BEE:
      color1 = COL_YELLOW;
      color2 = COL_ORANGE;
      break;
    case RACING_OBS_ENEMY_BUTTERFLY:
      color1 = COL_BLUE;
      color2 = COL_CYAN;
      break;
    case RACING_OBS_ENEMY_BOSS:
      color1 = COL_RED;
      color2 = COL_MAGENTA;
      break;
    case RACING_OBS_ASTEROID:
    default:
      color1 = COL_GRAY;
      color2 = COL_DARKGRAY;
      break;
    }
  }

  int16_t x = o->x;
  int16_t y = o->y;

  // Enemy body
  ILI_FillRect(x - 4, y - 3, 9, 6, color1);

  // Wings/antennae
  ILI_FillRect(x - 7, y - 1, 3, 4, color2);
  ILI_FillRect(x + 5, y - 1, 3, 4, color2);

  // Eyes
  if (!erase) {
    ILI_FillRect(x - 2, y - 2, 2, 2, COL_WHITE);
    ILI_FillRect(x + 1, y - 2, 2, 2, COL_WHITE);
  }

  // Top details
  ILI_FillRect(x - 2, y - 5, 2, 2, color2);
  ILI_FillRect(x + 1, y - 5, 2, 2, color2);
}

static void EraseObstacle(RacingObstacle *o) {
  ILI_FillRect(o->x - 8, o->y - 6, 17, 13, COL_BLACK);
}

// ============================================================================
// STARS BACKGROUND
// ============================================================================

static void InitStars(void) {
  for (int i = 0; i < RACING_MAX_STARS; i++) {
    stars[i].x = simple_rand() % SCREEN_WIDTH;
    stars[i].y = simple_rand() % SCREEN_HEIGHT;
    stars[i].brightness = simple_rand() % 3;
  }
}

static void UpdateStars(void) {
  for (int i = 0; i < RACING_MAX_STARS; i++) {
    // Erase old position
    ILI_FillRect(stars[i].x, stars[i].y, 2, 2, COL_BLACK);

    // Move star down (parallax based on brightness)
    stars[i].y += 1 + stars[i].brightness;

    // Wrap around
    if (stars[i].y >= SCREEN_HEIGHT) {
      stars[i].y = 0;
      stars[i].x = simple_rand() % SCREEN_WIDTH;
    }

    // Draw new position
    uint16_t color;
    switch (stars[i].brightness) {
    case 0:
      color = RACING_COL_STAR_DIM;
      break;
    case 1:
      color = RACING_COL_STAR_MED;
      break;
    default:
      color = RACING_COL_STAR_BRIGHT;
      break;
    }
    ILI_FillRect(stars[i].x, stars[i].y, 1 + stars[i].brightness / 2, 1, color);
  }
}

// ============================================================================
// INPUT READING
// ============================================================================

static int8_t ReadAccelInput(void) {
  if (hi2c_accel == NULL || !gy521_ok)
    return 0;

  // Leer sensor GY521
  GY521_read(&gy521_sensor);

  // gy521_sensor.ay está en g's (normalmente -1.0 a +1.0)
  // Mapear a velocidad de movimiento
  float accel_val = gy521_sensor.ay;

  // Apply deadzone (en g's)
  float deadzone_g = (float)RACING_ACCEL_DEADZONE / (float)RACING_ACCEL_MAX;
  if (accel_val > -deadzone_g && accel_val < deadzone_g) {
    return 0;
  }

  // Map to movement speed (-SHIP_SPEED_X to +SHIP_SPEED_X)
  int32_t speed = (int32_t)(accel_val * SHIP_SPEED_X * 2.0f);

  // Clamp
  if (speed > SHIP_SPEED_X)
    speed = SHIP_SPEED_X;
  if (speed < -SHIP_SPEED_X)
    speed = -SHIP_SPEED_X;

  return (int8_t)(-speed); // INVERTIDO para coincidir con orientación
}

static int8_t ReadJoystickY(void) {
  if (hadc_joystick == NULL)
    return 0;

  // Leer ADC directamente con polling (el DMA lo maneja main.c)
  HAL_ADC_Start(hadc_joystick);
  HAL_ADC_PollForConversion(hadc_joystick, 10);
  uint32_t adc_raw = HAL_ADC_GetValue(hadc_joystick);

  // Convertir a movimiento
  int16_t adc_y = (int16_t)adc_raw - RACING_ADC_CENTER;

  // Apply deadzone
  if (adc_y > -RACING_ADC_DEADZONE && adc_y < RACING_ADC_DEADZONE) {
    return 0;
  }

  // Map to movement speed
  int32_t speed = (adc_y * SHIP_SPEED_Y) / (2048 - RACING_ADC_DEADZONE);

  // Clamp
  if (speed > SHIP_SPEED_Y)
    speed = SHIP_SPEED_Y;
  if (speed < -SHIP_SPEED_Y)
    speed = -SHIP_SPEED_Y;

  return (
      int8_t)(-speed); // Invertido para coincidir con orientación del joystick
}

// ============================================================================
// OBSTACLE MANAGEMENT
// ============================================================================

static void SpawnObstacle(void) {
  for (int i = 0; i < RACING_MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) {
      obstacles[i].active = 1;
      obstacles[i].x = 20 + (simple_rand() % (SCREEN_WIDTH - 40));
      obstacles[i].y = -20;
      obstacles[i].speed = game.speed + (simple_rand() % 3);

      // Random type
      uint32_t r = simple_rand() % 10;
      if (r < 3) {
        obstacles[i].type = RACING_OBS_ENEMY_BEE;
      } else if (r < 6) {
        obstacles[i].type = RACING_OBS_ENEMY_BUTTERFLY;
      } else if (r < 8) {
        obstacles[i].type = RACING_OBS_ASTEROID;
      } else {
        obstacles[i].type = RACING_OBS_ENEMY_BOSS;
      }

      // Schedule next spawn
      game.next_spawn_frame = game.frame_count + RACING_SPAWN_INTERVAL_MIN +
                              (simple_rand() % (RACING_SPAWN_INTERVAL_MAX -
                                                RACING_SPAWN_INTERVAL_MIN));
      return;
    }
  }
}

static void UpdateObstacles(void) {
  for (int i = 0; i < RACING_MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      EraseObstacle(&obstacles[i]);

      obstacles[i].y += obstacles[i].speed;

      // Remove if off screen
      if (obstacles[i].y > SCREEN_HEIGHT + 20) {
        obstacles[i].active = 0;
        game.score += 10; // Points for dodging
      } else {
        DrawObstacle(&obstacles[i], 0);
      }
    }
  }
}

// ============================================================================
// COLLISION DETECTION
// ============================================================================

static uint8_t CheckCollision(void) {
  for (int i = 0; i < RACING_MAX_OBSTACLES; i++) {
    if (!obstacles[i].active)
      continue;

    int16_t px = player.x, py = player.y;
    int16_t ox = obstacles[i].x, oy = obstacles[i].y;

    // AABB collision (with smaller hitbox for fairness)
    if (ox >= px - 10 && ox <= px + 10 && oy >= py - 10 && oy <= py + 12) {
      return 1;
    }
  }
  return 0;
}

// ============================================================================
// SCORE DISPLAY
// ============================================================================

static void DrawDigit(int16_t x, int16_t y, uint8_t digit, uint16_t color) {
  const uint8_t digits[10][7] = {
      {1, 1, 1, 0, 1, 1, 1}, // 0
      {0, 0, 1, 0, 0, 1, 0}, // 1
      {1, 0, 1, 1, 1, 0, 1}, // 2
      {1, 0, 1, 1, 0, 1, 1}, // 3
      {0, 1, 1, 1, 0, 1, 0}, // 4
      {1, 1, 0, 1, 0, 1, 1}, // 5
      {1, 1, 0, 1, 1, 1, 1}, // 6
      {1, 0, 1, 0, 0, 1, 0}, // 7
      {1, 1, 1, 1, 1, 1, 1}, // 8
      {1, 1, 1, 1, 0, 1, 1}  // 9
  };

  if (digit > 9)
    return;

  if (digits[digit][0])
    ILI_FillRect(x + 1, y, 3, 1, color);
  if (digits[digit][1])
    ILI_FillRect(x, y + 1, 1, 2, color);
  if (digits[digit][2])
    ILI_FillRect(x + 4, y + 1, 1, 2, color);
  if (digits[digit][3])
    ILI_FillRect(x + 1, y + 3, 3, 1, color);
  if (digits[digit][4])
    ILI_FillRect(x, y + 4, 1, 2, color);
  if (digits[digit][5])
    ILI_FillRect(x + 4, y + 4, 1, 2, color);
  if (digits[digit][6])
    ILI_FillRect(x + 1, y + 6, 3, 1, color);
}

static void DrawScore(void) {
  // Clear score area
  ILI_FillRect(SCREEN_WIDTH - 60, 10, 55, 12, COL_BLACK);

  // Draw score (5 digits)
  uint32_t s = game.score;
  for (int i = 4; i >= 0; i--) {
    DrawDigit(SCREEN_WIDTH - 12 - (4 - i) * 8, 12, s % 10, COL_WHITE);
    s /= 10;
  }
}

// ============================================================================
// GAME STATES
// ============================================================================

static void ShowGameOver(void) {
  // Flash red
  ILI_FillScreen(COL_RED);
  HAL_Delay(300);

  // Explosion effect
  for (int r = 10; r < 100; r += 15) {
    ILI_DrawCircle(player.x, player.y, r, COL_YELLOW);
    HAL_Delay(30);
  }

  HAL_Delay(500);
}

// ============================================================================
// PUBLIC FUNCTIONS
// ============================================================================

void Racing_Init(ADC_HandleTypeDef *hadc, I2C_HandleTypeDef *hi2c) {
  hadc_joystick = hadc;
  hi2c_accel = hi2c;

// Inicializar sensor GY521
#define GY521_ADDR 0x69
  GY521_Init(&gy521_sensor, hi2c_accel, GY521_ADDR);
  gy521_ok = 0;
  for (int i = 0; i < 3; i++) {
    if (GY521_begin(&gy521_sensor) == true) {
      gy521_ok = 1;
      break;
    }
  }
  ILI_FillScreen(COL_BLACK);

  // Initialize player
  player.x = SHIP_START_X;
  player.y = SHIP_START_Y;

  // Initialize obstacles
  for (int i = 0; i < RACING_MAX_OBSTACLES; i++) {
    obstacles[i].active = 0;
  }

  // Initialize game state
  game.score = 0;
  game.speed = OBSTACLE_BASE_SPEED;
  game.state = RACING_GAME_PLAYING;
  game.frame_count = 0;
  game.next_spawn_frame = 30; // First spawn after 30 frames

  // Initialize random seed
  rand_seed = HAL_GetTick();

  // Initialize stars
  InitStars();

  // Draw initial state
  DrawPlayerShip(player.x, player.y, 0);
  DrawScore();

  HAL_Delay(300);
}

void Racing_Loop(void) {
  if (game.state == RACING_GAME_OVER) {
    ShowGameOver();
    Racing_Init(hadc_joystick, hi2c_accel);
    return;
  }

  game.frame_count++;

  // Read inputs
  int8_t move_x = ReadAccelInput();
  int8_t move_y = ReadJoystickY();

  // Erase player
  ErasePlayerShip(player.x, player.y);

  // Update player position
  player.x += move_x;
  player.y += move_y;

  // Clamp to screen bounds
  if (player.x < SHIP_MIN_X)
    player.x = SHIP_MIN_X;
  if (player.x > SHIP_MAX_X)
    player.x = SHIP_MAX_X;
  if (player.y < SHIP_MIN_Y)
    player.y = SHIP_MIN_Y;
  if (player.y > SHIP_MAX_Y)
    player.y = SHIP_MAX_Y;

  // Update stars
  UpdateStars();

  // Spawn new obstacles
  if (game.frame_count >= game.next_spawn_frame) {
    SpawnObstacle();
  }

  // Update obstacles
  UpdateObstacles();

  // Check collision
  if (CheckCollision()) {
    game.state = RACING_GAME_OVER;
    if (game.score > game.hi_score) {
      game.hi_score = game.score;
    }
    return;
  }

  // Draw player
  DrawPlayerShip(player.x, player.y, 0);

  // Update score periodically
  if (game.frame_count % 5 == 0) {
    game.score++;
  }

  // Increase speed periodically
  if (game.score % RACING_SPEED_INCREMENT_SCORE == 0 && game.score > 0) {
    if (game.speed < OBSTACLE_MAX_SPEED) {
      game.speed++;
    }
  }

  // Redraw score
  if (game.frame_count % 10 == 0) {
    DrawScore();
  }

  HAL_Delay(RACING_GAME_DELAY);
}
