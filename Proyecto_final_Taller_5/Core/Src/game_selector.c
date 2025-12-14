/**
 * @file game_selector.c
 * @brief Implementación del Selector de Juegos
 */

#include "game_selector.h"
#include "dino_runner.h"
#include "galaga_game.h"
#include "racing_game.h"
#include <stdio.h>

// ============================================================================
// VARIABLES DE ESTADO
// ============================================================================

static GameType juego_actual = GAME_RACING;
static GameType juego_previo = GAME_RACING;
static uint8_t necesita_init = 1;

// Handles de periféricos
static ADC_HandleTypeDef *hadc_ptr = NULL;
static I2C_HandleTypeDef *hi2c_ptr = NULL;
static UART_HandleTypeDef *huart_ptr = NULL;

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

static void IniciarJuegoActual(void) {
  printf("Iniciando juego: ");

  switch (juego_actual) {
  case GAME_RACING:
    printf("RACING GAME\r\n");
    Racing_Init(hadc_ptr, hi2c_ptr);
    break;

  case GAME_DINO:
    printf("DINO RUNNER\r\n");
    Dino_Init(hadc_ptr);
    break;

  case GAME_GALAGA:
    printf("GALAGA\r\n");
    Galaga_SetManualControl(1);
    Galaga_Init();
    break;
  }

  necesita_init = 0;
}

// ============================================================================
// FUNCIONES PÚBLICAS
// ============================================================================

void GameSelector_Init(ADC_HandleTypeDef *hadc, I2C_HandleTypeDef *hi2c,
                       UART_HandleTypeDef *huart, GameType juego_inicial) {
  hadc_ptr = hadc;
  hi2c_ptr = hi2c;
  huart_ptr = huart;

  juego_actual = juego_inicial;
  juego_previo = juego_inicial;
  necesita_init = 1;

  printf("=== SELECTOR DE JUEGOS ===\r\n");
  printf("Comandos: R=Racing, D=Dino, G=Galaga\r\n");

  // Iniciar el juego inicial
  IniciarJuegoActual();
}

void GameSelector_Loop(void) {
  // Si cambió el juego, iniciar el nuevo
  if (necesita_init) {
    IniciarJuegoActual();
  }

  // Ejecutar loop del juego actual
  switch (juego_actual) {
  case GAME_RACING:
    Racing_Loop();
    break;

  case GAME_DINO:
    Dino_Loop();
    break;

  case GAME_GALAGA:
    Galaga_Loop();
    break;
  }
}

void GameSelector_SetGame(GameType nuevo_juego) {
  if (nuevo_juego != juego_actual) {
    juego_previo = juego_actual;
    juego_actual = nuevo_juego;
    necesita_init = 1;

    printf("Cambiando juego...\r\n");
  }
}

void GameSelector_ProcessUART(uint8_t caracter) {
  switch (caracter) {
  case 'R':
  case 'r':
    GameSelector_SetGame(GAME_RACING);
    break;

  case 'D':
  case 'd':
    GameSelector_SetGame(GAME_DINO);
    break;

  case 'G':
  case 'g':
    GameSelector_SetGame(GAME_GALAGA);
    break;

  default:
    // Ignorar otros caracteres
    break;
  }
}

GameType GameSelector_GetCurrentGame(void) { return juego_actual; }
