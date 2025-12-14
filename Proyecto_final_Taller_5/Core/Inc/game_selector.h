/**
 * @file game_selector.h
 * @brief Selector de Juegos - Gestiona múltiples juegos con cambio via UART
 *
 * Comandos UART:
 *   - 'R' o 'r': Racing Game
 *   - 'D' o 'd': Dino Runner
 *   - 'G' o 'g': Galaga
 */

#ifndef INC_GAME_SELECTOR_H_
#define INC_GAME_SELECTOR_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

// ============================================================================
// ENUMERACIÓN DE JUEGOS
// ============================================================================

typedef enum {
  GAME_RACING = 0, // Racing Game (acelerómetro + joystick)
  GAME_DINO,       // Dino Runner (joystick)
  GAME_GALAGA      // Galaga (acelerómetro + joystick)
} GameType;

// ============================================================================
// PROTOTIPOS DE FUNCIONES
// ============================================================================

/**
 * @brief Inicializa el selector de juegos
 * @param hadc Handle del ADC para joystick
 * @param hi2c Handle del I2C para acelerómetro
 * @param huart Handle del UART para comandos
 * @param juego_inicial Juego con el que empezar
 */
void GameSelector_Init(ADC_HandleTypeDef *hadc, I2C_HandleTypeDef *hi2c,
                       UART_HandleTypeDef *huart, GameType juego_inicial);

/**
 * @brief Loop principal - ejecuta el juego actual
 */
void GameSelector_Loop(void);

/**
 * @brief Cambia al juego especificado
 * @param nuevo_juego Juego al cual cambiar
 */
void GameSelector_SetGame(GameType nuevo_juego);

/**
 * @brief Procesa un carácter recibido por UART
 * @param caracter Carácter recibido
 */
void GameSelector_ProcessUART(uint8_t caracter);

/**
 * @brief Obtiene el juego actual
 * @return GameType juego activo
 */
GameType GameSelector_GetCurrentGame(void);

#endif /* INC_GAME_SELECTOR_H_ */
