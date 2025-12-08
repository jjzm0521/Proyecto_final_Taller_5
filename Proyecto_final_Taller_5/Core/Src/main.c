/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "STM32_GY521.h"
#include "TFT_ILI9486.h"
#include "galaga_game.h"
#include <stdint.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define GY521_ADDR 0x69

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim11;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

GY521 sensor;
volatile uint8_t flag_acelerometro = 0;

// ADC DMA buffer
volatile uint32_t adc_value = 0;

// Filtro de suavizado para posición del jugador
float filtered_ay = 0.0f;
#define SMOOTHING_FACTOR 0.15f // 0.0-1.0 (menor = más suave)

// Debug counter
uint32_t debug_counter = 0;

// Disparo con cooldown (auto-fire mientras esté en rango)
#define ADC_FIRE_MIN 0 // Rango de disparo: 0 a 400
#define ADC_FIRE_MAX 400
#define FIRE_COOLDOWN 10 // Ciclos entre disparos (más bajo = más rápido)
uint32_t fire_cooldown_counter = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM11_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM11_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

  // Inicializar Pantalla
  ILI_Init();

  // Mensaje de inicio por UART
  printf("=== GALAGA DEBUG ===\r\n");

  // Iniciar ADC con DMA
  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_value, 1);
  printf("ADC DMA iniciado\r\n");

  // Inicializar sensor GY521
  GY521_Init(&sensor, &hi2c1, GY521_ADDR);
  uint8_t gy_ok = 0;
  for (int i = 0; i < 5; i++) {
    if (GY521_begin(&sensor) == true) {
      gy_ok = 1;
      printf("GY521 OK!\r\n");
      break;
    }
    printf("GY521 intento %d...\r\n", i + 1);
    HAL_Delay(200);
  }
  if (!gy_ok) {
    printf("GY521 FALLO - continuando sin acelerometro\r\n");
  }

  // Iniciar juego
  Galaga_SetManualControl(1); // Modo manual
  Galaga_Init();
  printf("Galaga iniciado en modo MANUAL\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // === LEER ACELEROMETRO CON FILTRO ===
    if (gy_ok) {
      GY521_read(&sensor);

      // Aplicar filtro exponencial (suavizado)
      filtered_ay = filtered_ay * (1.0f - SMOOTHING_FACTOR) +
                    sensor.ay * SMOOTHING_FACTOR;

      // Mapear aceleracion filtrada a posicion X (rango -0.5 a 0.5g -> 15 a
      // 305)
      float ay = filtered_ay;
      if (ay < -0.5f)
        ay = -0.5f;
      if (ay > 0.5f)
        ay = 0.5f;
      int16_t player_x = (int16_t)((ay + 0.5f) * 290.0f + 15.0f);

      Galaga_SetPlayerX(player_x);
    }

    // === LEER ADC (joystick) - AUTO-FIRE ===
    uint32_t adc_now = adc_value; // Leer del DMA

    // Disparar mientras el joystick esté en el rango de disparo (0 a 400)
    // No requiere soltar y volver a presionar
    if (adc_now >= ADC_FIRE_MIN && adc_now <= ADC_FIRE_MAX) {
      // El joystick está en posición de disparo
      fire_cooldown_counter++;
      if (fire_cooldown_counter >= FIRE_COOLDOWN) {
        fire_cooldown_counter = 0;
        Galaga_Fire();
      }
    } else {
      // Fuera del rango de disparo, resetear cooldown para disparo inmediato al
      // entrar
      fire_cooldown_counter = FIRE_COOLDOWN;
    }

    // === DEBUG cada 100 ciclos ===
    debug_counter++;
    if (debug_counter >= 100) {
      debug_counter = 0;
      uint8_t in_fire_range =
          (adc_now >= ADC_FIRE_MIN && adc_now <= ADC_FIRE_MAX) ? 1 : 0;
      if (gy_ok) {
        printf("filt:%.2f ADC:%lu fire:%d\r\n", filtered_ay, adc_now,
               in_fire_range);
      } else {
        printf("ADC:%lu fire:%d\r\n", adc_now, in_fire_range);
      }
    }

    // === EJECUTAR JUEGO ===
    Galaga_Loop();
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data
   * Alignment and number of conversion)
   */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK) {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in
   * the sequencer and its sample time.
   */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */
}

/**
 * @brief TIM11 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM11_Init(void) {

  /* USER CODE BEGIN TIM11_Init 0 */

  /* USER CODE END TIM11_Init 0 */

  /* USER CODE BEGIN TIM11_Init 1 */

  /* USER CODE END TIM11_Init 1 */
  htim11.Instance = TIM11;
  htim11.Init.Prescaler = 99;
  htim11.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim11.Init.Period = 19999;
  htim11.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim11.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim11) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM11_Init 2 */

  /* USER CODE END TIM11_Init 2 */
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */
}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOH, GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(
      GPIOA, DB12_Pin | DB13_Pin | DB14_Pin | DB5_Pin | DB6_Pin | DB2_Pin,
      GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC,
                    DB1_Pin | DB8_Pin | DB9_Pin | DB7_Pin | DB10_Pin | DB11_Pin,
                    GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB,
                    CS_Pin | RST_Pin | DB4_Pin | DB0_Pin | RS_Pin | WR_Pin |
                        DB3_Pin | DB15_Pin,
                    GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PH1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pins : DB12_Pin DB13_Pin DB14_Pin */
  GPIO_InitStruct.Pin = DB12_Pin | DB13_Pin | DB14_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DB1_Pin DB8_Pin DB9_Pin DB7_Pin
                     DB10_Pin DB11_Pin */
  GPIO_InitStruct.Pin =
      DB1_Pin | DB8_Pin | DB9_Pin | DB7_Pin | DB10_Pin | DB11_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_Pin RST_Pin DB4_Pin DB0_Pin
                     RS_Pin WR_Pin */
  GPIO_InitStruct.Pin = CS_Pin | RST_Pin | DB4_Pin | DB0_Pin | RS_Pin | WR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : DB5_Pin DB6_Pin DB2_Pin */
  GPIO_InitStruct.Pin = DB5_Pin | DB6_Pin | DB2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DB3_Pin DB15_Pin */
  GPIO_InitStruct.Pin = DB3_Pin | DB15_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  // Verificar si la interrupción viene del Timer 11 (Pantalla)
  if (htim->Instance == TIM11) {
    flag_acelerometro = 1; // Encender bandera para refresco de LCD
    HAL_GPIO_TogglePin(GPIOH, GPIO_PIN_1);
  }
}

int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 10);
  return ch;
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state
   */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
line)
   */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
