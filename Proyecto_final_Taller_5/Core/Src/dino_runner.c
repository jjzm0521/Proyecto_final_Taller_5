/**
 * @file dino_runner.c
 * @brief T-Rex Runner Game Implementation
 */

#include "dino_runner.h"
#include "TFT_ILI9486.h"

// ============================================================================
// GAME STATE VARIABLES
// ============================================================================
static Dino dino;
static DinoObstacle obstacles[DINO_MAX_OBSTACLES];
static DinoGame game;
static ADC_HandleTypeDef *hadc_joystick = NULL;

// Simple random seed
static uint32_t rand_seed = 12345;

static uint32_t simple_rand(void) {
  rand_seed = rand_seed * 1103515245 + 12345;
  return (rand_seed >> 16) & 0x7FFF;
}

// ============================================================================
// ADC INPUT (DMA Buffer)
// ============================================================================

static JoystickInput ReadJoystick(void) {
  if (hadc_joystick == NULL)
    return INPUT_NONE;

  // Leer ADC directamente con polling
  HAL_ADC_Start(hadc_joystick);
  HAL_ADC_PollForConversion(hadc_joystick, 10);
  uint16_t adc_y = HAL_ADC_GetValue(hadc_joystick);

  // Check joystick direction
  if (adc_y > DINO_ADC_UP_THRESHOLD) {
    return INPUT_UP;
  } else if (adc_y < DINO_ADC_DOWN_THRESHOLD) {
    return INPUT_DOWN;
  }
  return INPUT_NONE;
}

// ============================================================================
// T-REX SPRITES
// ============================================================================

static void DrawDino(uint8_t erase) {
  uint16_t c = erase ? DINO_COL_BG : DINO_COL_DINO;
  int16_t x = dino.x;
  int16_t y = dino.y;

  // T-Rex based on Chrome dino reference (~24x26px)
  // Head
  ILI_FillRect(x + 10, y, 14, 4, c);    // Top of head
  ILI_FillRect(x + 8, y + 4, 16, 8, c); // Main head
  ILI_FillRect(x + 18, y + 6, 6, 2, c); // Snout extension

  // Eye (white dot when not erasing)
  if (!erase) {
    ILI_FillRect(x + 18, y + 4, 2, 2, DINO_COL_BG);
  }

  // Neck
  ILI_FillRect(x + 8, y + 12, 8, 4, c);

  // Body
  ILI_FillRect(x + 2, y + 14, 16, 8, c);
  ILI_FillRect(x, y + 16, 4, 6, c); // Tail

  // Arms (small)
  ILI_FillRect(x + 14, y + 14, 4, 4, c);

  // Legs with animation
  if (dino.state == DINO_STATE_RUN) {
    if (dino.anim_frame == 0) {
      // Frame 0: Left leg forward
      ILI_FillRect(x + 4, y + 22, 4, 6, c);  // Left leg
      ILI_FillRect(x + 10, y + 22, 4, 4, c); // Right leg back
    } else {
      // Frame 1: Right leg forward
      ILI_FillRect(x + 4, y + 22, 4, 4, c);  // Left leg back
      ILI_FillRect(x + 10, y + 22, 4, 6, c); // Right leg
    }
  } else if (dino.state == DINO_STATE_JUMP) {
    // Both legs together when jumping
    ILI_FillRect(x + 4, y + 22, 4, 4, c);
    ILI_FillRect(x + 10, y + 22, 4, 4, c);
  }
}

static void DrawDinoDuck(uint8_t erase) {
  uint16_t c = erase ? DINO_COL_BG : DINO_COL_DINO;
  int16_t x = dino.x;
  int16_t y = GROUND_Y - DINO_DUCK_HEIGHT;

  // Ducking T-Rex (wider, shorter)
  // Horizontal body
  ILI_FillRect(x, y + 4, 28, 8, c);

  // Head
  ILI_FillRect(x + 20, y, 12, 8, c);

  // Eye
  if (!erase) {
    ILI_FillRect(x + 26, y + 2, 2, 2, DINO_COL_BG);
  }

  // Tail
  ILI_FillRect(x - 4, y + 6, 6, 4, c);

  // Legs with animation
  if (dino.anim_frame == 0) {
    ILI_FillRect(x + 6, y + 12, 4, 4, c);
    ILI_FillRect(x + 14, y + 12, 4, 2, c);
  } else {
    ILI_FillRect(x + 6, y + 12, 4, 2, c);
    ILI_FillRect(x + 14, y + 12, 4, 4, c);
  }
}

static void EraseDino(void) {
  if (dino.state == DINO_STATE_DUCK) {
    ILI_FillRect(dino.x - 5, GROUND_Y - DINO_DUCK_HEIGHT - 2,
                 DINO_DUCK_WIDTH + 10, DINO_DUCK_HEIGHT + 4, DINO_COL_BG);
  } else {
    ILI_FillRect(dino.x - 2, dino.y - 2, DINO_WIDTH + 4, DINO_HEIGHT + 8,
                 DINO_COL_BG);
  }
}

// ============================================================================
// OBSTACLES
// ============================================================================

static void DrawCactus(DinoObstacle *obs, uint8_t erase) {
  uint16_t c = erase ? DINO_COL_BG : DINO_COL_OBSTACLE;
  int16_t x = obs->x;
  int16_t y = obs->y;

  if (obs->type == DINO_OBS_CACTUS_SMALL) {
    // Small cactus
    ILI_FillRect(x + 4, y, 4, 24, c);     // Main stem
    ILI_FillRect(x, y + 8, 4, 8, c);      // Left arm
    ILI_FillRect(x, y + 4, 4, 4, c);      // Left arm top
    ILI_FillRect(x + 8, y + 12, 4, 8, c); // Right arm
    ILI_FillRect(x + 8, y + 8, 4, 4, c);  // Right arm top
  } else {
    // Large cactus
    ILI_FillRect(x + 6, y, 6, 36, c);       // Main stem
    ILI_FillRect(x, y + 10, 6, 12, c);      // Left arm
    ILI_FillRect(x, y + 6, 4, 4, c);        // Left arm top
    ILI_FillRect(x + 12, y + 16, 6, 12, c); // Right arm
    ILI_FillRect(x + 12, y + 12, 4, 4, c);  // Right arm top
  }
}

static void DrawBird(DinoObstacle *obs, uint8_t erase) {
  uint16_t c = erase ? DINO_COL_BG : DINO_COL_OBSTACLE;
  int16_t x = obs->x;
  int16_t y = obs->y;

  // Body
  ILI_FillRect(x + 4, y + 6, 16, 6, c);

  // Head/beak
  ILI_FillRect(x + 18, y + 6, 8, 4, c);

  // Eye
  if (!erase) {
    ILI_FillRect(x + 20, y + 6, 2, 2, DINO_COL_BG);
  }

  // Tail
  ILI_FillRect(x, y + 8, 6, 4, c);

  // Wings with animation
  if (obs->anim_frame == 0) {
    ILI_FillRect(x + 8, y, 8, 6, c); // Wing up
  } else {
    ILI_FillRect(x + 8, y + 12, 8, 4, c); // Wing down
  }
}

static void EraseObstacle(DinoObstacle *obs) {
  int16_t w, h;
  switch (obs->type) {
  case DINO_OBS_CACTUS_SMALL:
    w = CACTUS_SMALL_W + 4;
    h = CACTUS_SMALL_H + 4;
    break;
  case DINO_OBS_CACTUS_LARGE:
    w = CACTUS_LARGE_W + 8;
    h = CACTUS_LARGE_H + 4;
    break;
  default: // Birds
    w = BIRD_WIDTH + 4;
    h = BIRD_HEIGHT + 4;
    break;
  }
  ILI_FillRect(obs->x - 2, obs->y - 2, w, h, DINO_COL_BG);
}

static void SpawnObstacle(void) {
  for (int i = 0; i < DINO_MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) {
      obstacles[i].active = 1;
      obstacles[i].x = SCREEN_WIDTH + 20;
      obstacles[i].anim_frame = 0;

      // Random type
      uint32_t r = simple_rand() % 10;
      if (r < 4) {
        obstacles[i].type = DINO_OBS_CACTUS_SMALL;
        obstacles[i].y = GROUND_Y - CACTUS_SMALL_H;
      } else if (r < 7) {
        obstacles[i].type = DINO_OBS_CACTUS_LARGE;
        obstacles[i].y = GROUND_Y - CACTUS_LARGE_H;
      } else if (r < 9) {
        obstacles[i].type = DINO_OBS_BIRD_LOW;
        obstacles[i].y = GROUND_Y - 30; // Low bird (need to duck)
      } else {
        obstacles[i].type = DINO_OBS_BIRD_HIGH;
        obstacles[i].y = GROUND_Y - 60; // High bird (can run under)
      }

      game.last_obstacle_x = obstacles[i].x;
      return;
    }
  }
}

// ============================================================================
// GROUND
// ============================================================================

static void DrawGround(void) {
  ILI_FillRect(0, GROUND_Y, SCREEN_WIDTH, GROUND_THICKNESS, DINO_COL_GROUND);

  // Simple texture dots
  for (int x = 0; x < SCREEN_WIDTH; x += 20) {
    ILI_FillRect(x + (game.frame_count % 20), GROUND_Y + 3, 2, 1,
                 DINO_COL_GROUND);
  }
}

// ============================================================================
// SCORE DISPLAY
// ============================================================================

// Simple digit drawing (5x7 pixels each)
static void DrawDigit(int16_t x, int16_t y, uint8_t digit, uint16_t color) {
  // Simplified 7-segment style digits
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

  // Top horizontal
  if (digits[digit][0])
    ILI_FillRect(x + 1, y, 3, 1, color);
  // Top-left vertical
  if (digits[digit][1])
    ILI_FillRect(x, y + 1, 1, 2, color);
  // Top-right vertical
  if (digits[digit][2])
    ILI_FillRect(x + 4, y + 1, 1, 2, color);
  // Middle horizontal
  if (digits[digit][3])
    ILI_FillRect(x + 1, y + 3, 3, 1, color);
  // Bottom-left vertical
  if (digits[digit][4])
    ILI_FillRect(x, y + 4, 1, 2, color);
  // Bottom-right vertical
  if (digits[digit][5])
    ILI_FillRect(x + 4, y + 4, 1, 2, color);
  // Bottom horizontal
  if (digits[digit][6])
    ILI_FillRect(x + 1, y + 6, 3, 1, color);
}

static void DrawScore(void) {
  // Clear score area
  ILI_FillRect(SCREEN_WIDTH - 60, 10, 55, 12, DINO_COL_BG);

  // Draw score (5 digits)
  uint32_t s = game.score;
  for (int i = 4; i >= 0; i--) {
    DrawDigit(SCREEN_WIDTH - 12 - (4 - i) * 8, 12, s % 10, DINO_COL_TEXT);
    s /= 10;
  }
}

// ============================================================================
// PHYSICS & COLLISION
// ============================================================================

static void UpdateDino(JoystickInput input) {
  // Handle input
  if (dino.on_ground) {
    if (input == INPUT_UP && dino.state != DINO_STATE_JUMP) {
      // Jump
      dino.state = DINO_STATE_JUMP;
      dino.vy = JUMP_VELOCITY;
      dino.on_ground = 0;
    } else if (input == INPUT_DOWN) {
      // Duck
      if (dino.state != DINO_STATE_DUCK) {
        EraseDino();
        dino.state = DINO_STATE_DUCK;
      }
    } else if (dino.state == DINO_STATE_DUCK && input != INPUT_DOWN) {
      // Stop ducking
      EraseDino();
      dino.state = DINO_STATE_RUN;
      dino.y = GROUND_Y - DINO_HEIGHT;
    }
  }

  // Apply gravity
  if (!dino.on_ground) {
    dino.vy += GRAVITY;
    if (dino.vy > MAX_FALL_SPEED)
      dino.vy = MAX_FALL_SPEED;
    dino.y += dino.vy;

    // Land on ground
    if (dino.y >= GROUND_Y - DINO_HEIGHT) {
      dino.y = GROUND_Y - DINO_HEIGHT;
      dino.vy = 0;
      dino.on_ground = 1;
      dino.state = DINO_STATE_RUN;
    }
  }

  // Animation frame toggle
  if (game.frame_count % 4 == 0) {
    dino.anim_frame = !dino.anim_frame;
  }
}

static uint8_t CheckCollision(void) {
  int16_t dx, dy, dw, dh;

  // Dino hitbox depends on state
  if (dino.state == DINO_STATE_DUCK) {
    dx = dino.x + 4;
    dy = GROUND_Y - DINO_DUCK_HEIGHT + 2;
    dw = DINO_DUCK_WIDTH - 8;
    dh = DINO_DUCK_HEIGHT - 4;
  } else {
    dx = dino.x + 4;
    dy = dino.y + 4;
    dw = DINO_WIDTH - 8;
    dh = DINO_HEIGHT - 4;
  }

  for (int i = 0; i < DINO_MAX_OBSTACLES; i++) {
    if (!obstacles[i].active)
      continue;

    int16_t ox = obstacles[i].x;
    int16_t oy = obstacles[i].y;
    int16_t ow, oh;

    switch (obstacles[i].type) {
    case DINO_OBS_CACTUS_SMALL:
      ox += 2;
      ow = CACTUS_SMALL_W - 4;
      oh = CACTUS_SMALL_H - 4;
      break;
    case DINO_OBS_CACTUS_LARGE:
      ox += 4;
      ow = CACTUS_LARGE_W - 4;
      oh = CACTUS_LARGE_H - 4;
      break;
    default: // Birds
      ox += 4;
      ow = BIRD_WIDTH - 8;
      oh = BIRD_HEIGHT - 4;
      break;
    }

    // AABB collision
    if (dx < ox + ow && dx + dw > ox && dy < oy + oh && dy + dh > oy) {
      return 1;
    }
  }
  return 0;
}

// ============================================================================
// GAME STATE
// ============================================================================

static void ShowGameOver(void) {
  // Flash red briefly
  ILI_FillScreen(TFT_RED);
  HAL_Delay(200);

  // Show score
  ILI_FillScreen(DINO_COL_BG);

  // Draw "GAME OVER" as simple rectangles
  ILI_FillRect(SCREEN_WIDTH / 2 - 40, SCREEN_HEIGHT / 2 - 20, 80, 4,
               DINO_COL_DINO);
  ILI_FillRect(SCREEN_WIDTH / 2 - 40, SCREEN_HEIGHT / 2, 80, 4, DINO_COL_DINO);
  ILI_FillRect(SCREEN_WIDTH / 2 - 40, SCREEN_HEIGHT / 2 + 20, 80, 4,
               DINO_COL_DINO);

  HAL_Delay(1500);
}

void Dino_Init(ADC_HandleTypeDef *hadc) {
  hadc_joystick = hadc;

  // Clear screen with game background
  ILI_FillScreen(DINO_COL_BG);

  // Initialize dino
  dino.x = DINO_START_X;
  dino.y = GROUND_Y - DINO_HEIGHT;
  dino.vy = 0;
  dino.state = DINO_STATE_RUN;
  dino.anim_frame = 0;
  dino.on_ground = 1;

  // Initialize obstacles
  for (int i = 0; i < DINO_MAX_OBSTACLES; i++) {
    obstacles[i].active = 0;
  }

  // Initialize game state
  game.score = 0;
  game.speed = INITIAL_SPEED;
  game.state = DINO_GAME_PLAYING;
  game.last_obstacle_x = SCREEN_WIDTH + 100;
  game.frame_count = 0;

  // Initialize random seed with system tick
  rand_seed = HAL_GetTick();

  // Draw initial state
  DrawGround();
  DrawDino(0);
  DrawScore();

  HAL_Delay(500);
}

void Dino_Loop(void) {
  if (game.state == DINO_GAME_OVER) {
    ShowGameOver();
    Dino_Init(hadc_joystick);
    return;
  }

  game.frame_count++;

  // Read input
  JoystickInput input = ReadJoystick();

  // Erase moving objects
  EraseDino();
  for (int i = 0; i < DINO_MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      EraseObstacle(&obstacles[i]);
    }
  }

  // Update dino
  UpdateDino(input);

  // Update obstacles
  for (int i = 0; i < DINO_MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      obstacles[i].x -= game.speed;

      // Animate birds
      if ((obstacles[i].type == DINO_OBS_BIRD_LOW ||
           obstacles[i].type == DINO_OBS_BIRD_HIGH) &&
          game.frame_count % 6 == 0) {
        obstacles[i].anim_frame = !obstacles[i].anim_frame;
      }

      // Remove if off screen
      if (obstacles[i].x < -30) {
        obstacles[i].active = 0;
      }
    }
  }

  // Spawn new obstacles
  uint32_t min_gap = DINO_OBSTACLE_SPAWN_MIN + (MAX_SPEED - game.speed) * 5;
  if (game.last_obstacle_x < SCREEN_WIDTH - min_gap) {
    if ((simple_rand() % 100) < 30) { // 30% chance each frame
      SpawnObstacle();
    }
  }
  game.last_obstacle_x -= game.speed;

  // Check collision
  if (CheckCollision()) {
    game.state = DINO_GAME_OVER;
    if (game.score > game.hi_score) {
      game.hi_score = game.score;
    }
    return;
  }

  // Draw updated state
  if (dino.state == DINO_STATE_DUCK) {
    DrawDinoDuck(0);
  } else {
    DrawDino(0);
  }

  for (int i = 0; i < DINO_MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      if (obstacles[i].type == DINO_OBS_CACTUS_SMALL ||
          obstacles[i].type == DINO_OBS_CACTUS_LARGE) {
        DrawCactus(&obstacles[i], 0);
      } else {
        DrawBird(&obstacles[i], 0);
      }
    }
  }

  // Update score every few frames
  if (game.frame_count % 3 == 0) {
    game.score++;

    // Increase speed periodically
    if (game.score % DINO_SPEED_INCREMENT_SCORE == 0 &&
        game.speed < MAX_SPEED) {
      game.speed++;
    }
  }

  // Redraw score periodically
  if (game.frame_count % 10 == 0) {
    DrawScore();
  }

  // Repair ground line
  DrawGround();

  HAL_Delay(DINO_GAME_DELAY);
}

void Dino_Jump(void) {
  if (dino.on_ground && dino.state != DINO_STATE_JUMP) {
    EraseDino();
    dino.state = DINO_STATE_JUMP;
    dino.vy = JUMP_VELOCITY;
    dino.on_ground = 0;
  }
}

void Dino_Duck(uint8_t duck) {
  if (dino.on_ground) {
    if (duck && dino.state != DINO_STATE_DUCK) {
      EraseDino();
      dino.state = DINO_STATE_DUCK;
    } else if (!duck && dino.state == DINO_STATE_DUCK) {
      EraseDino();
      dino.state = DINO_STATE_RUN;
      dino.y = GROUND_Y - DINO_HEIGHT;
    }
  }
}
