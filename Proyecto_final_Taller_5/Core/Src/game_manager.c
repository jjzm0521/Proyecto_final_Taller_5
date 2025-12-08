/**
 * @file game_manager.c
 * @brief Game Manager - Implementación
 */

#include "game_manager.h"
#include "STM32_GY521.h"
#include "TFT_ILI9486.h"
#include "galaga_game.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// VARIABLES PRIVADAS
// ============================================================================

static GameType current_game = GAME_NONE;
static UART_HandleTypeDef *uart_handle = NULL;
static ADC_HandleTypeDef *adc_handle = NULL;
static I2C_HandleTypeDef *i2c_handle = NULL;

// Buffer para recepción UART
static uint8_t uart_rx_byte = 0;

// Sensor GY521
static GY521 accel_sensor;
static uint8_t sensor_ok = 0;

// ADC con DMA
static volatile uint32_t adc_dma_buffer = 0; // Buffer para DMA
static uint8_t adc_dma_started = 0;          // Flag de DMA iniciado

// Estado del disparo (para evitar disparos repetidos)
static uint8_t last_fire_state = 0;

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

// Mapear valor float a rango entero
static int16_t map_float_to_int(float x, float in_min, float in_max,
                                int16_t out_min, int16_t out_max) {
  if (x < in_min)
    x = in_min;
  if (x > in_max)
    x = in_max;
  return (int16_t)((x - in_min) * (out_max - out_min) / (in_max - in_min) +
                   out_min);
}

// Enviar respuesta por UART
static void send_response(const char *msg) {
  if (uart_handle != NULL) {
    HAL_UART_Transmit(uart_handle, (uint8_t *)msg, strlen(msg), 100);
  }
}

// ============================================================================
// FUNCIONES PÚBLICAS
// ============================================================================

void GameManager_Init(UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc,
                      I2C_HandleTypeDef *hi2c) {
  uart_handle = huart;
  adc_handle = hadc;
  i2c_handle = hi2c;
  current_game = GAME_NONE;

  // Inicializar sensor GY521
  if (hi2c != NULL) {
    GY521_Init(&accel_sensor, hi2c, 0x69);

    // Intentar iniciar el sensor (con timeout)
    uint8_t retries = 3;
    while (retries > 0 && !sensor_ok) {
      if (GY521_begin(&accel_sensor) == true) {
        sensor_ok = 1;
        send_response("GY521 OK\r\n");
      } else {
        retries--;
        HAL_Delay(100);
      }
    }

    if (!sensor_ok) {
      send_response("GY521 FAIL\r\n");
    }
  }

  // Iniciar ADC con DMA (lectura continua)
  if (hadc != NULL && !adc_dma_started) {
    HAL_ADC_Start_DMA(hadc, (uint32_t *)&adc_dma_buffer, 1);
    adc_dma_started = 1;
    send_response("ADC DMA OK\r\n");
  }

  // Iniciar recepción UART por interrupción
  if (huart != NULL) {
    HAL_UART_Receive_IT(huart, &uart_rx_byte, 1);
    send_response("GameManager Ready\r\n");
  }
}

void GameManager_SetGame(GameType game) {
  // Si cambia el juego, inicializar el nuevo
  if (current_game != game) {
    current_game = game;

    switch (game) {
    case GAME_GALAGA:
      Galaga_SetManualControl(1); // Control manual
      Galaga_Init();
      send_response("GAME: GALAGA\r\n");
      break;

    case GAME_2:
      // TODO: Inicializar segundo juego
      send_response("GAME: 2 (TODO)\r\n");
      break;

    case GAME_NONE:
    default:
      ILI_FillScreen(0x0000); // Pantalla negra
      send_response("GAME: NONE\r\n");
      break;
    }
  }
}

GameType GameManager_GetGame(void) { return current_game; }

void GameManager_Loop(void) {
  // Actualizar inputs de sensores
  GameManager_UpdateInputs();

  // Ejecutar loop del juego activo
  switch (current_game) {
  case GAME_GALAGA:
    Galaga_Loop();
    break;

  case GAME_2:
    // TODO: Loop del segundo juego
    break;

  case GAME_NONE:
  default:
    // Sin juego activo, solo delay
    HAL_Delay(100);
    break;
  }
}

void GameManager_ProcessUART(uint8_t cmd) {
  char response[32];

  switch (cmd) {
  case 'G':
  case 'g':
  case '1':
    // Cambiar a Galaga
    GameManager_SetGame(GAME_GALAGA);
    break;

  case '2':
    // Cambiar a Juego 2
    GameManager_SetGame(GAME_2);
    break;

  case '0':
    // Detener juego
    GameManager_SetGame(GAME_NONE);
    break;

  case 'S':
  case 's':
    // Status
    sprintf(response, "STATUS: Game=%d\r\n", current_game);
    send_response(response);
    break;

  case 'R':
  case 'r':
    // Reiniciar juego actual
    if (current_game == GAME_GALAGA) {
      Galaga_Init();
      send_response("RESET OK\r\n");
    }
    break;

  case 'A':
  case 'a':
    // Modo auto-play
    if (current_game == GAME_GALAGA) {
      Galaga_SetManualControl(0);
      send_response("AUTOPLAY ON\r\n");
    }
    break;

  case 'M':
  case 'm':
    // Modo manual
    if (current_game == GAME_GALAGA) {
      Galaga_SetManualControl(1);
      send_response("MANUAL ON\r\n");
    }
    break;

  default:
    // Comando no reconocido
    break;
  }
}

void GameManager_UART_Callback(void) {
  // Procesar byte recibido
  GameManager_ProcessUART(uart_rx_byte);

  // Reiniciar recepción
  if (uart_handle != NULL) {
    HAL_UART_Receive_IT(uart_handle, &uart_rx_byte, 1);
  }
}

void GameManager_UpdateInputs(void) {
  // Solo actualizar si hay juego activo
  if (current_game == GAME_NONE)
    return;

  // =========================================
  // LEER ACELERÓMETRO (posición X del jugador)
  // =========================================
  if (sensor_ok) {
    GY521_read(&accel_sensor);

    // El eje X del acelerómetro controla la posición horizontal
    // Invertido: inclinar a la derecha = nave a la derecha
    float ax = accel_sensor.ax;

    // Mapear aceleración a posición de pantalla
    int16_t player_x =
        map_float_to_int(ax, ACCEL_MIN, ACCEL_MAX, 15, GM_SCREEN_WIDTH - 15);

    // Enviar al juego
    if (current_game == GAME_GALAGA) {
      Galaga_SetPlayerX(player_x);
    }
  }

  // =========================================
  // LEER ADC CON DMA (joystick para disparo)
  // =========================================
  // Joystick analógico: centro ~2048, extremos 0 y 4095
  // Disparamos cuando el valor sale de la zona neutral
  if (adc_dma_started) {
    uint32_t adc_value = adc_dma_buffer; // Leer del buffer DMA

    // Disparar si el joystick se mueve fuera de la zona muerta
    // Mover hacia arriba (>2548) o hacia abajo (<1548) = disparar
    uint8_t fire_now =
        (adc_value > ADC_FIRE_HIGH || adc_value < ADC_FIRE_LOW) ? 1 : 0;

    // Solo disparar en flanco (evitar disparos repetidos)
    if (fire_now && !last_fire_state) {
      if (current_game == GAME_GALAGA) {
        Galaga_Fire();
      }
    }

    last_fire_state = fire_now;
  }
}
