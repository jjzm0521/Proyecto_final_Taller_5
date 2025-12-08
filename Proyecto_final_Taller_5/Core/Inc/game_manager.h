/**
 * @file game_manager.h
 * @brief Game Manager - Maneja múltiples juegos y control por USART/Sensores
 */

#ifndef INC_GAME_MANAGER_H_
#define INC_GAME_MANAGER_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

// ============================================================================
// TIPOS DE JUEGO
// ============================================================================

typedef enum {
  GAME_NONE = 0,   // Sin juego activo
  GAME_GALAGA = 1, // Galaga
  GAME_2 = 2       // Segundo juego (futuro)
} GameType;

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

// Umbrales del ADC para joystick analógico (0-4095 para 12 bits)
// Centro en reposo: ~2048
// Disparamos cuando el valor sale de la zona neutral
#define ADC_CENTER 2048
#define ADC_DEADZONE 500 // Zona muerta alrededor del centro
#define ADC_FIRE_HIGH (ADC_CENTER + ADC_DEADZONE) // 2548
#define ADC_FIRE_LOW (ADC_CENTER - ADC_DEADZONE)  // 1548

// Rango del acelerómetro para mapear a pantalla
#define ACCEL_MIN -0.5f
#define ACCEL_MAX 0.5f

// Ancho de pantalla (para mapeo del acelerómetro)
#define GM_SCREEN_WIDTH 320

// ============================================================================
// FUNCIONES PÚBLICAS
// ============================================================================

/**
 * @brief Inicializar el GameManager
 * @param huart Puntero al handler UART para comunicación
 * @param hadc Puntero al handler ADC para joystick
 * @param hi2c Puntero al handler I2C para acelerómetro
 */
void GameManager_Init(UART_HandleTypeDef *huart, ADC_HandleTypeDef *hadc,
                      I2C_HandleTypeDef *hi2c);

/**
 * @brief Establecer el juego activo
 * @param game Tipo de juego a activar
 */
void GameManager_SetGame(GameType game);

/**
 * @brief Obtener el juego activo actualmente
 * @return Tipo de juego activo
 */
GameType GameManager_GetGame(void);

/**
 * @brief Loop principal del GameManager - llamar desde while(1)
 */
void GameManager_Loop(void);

/**
 * @brief Procesar comando USART recibido
 * @param cmd Caracter de comando recibido
 */
void GameManager_ProcessUART(uint8_t cmd);

/**
 * @brief Callback para recepción UART (llamar desde HAL_UART_RxCpltCallback)
 */
void GameManager_UART_Callback(void);

/**
 * @brief Actualizar inputs de sensores (acelerómetro y ADC)
 */
void GameManager_UpdateInputs(void);

#endif /* INC_GAME_MANAGER_H_ */
