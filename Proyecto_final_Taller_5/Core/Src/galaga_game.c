#include "galaga_game.h"
#include "TFT_ILI9486.h"

// ============================================================================
// GAME STATE VARIABLES
// ============================================================================
static PlayerShip player;
static Bullet bullets[MAX_BULLETS];
static Enemy enemies[MAX_ENEMIES];
static GameState game;

// Control manual (acelerómetro/joystick)
static uint8_t manual_control = 1; // 1 = control manual, 0 = auto-play
static int16_t target_player_x =
    SCREEN_WIDTH / 2;              // Posición objetivo del jugador
static uint8_t fire_requested = 0; // Bandera de disparo

// ============================================================================
// SHIP SPRITE DATA (24x20 pixels, Galaga classic style)
// Each row is a bitmask for the ship shape
// We'll draw it procedurally for efficiency
// ============================================================================

// Draw the player ship at position (x, y) with specified color scheme
// Based on the classic Galaga fighter ship from the reference image
static void DrawPlayerShip(int16_t cx, int16_t cy, uint8_t erase) {
  uint16_t white = erase ? COL_BLACK : COL_WHITE;
  uint16_t red = erase ? COL_BLACK : COL_RED;
  uint16_t blue = erase ? COL_BLACK : COL_BLUE;

  // Central tower (top spike)
  ILI_FillRect(cx - 1, cy - 10, 3, 4, white); // Top spike

  // Main body top section
  ILI_FillRect(cx - 3, cy - 6, 7, 3, white); // Upper body

  // Cockpit (red center)
  ILI_FillRect(cx - 1, cy - 4, 3, 2, red); // Cockpit

  // Main body middle
  ILI_FillRect(cx - 5, cy - 3, 11, 4, white); // Wide body

  // Blue accents on body
  ILI_FillRect(cx - 4, cy - 2, 2, 2, blue); // Left blue
  ILI_FillRect(cx + 3, cy - 2, 2, 2, blue); // Right blue

  // Wings extending outward
  ILI_FillRect(cx - 9, cy, 4, 5, white); // Left wing base
  ILI_FillRect(cx + 6, cy, 4, 5, white); // Right wing base

  // Wing tips (outer tall sections)
  ILI_FillRect(cx - 11, cy - 4, 3, 9, white); // Left wing tall
  ILI_FillRect(cx + 9, cy - 4, 3, 9, white);  // Right wing tall

  // Red accents on wings
  ILI_FillRect(cx - 10, cy - 2, 2, 4, red); // Left wing red
  ILI_FillRect(cx + 9, cy - 2, 2, 4, red);  // Right wing red

  // Lower body sections
  ILI_FillRect(cx - 7, cy + 2, 3, 4, white); // Left lower
  ILI_FillRect(cx + 5, cy + 2, 3, 4, white); // Right lower

  // Red accents lower
  ILI_FillRect(cx - 6, cy + 3, 2, 2, red); // Left lower red
  ILI_FillRect(cx + 5, cy + 3, 2, 2, red); // Right lower red

  // Central lower section
  ILI_FillRect(cx - 3, cy + 1, 7, 5, white); // Center bottom

  // Bottom red accent
  ILI_FillRect(cx - 2, cy + 4, 5, 2, red); // Bottom red
}

// Fast erase using bounding box
static void ErasePlayerShip(int16_t cx, int16_t cy) {
  ILI_FillRect(cx - 12, cy - 11, 25, 18, COL_BLACK);
}

// ============================================================================
// ENEMY SPRITES
// ============================================================================

static void DrawEnemy(Enemy *e, uint8_t erase) {
  uint16_t color1, color2;

  if (erase) {
    color1 = COL_BLACK;
    color2 = COL_BLACK;
  } else {
    switch (e->type) {
    case ENEMY_TYPE_BEE:
      color1 = COL_YELLOW;
      color2 = COL_ORANGE;
      break;
    case ENEMY_TYPE_BUTTERFLY:
      color1 = COL_BLUE;
      color2 = COL_CYAN;
      break;
    case ENEMY_TYPE_BOSS:
    default:
      color1 = COL_RED;
      color2 = COL_MAGENTA;
      break;
    }
  }

  // Simple enemy shape (invader-style)
  int16_t x = e->x;
  int16_t y = e->y;

  // Body
  ILI_FillRect(x - 4, y - 3, 9, 6, color1);

  // Wings/antennae
  ILI_FillRect(x - 7, y - 1, 3, 4, color2);
  ILI_FillRect(x + 5, y - 1, 3, 4, color2);

  // Eyes (small dots)
  if (!erase) {
    ILI_FillRect(x - 2, y - 2, 2, 2, COL_WHITE);
    ILI_FillRect(x + 1, y - 2, 2, 2, COL_WHITE);
  }

  // Top details
  ILI_FillRect(x - 2, y - 5, 2, 2, color2);
  ILI_FillRect(x + 1, y - 5, 2, 2, color2);
}

static void EraseEnemy(Enemy *e) {
  ILI_FillRect(e->x - 8, e->y - 6, 17, 13, COL_BLACK);
}

// ============================================================================
// BULLET FUNCTIONS
// ============================================================================

static void DrawBullet(Bullet *b, uint8_t erase) {
  uint16_t color = erase ? COL_BLACK : COL_CYAN;
  ILI_FillRect(b->x - 1, b->y - 4, BULLET_WIDTH, BULLET_HEIGHT, color);
}

static void FireBullet(void) {
  uint32_t now = HAL_GetTick();
  if (now - game.last_shot_time < SHOOT_COOLDOWN)
    return;

  // Find inactive bullet slot
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].x = player.x;
      bullets[i].y = player.y - 12;
      bullets[i].active = 1;
      game.last_shot_time = now;
      DrawBullet(&bullets[i], 0);
      return;
    }
  }
}

static void UpdateBullets(void) {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      // Erase old position
      DrawBullet(&bullets[i], 1);

      // Update position
      bullets[i].y -= BULLET_SPEED;

      // Check if off screen
      if (bullets[i].y < 0) {
        bullets[i].active = 0;
      } else {
        // Draw new position
        DrawBullet(&bullets[i], 0);
      }
    }
  }
}

// ============================================================================
// COLLISION DETECTION
// ============================================================================

static uint8_t CheckBulletEnemyCollision(Bullet *b, Enemy *e) {
  if (!b->active || !e->active)
    return 0;

  // Simple AABB
  int16_t bx = b->x, by = b->y;
  int16_t ex = e->x, ey = e->y;

  if (bx >= ex - 6 && bx <= ex + 6 && by >= ey - 5 && by <= ey + 5) {
    return 1;
  }
  return 0;
}

static uint8_t CheckPlayerEnemyCollision(Enemy *e) {
  if (!e->active)
    return 0;

  int16_t px = player.x, py = player.y;
  int16_t ex = e->x, ey = e->y;

  // Player ship is about 24 wide, 20 tall
  // Enemy is about 16 wide, 12 tall
  if (ex >= px - 15 && ex <= px + 15 && ey >= py - 15 && ey <= py + 15) {
    return 1;
  }
  return 0;
}

static void ProcessCollisions(void) {
  for (int e = 0; e < MAX_ENEMIES; e++) {
    if (!enemies[e].active)
      continue;

    // Check bullet collisions
    for (int b = 0; b < MAX_BULLETS; b++) {
      if (CheckBulletEnemyCollision(&bullets[b], &enemies[e])) {
        // Destroy enemy
        EraseEnemy(&enemies[e]);
        enemies[e].active = 0;

        // Deactivate bullet
        DrawBullet(&bullets[b], 1);
        bullets[b].active = 0;

        // Score based on enemy type
        switch (enemies[e].type) {
        case ENEMY_TYPE_BEE:
          game.score += 50;
          break;
        case ENEMY_TYPE_BUTTERFLY:
          game.score += 80;
          break;
        case ENEMY_TYPE_BOSS:
          game.score += 150;
          break;
        }

        game.enemies_alive--;
        break;
      }
    }

    // Check player collision
    if (CheckPlayerEnemyCollision(&enemies[e])) {
      game.state = GAME_STATE_GAMEOVER;
      return;
    }
  }

  // Check win condition
  if (game.enemies_alive == 0) {
    game.state = GAME_STATE_WIN;
  }
}

// ============================================================================
// ENEMY MOVEMENT
// ============================================================================

static void UpdateEnemies(void) {
  static uint8_t move_counter = 0;
  static uint8_t should_drop = 0;

  move_counter++;
  if (move_counter < 3)
    return; // Slow down enemy movement
  move_counter = 0;

  // Check if any enemy hits the edge
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      if ((enemies[i].x + ENEMY_WIDTH / 2 >= SCREEN_WIDTH - 10 &&
           game.formation_dx > 0) ||
          (enemies[i].x - ENEMY_WIDTH / 2 <= 10 && game.formation_dx < 0)) {
        should_drop = 1;
        game.formation_dx = -game.formation_dx;
        break;
      }
    }
  }

  // Move all enemies
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      DrawEnemy(&enemies[i], 1); // Erase

      enemies[i].x += game.formation_dx * ENEMY_SPEED_X;

      if (should_drop) {
        enemies[i].y += ENEMY_DROP_DIST;

        // Check if enemies reached player level
        if (enemies[i].y > SHIP_Y - 30) {
          game.state = GAME_STATE_GAMEOVER;
        }
      }

      DrawEnemy(&enemies[i], 0); // Redraw
    }
  }

  should_drop = 0;
}

// ============================================================================
// PLAYER MOVEMENT (Auto-play)
// ============================================================================

static void UpdatePlayer(void) {
  static uint8_t shoot_counter = 0;

  // Erase old position
  ErasePlayerShip(player.x, player.y);

  if (manual_control) {
    // CONTROL MANUAL: Mover hacia posición objetivo (suavizado)
    int16_t diff = target_player_x - player.x;
    if (diff > SHIP_SPEED) {
      player.x += SHIP_SPEED;
    } else if (diff < -SHIP_SPEED) {
      player.x -= SHIP_SPEED;
    } else {
      player.x = target_player_x;
    }

    // Límites de pantalla
    if (player.x < 15)
      player.x = 15;
    if (player.x > SCREEN_WIDTH - 15)
      player.x = SCREEN_WIDTH - 15;

    // Disparar si fue solicitado
    if (fire_requested) {
      FireBullet();
      fire_requested = 0;
    }
  } else {
    // AUTO-PLAY: Movimiento automático (bouncing)
    player.x += player.dx * SHIP_SPEED;

    // Bounce off edges
    if (player.x >= SCREEN_WIDTH - 15) {
      player.x = SCREEN_WIDTH - 15;
      player.dx = -1;
    } else if (player.x <= 15) {
      player.x = 15;
      player.dx = 1;
    }

    // Auto-fire
    shoot_counter++;
    if (shoot_counter >= 8) {
      FireBullet();
      shoot_counter = 0;
    }
  }

  // Draw new position
  DrawPlayerShip(player.x, player.y, 0);
}

// ============================================================================
// GAME STATE MANAGEMENT
// ============================================================================

static void DrawScore(void) {
  // Draw score as simple bar at top (visual representation)
  // Score bar: max width 200, grows with score
  uint16_t bar_width = (game.score > 2000) ? 200 : (game.score / 10);
  ILI_FillRect(10, 5, 200, 8, COL_DARKBLUE);
  if (bar_width > 0) {
    ILI_FillRect(10, 5, bar_width, 8, COL_GREEN);
  }
}

static void ShowGameOver(void) {
  // Flash red
  ILI_FillScreen(COL_RED);
  HAL_Delay(500);

  // Show "explosion" effect
  for (int r = 10; r < 100; r += 10) {
    ILI_DrawCircle(player.x, player.y, r, COL_YELLOW);
    HAL_Delay(30);
  }
  HAL_Delay(500);
}

static void ShowVictory(void) {
  // Flash green
  ILI_FillScreen(COL_GREEN);
  HAL_Delay(500);

  // Victory circles
  for (int r = 20; r < 150; r += 20) {
    ILI_DrawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, r, COL_YELLOW);
    HAL_Delay(50);
  }
  HAL_Delay(1000);
}

static void InitFormation(void) {
  int idx = 0;

  // Row 1: Boss enemies (top, 4 enemies)
  for (int i = 0; i < 4; i++) {
    enemies[idx].x = FORMATION_START_X + 60 + i * (ENEMY_SPACING_X + 10);
    enemies[idx].y = FORMATION_START_Y;
    enemies[idx].type = ENEMY_TYPE_BOSS;
    enemies[idx].active = 1;
    idx++;
  }

  // Row 2: Butterfly enemies (8 enemies)
  for (int i = 0; i < 8; i++) {
    enemies[idx].x = FORMATION_START_X + i * ENEMY_SPACING_X;
    enemies[idx].y = FORMATION_START_Y + ENEMY_SPACING_Y;
    enemies[idx].type = ENEMY_TYPE_BUTTERFLY;
    enemies[idx].active = 1;
    idx++;
  }

  // Row 3: Bee enemies (8 enemies)
  for (int i = 0; i < 8; i++) {
    enemies[idx].x = FORMATION_START_X + i * ENEMY_SPACING_X;
    enemies[idx].y = FORMATION_START_Y + 2 * ENEMY_SPACING_Y;
    enemies[idx].type = ENEMY_TYPE_BEE;
    enemies[idx].active = 1;
    idx++;
  }

  // Row 4: More bees (4 enemies) - fill remaining slots
  for (int i = 0; i < 4 && idx < MAX_ENEMIES; i++) {
    enemies[idx].x = FORMATION_START_X + 60 + i * (ENEMY_SPACING_X + 10);
    enemies[idx].y = FORMATION_START_Y + 3 * ENEMY_SPACING_Y;
    enemies[idx].type = ENEMY_TYPE_BEE;
    enemies[idx].active = 1;
    idx++;
  }

  game.enemies_alive = idx;
}

// ============================================================================
// PUBLIC FUNCTIONS
// ============================================================================

void Galaga_Init(void) {
  // Clear screen
  ILI_FillScreen(COL_BLACK);

  // Initialize player
  player.x = SCREEN_WIDTH / 2;
  player.y = SHIP_Y;
  player.dx = 1; // Start moving right

  // Initialize bullets
  for (int i = 0; i < MAX_BULLETS; i++) {
    bullets[i].active = 0;
  }

  // Initialize game state
  game.score = 0;
  game.state = GAME_STATE_PLAYING;
  game.last_shot_time = 0;
  game.formation_dx = 1; // Enemies start moving right

  // Create enemy formation
  InitFormation();

  // Draw initial state
  DrawPlayerShip(player.x, player.y, 0);

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      DrawEnemy(&enemies[i], 0);
    }
  }

  // Draw score bar background
  DrawScore();

  // Small delay to see initial state
  HAL_Delay(500);
}

void Galaga_Loop(void) {
  if (game.state != GAME_STATE_PLAYING) {
    // Handle end states
    if (game.state == GAME_STATE_GAMEOVER) {
      ShowGameOver();
    } else if (game.state == GAME_STATE_WIN) {
      ShowVictory();
    }

    // Reset game
    HAL_Delay(500);
    Galaga_Init();
    return;
  }

  // Game loop
  UpdatePlayer();
  UpdateBullets();
  UpdateEnemies();
  ProcessCollisions();
  DrawScore();

  // Frame delay
  HAL_Delay(GAME_DELAY);
}

// ============================================================================
// PUBLIC CONTROL FUNCTIONS
// ============================================================================

void Galaga_SetPlayerX(int16_t x) {
  // Clamp to valid range
  if (x < 15)
    x = 15;
  if (x > SCREEN_WIDTH - 15)
    x = SCREEN_WIDTH - 15;
  target_player_x = x;
}

void Galaga_Fire(void) { fire_requested = 1; }

uint8_t Galaga_IsActive(void) {
  return (game.state == GAME_STATE_PLAYING) ? 1 : 0;
}

void Galaga_SetManualControl(uint8_t enable) { manual_control = enable; }
