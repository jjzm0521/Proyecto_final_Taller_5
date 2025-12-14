/**
 * @file buzzer.h
 * @brief Controlador de buzzer/melodías usando PWM
 * @details Usa TIM3_CH3 (PB0) para generar tonos musicales
 */

#ifndef INC_BUZZER_H_
#define INC_BUZZER_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

// ============================================================================
// TIPOS Y ESTRUCTURAS
// ============================================================================

/**
 * @brief Estructura para una nota musical
 * @param frecuencia Frecuencia en Hz (0 = silencio)
 * @param duracion Duración en milisegundos
 */
typedef struct {
  uint16_t frecuencia;
  uint16_t duracion;
} Nota;

/**
 * @brief Estructura para una melodía
 */
typedef struct {
  const Nota *notas;
  uint16_t num_notas;
} Melodia;

// ============================================================================
// MELODÍAS PREDEFINIDAS
// ============================================================================

// MELODIA_PONG_1 - 21 notas
extern const Nota MELODIA_PONG_1[];
extern const uint16_t MELODIA_PONG_1_LEN;

// MELODIA_PONG_2 - 34 notas
extern const Nota MELODIA_PONG_2[];
extern const uint16_t MELODIA_PONG_2_LEN;

// ============================================================================
// FUNCIONES PÚBLICAS
// ============================================================================

/**
 * @brief Inicializa el buzzer (inicia PWM en TIM3_CH3)
 * @param htim Puntero al handle del timer (htim3)
 */
void Buzzer_Init(TIM_HandleTypeDef *htim);

/**
 * @brief Reproduce una nota individual
 * @param frecuencia Frecuencia en Hz (0 para silencio)
 * @param duracion Duración en milisegundos
 */
void Buzzer_TocarNota(uint16_t frecuencia, uint16_t duracion);

/**
 * @brief Detiene el sonido del buzzer
 */
void Buzzer_Detener(void);

/**
 * @brief Reproduce una melodía completa (bloqueante)
 * @param notas Array de notas
 * @param num_notas Número de notas en el array
 */
void Buzzer_TocarMelodia(const Nota *notas, uint16_t num_notas);

/**
 * @brief Inicia reproducción de melodía (no bloqueante)
 * @param notas Array de notas
 * @param num_notas Número de notas en el array
 */
void Buzzer_IniciarMelodia(const Nota *notas, uint16_t num_notas);

/**
 * @brief Actualiza la reproducción de melodía (llamar en el loop principal)
 * @return 1 si la melodía sigue reproduciéndose, 0 si terminó
 */
uint8_t Buzzer_ActualizarMelodia(void);

/**
 * @brief Verifica si hay una melodía reproduciéndose
 * @return 1 si está reproduciendo, 0 si no
 */
uint8_t Buzzer_EstaReproduciendo(void);

/**
 * @brief Detiene la melodía en reproducción
 */
void Buzzer_DetenerMelodia(void);

#endif /* INC_BUZZER_H_ */
