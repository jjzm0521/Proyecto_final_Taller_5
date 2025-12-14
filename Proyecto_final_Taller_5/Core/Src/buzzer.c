/**
 * @file buzzer.c
 * @brief Implementación del controlador de buzzer/melodías
 * @details Usa TIM3_CH3 (PB0) con PWM para generar tonos
 *
 * Configuración del timer:
 * - Timer Clock: 100 MHz (APB1 Timer clock)
 * - Prescaler: 99 -> 100 MHz / 100 = 1 MHz
 * - ARR se calcula dinámicamente: ARR = (1,000,000 / frecuencia) - 1
 */

#include "buzzer.h"

// ============================================================================
// VARIABLES PRIVADAS
// ============================================================================

static TIM_HandleTypeDef *buzzer_htim = NULL;

// Estado de reproducción no bloqueante
static const Nota *melodia_actual = NULL;
static uint16_t melodia_total_notas = 0;
static uint16_t melodia_nota_actual = 0;
static uint32_t melodia_tiempo_inicio = 0;
static uint8_t melodia_reproduciendo = 0;

// Constante para cálculo de ARR (Timer clock / (PSC + 1))
#define TIMER_FREQ_HZ 1000000 // 1 MHz después del prescaler

// Definición de silencio
#define REST 0

// ============================================================================
// STAR WARS MAIN THEME (frecuencias en Hz)
// Tempo: 108 BPM -> wholenote = 2222ms
// ============================================================================

const Nota MELODIA_PONG_1[] = {
    // Star Wars Main Theme - frecuencias en Hz, duraciones en ms
    // Compás 1-2
    {466, 277},
    {466, 277},
    {466, 277}, // AS4 x3
    {698, 1111},
    {1047, 1111}, // F5, C6
    // Compás 3-4
    {932, 277},
    {880, 277},
    {784, 277},
    {1397, 1111},
    {1047, 555},
    {932, 277},
    {880, 277},
    {784, 277},
    {1397, 1111},
    {1047, 555},
    // Compás 5-6
    {932, 277},
    {880, 277},
    {932, 277},
    {784, 1111},
    {523, 277},
    {523, 277},
    {523, 277},
    {698, 1111},
    {1047, 1111},
    // Compás 7-8
    {932, 277},
    {880, 277},
    {784, 277},
    {1397, 1111},
    {1047, 555},
    {932, 277},
    {880, 277},
    {784, 277},
    {1397, 1111},
    {1047, 555},
    // Compás 9
    {932, 277},
    {880, 277},
    {932, 277},
    {784, 1111},
    {523, 416},
    {523, 138},
    // Compás 10-11
    {587, 833},
    {587, 277},
    {932, 277},
    {880, 277},
    {784, 277},
    {698, 277},
    {698, 277},
    {784, 277},
    {880, 277},
    {784, 555},
    {587, 277},
    {659, 555},
    {523, 416},
    {523, 138},
    // Compás 12
    {587, 833},
    {587, 277},
    {932, 277},
    {880, 277},
    {784, 277},
    {698, 277},
    // Compás 13-14
    {1047, 416},
    {784, 138},
    {784, 1111},
    {REST, 277},
    {523, 277},
    {587, 833},
    {587, 277},
    {932, 277},
    {880, 277},
    {784, 277},
    {698, 277},
    // Compás 15
    {698, 277},
    {784, 277},
    {880, 277},
    {784, 555},
    {587, 277},
    {659, 555},
    {1047, 416},
    {1047, 138},
    // Compás 16 (Final)
    {1397, 555},
    {1245, 277},
    {1109, 555},
    {1047, 277},
    {932, 555},
    {831, 277},
    {784, 555},
    {698, 277},
    {1047, 2222} // C6 nota final
};
const uint16_t MELODIA_PONG_1_LEN = 71;

// Star Wars - versión corta (intro)
const Nota MELODIA_PONG_2[] = {
    {466, 277}, {466, 277}, {466, 277}, {698, 1111},  {1047, 1111},
    {932, 277}, {880, 277}, {784, 277}, {1397, 1111}, {1047, 555},
    {932, 277}, {880, 277}, {784, 277}, {1397, 1111}, {1047, 555},
    {932, 277}, {880, 277}, {932, 277}, {784, 2222}};
const uint16_t MELODIA_PONG_2_LEN = 20;

// ============================================================================
// FUNCIONES PRIVADAS
// ============================================================================

/**
 * @brief Configura el ARR y duty cycle para una frecuencia Hz dada
 * @param frecuencia Frecuencia en Hz (0 = silencio)
 */
static void Buzzer_SetFrecuencia(uint16_t frecuencia) {
  if (buzzer_htim == NULL)
    return;

  if (frecuencia == 0) {
    // Silencio: duty cycle = 0
    __HAL_TIM_SET_COMPARE(buzzer_htim, TIM_CHANNEL_3, 0);
  } else {
    // Calcular ARR para la frecuencia deseada
    uint32_t arr = (TIMER_FREQ_HZ / frecuencia) - 1;

    // Limitar ARR al máximo del timer (16 bits)
    if (arr > 65535)
      arr = 65535;
    if (arr < 1)
      arr = 1;

    // Configurar ARR
    __HAL_TIM_SET_AUTORELOAD(buzzer_htim, arr);

    // Duty cycle al 50% para mejor sonido
    __HAL_TIM_SET_COMPARE(buzzer_htim, TIM_CHANNEL_3, arr / 2);

    // Reiniciar contador y forzar evento de actualización
    __HAL_TIM_SET_COUNTER(buzzer_htim, 0);
    buzzer_htim->Instance->EGR = TIM_EGR_UG; // Forzar update event
  }
}

// ============================================================================
// FUNCIONES PÚBLICAS
// ============================================================================

void Buzzer_Init(TIM_HandleTypeDef *htim) {
  buzzer_htim = htim;

  // Iniciar PWM en canal 3
  HAL_TIM_PWM_Start(buzzer_htim, TIM_CHANNEL_3);

  // Empezar en silencio
  Buzzer_Detener();
}

void Buzzer_TocarNota(uint16_t frecuencia, uint16_t duracion) {
  Buzzer_SetFrecuencia(frecuencia);
  HAL_Delay(duracion);
  Buzzer_Detener();
}

void Buzzer_Detener(void) {
  if (buzzer_htim != NULL) {
    __HAL_TIM_SET_COMPARE(buzzer_htim, TIM_CHANNEL_3, 0);
  }
}

void Buzzer_TocarMelodia(const Nota *notas, uint16_t num_notas) {
  for (uint16_t i = 0; i < num_notas; i++) {
    if (notas[i].frecuencia > 0) {
      Buzzer_SetFrecuencia(notas[i].frecuencia);
      HAL_Delay(notas[i].duracion);
    } else {
      // Silencio
      Buzzer_Detener();
      HAL_Delay(notas[i].duracion);
    }
    // Pequeña pausa entre notas para separación
    Buzzer_Detener();
    HAL_Delay(10);
  }
  Buzzer_Detener();
}

// ============================================================================
// FUNCIONES NO BLOQUEANTES
// ============================================================================

void Buzzer_IniciarMelodia(const Nota *notas, uint16_t num_notas) {
  melodia_actual = notas;
  melodia_total_notas = num_notas;
  melodia_nota_actual = 0;
  melodia_reproduciendo = 1;

  // Empezar primera nota
  if (num_notas > 0) {
    Buzzer_SetFrecuencia(notas[0].frecuencia);
    melodia_tiempo_inicio = HAL_GetTick();
  }
}

uint8_t Buzzer_ActualizarMelodia(void) {
  if (!melodia_reproduciendo || melodia_actual == NULL) {
    return 0;
  }

  uint32_t tiempo_actual = HAL_GetTick();
  uint32_t duracion_nota = melodia_actual[melodia_nota_actual].duracion;

  // ¿Terminó la nota actual?
  if ((tiempo_actual - melodia_tiempo_inicio) >= duracion_nota) {
    // Pasar a la siguiente nota
    melodia_nota_actual++;

    if (melodia_nota_actual >= melodia_total_notas) {
      // Melodía terminada
      Buzzer_Detener();
      melodia_reproduciendo = 0;
      melodia_actual = NULL;
      return 0;
    }

    // Pequeña pausa entre notas (5ms de silencio)
    Buzzer_Detener();

    // Tocar siguiente nota
    Buzzer_SetFrecuencia(melodia_actual[melodia_nota_actual].frecuencia);
    melodia_tiempo_inicio = tiempo_actual + 5; // 5ms de pausa
  }

  return 1;
}

uint8_t Buzzer_EstaReproduciendo(void) { return melodia_reproduciendo; }

void Buzzer_DetenerMelodia(void) {
  melodia_reproduciendo = 0;
  melodia_actual = NULL;
  Buzzer_Detener();
}
