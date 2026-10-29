/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "cmsis_os.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

RTC_HandleTypeDef hrtc;

/* Definitions for taskBattery */
osThreadId_t taskBatteryHandle;
const osThreadAttr_t taskBattery_attributes = {
  .name = "taskBattery",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for taskPower */
osThreadId_t taskPowerHandle;
const osThreadAttr_t taskPower_attributes = {
  .name = "taskPower",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for taskSurge */
osThreadId_t taskSurgeHandle;
const osThreadAttr_t taskSurge_attributes = {
  .name = "taskSurge",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for queueBatteryHandle */
osMessageQueueId_t queueBatteryHandleHandle;
const osMessageQueueAttr_t queueBatteryHandle_attributes = {
  .name = "queueBatteryHandle"
};
/* USER CODE BEGIN PV */
volatile uint16_t last_battery_percent = 100;
volatile uint32_t surge_counter = 0;
volatile uint8_t surge_protection_active = 0; // 1 = Modo de isolamento pós-raio ativo
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_RTC_Init(void);
void StartTaskBattery(void *argument);
void StartTaskPower(void *argument);
void StartTaskSurge(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of queueBatteryHandle */
  queueBatteryHandleHandle = osMessageQueueNew (4, sizeof(uint16_t), &queueBatteryHandle_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of taskBattery */
  taskBatteryHandle = osThreadNew(StartTaskBattery, NULL, &taskBattery_attributes);

  /* creation of taskPower */
  taskPowerHandle = osThreadNew(StartTaskPower, NULL, &taskPower_attributes);

  /* creation of taskSurge */
  taskSurgeHandle = osThreadNew(StartTaskSurge, NULL, &taskSurge_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED_ANTENNA_Pin|LED_COOLING_Pin|LED_LIGHTING_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BTN_DPS_SURGE_Pin */
  GPIO_InitStruct.Pin = BTN_DPS_SURGE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN_DPS_SURGE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_ANTENNA_Pin LED_COOLING_Pin LED_LIGHTING_Pin */
  GPIO_InitStruct.Pin = LED_ANTENNA_Pin|LED_COOLING_Pin|LED_LIGHTING_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// Callback disparado imediatamente quando o botão PA0 é pressionado (Gatilho de Raio)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == BTN_DPS_SURGE_Pin)
  {
    // Acorda instantaneamente a tarefa de emergência com prioridade máxima
    osThreadFlagsSet(taskSurgeHandle, 0x01);
  }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartTaskBattery */
/**
  * @brief  Function implementing the taskBattery thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTaskBattery */
void StartTaskBattery(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    // 1. Dispara a leitura analógica do potenciômetro (PA1)
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
      uint32_t adc_raw = HAL_ADC_GetValue(&hadc1); // Lê valor de 0 a 4095
      uint16_t percent = (uint16_t)((adc_raw * 100) / 4095); // Converte para 0 a 100%

      last_battery_percent = percent; // Atualiza a telemetria global

      // Envia a porcentagem para a Fila do FreeRTOS
      osMessageQueuePut(queueBatteryHandleHandle, &percent, 0U, 0U);
    }

    // Amostragem periódica a cada 200 ms (Soft Real-Time)
    osDelay(200);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTaskPower */
/**
* @brief Function implementing the taskPower thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskPower */
void StartTaskPower(void *argument)
{
  /* USER CODE BEGIN StartTaskPower */
  for(;;)
  {
    uint16_t battery_level = 0;

    if (osMessageQueueGet(queueBatteryHandleHandle, &battery_level, NULL, osWaitForever) == osOK)
    {
      // Se o raio caiu, desarma 100% de tudo (Modo Casulo / Isolamento Total)
      if (surge_protection_active)
      {
        HAL_GPIO_WritePin(LED_ANTENNA_GPIO_Port, LED_ANTENNA_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_COOLING_GPIO_Port, LED_COOLING_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_LIGHTING_GPIO_Port, LED_LIGHTING_Pin, GPIO_PIN_RESET);
      }
      else
      {
        // --- REGIME NORMAL DE DESCARTE DE BATERIA ---
        if (battery_level > 50)
        {
          // > 50%: Normal (Tudo ligado)
          HAL_GPIO_WritePin(LED_ANTENNA_GPIO_Port, LED_ANTENNA_Pin, GPIO_PIN_SET);
          HAL_GPIO_WritePin(LED_COOLING_GPIO_Port, LED_COOLING_Pin, GPIO_PIN_SET);
          HAL_GPIO_WritePin(LED_LIGHTING_GPIO_Port, LED_LIGHTING_Pin, GPIO_PIN_SET);
        }
        else if (battery_level > 15)
        {
          // 16% a 50%: Alerta (Desliga Iluminação)
          HAL_GPIO_WritePin(LED_ANTENNA_GPIO_Port, LED_ANTENNA_Pin, GPIO_PIN_SET);
          HAL_GPIO_WritePin(LED_COOLING_GPIO_Port, LED_COOLING_Pin, GPIO_PIN_SET);
          HAL_GPIO_WritePin(LED_LIGHTING_GPIO_Port, LED_LIGHTING_Pin, GPIO_PIN_RESET);
        }
        else if (battery_level > 3)
        {
          // 4% a 15%: Crítico (Desliga Climatização, preserva unicamente a Antena)
          HAL_GPIO_WritePin(LED_ANTENNA_GPIO_Port, LED_ANTENNA_Pin, GPIO_PIN_SET);
          HAL_GPIO_WritePin(LED_COOLING_GPIO_Port, LED_COOLING_Pin, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(LED_LIGHTING_GPIO_Port, LED_LIGHTING_Pin, GPIO_PIN_RESET);
        }
        else
        {
          // <= 3%: Bateria Esgotada / Blecaute Total (Apaga TUDO)
          HAL_GPIO_WritePin(LED_ANTENNA_GPIO_Port, LED_ANTENNA_Pin, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(LED_COOLING_GPIO_Port, LED_COOLING_Pin, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(LED_LIGHTING_GPIO_Port, LED_LIGHTING_Pin, GPIO_PIN_RESET);
        }
      }

      // =======================================================================
      // ENVIO DE TELEMETRIA PARA A DASHBOARD WEB (USB CDC)
      // =======================================================================
      uint8_t l1 = (HAL_GPIO_ReadPin(LED_ANTENNA_GPIO_Port, LED_ANTENNA_Pin) == GPIO_PIN_SET) ? 1 : 0;
      uint8_t l2 = (HAL_GPIO_ReadPin(LED_COOLING_GPIO_Port, LED_COOLING_Pin) == GPIO_PIN_SET) ? 1 : 0;
      uint8_t l3 = (HAL_GPIO_ReadPin(LED_LIGHTING_GPIO_Port, LED_LIGHTING_Pin) == GPIO_PIN_SET) ? 1 : 0;

      extern USBD_HandleTypeDef hUsbDeviceFS;
      if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED)
      {
        static char tx_buf[128];
        int len = snprintf(tx_buf, sizeof(tx_buf),
            "{\"bat\":%u,\"l1\":%u,\"l2\":%u,\"l3\":%u,\"casulo\":%u}\r\n",
            battery_level, l1, l2, l3, (surge_protection_active ? 1 : 0));

        CDC_Transmit_FS((uint8_t*)tx_buf, len);
      }
      // =======================================================================
    }
  }
  /* USER CODE END StartTaskPower */
}

/* USER CODE BEGIN Header_StartTaskSurge */
/**
* @brief Function implementing the taskSurge thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskSurge */
void StartTaskSurge(void *argument)
{
  /* USER CODE BEGIN StartTaskSurge */
  for(;;)
  {
    // 1. Aguarda a interrupção de hardware do botão/DPS (PA0)
    osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);

    // 2. Ativa o estado de contingência
    surge_protection_active = 1;

    // 3. Desacoplamento preventivo imediato de 100% das cargas
    HAL_GPIO_WritePin(LED_ANTENNA_GPIO_Port, LED_ANTENNA_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_COOLING_GPIO_Port, LED_COOLING_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_LIGHTING_GPIO_Port, LED_LIGHTING_Pin, GPIO_PIN_RESET);

    // 4. Incrementa o contador de surtos (memória não-volátil do RTC BKP)
    // Se você estiver usando a variável global surge_counter:
    surge_counter++;
    #if defined(RTC_BKP_DR1)
      HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, surge_counter);
    #endif

    // 5. Envia o alerta vermelho instantâneo para a dashboard no navegador
    extern USBD_HandleTypeDef hUsbDeviceFS;
    if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED)
    {
      static char surge_buf[80];
      int len = snprintf(surge_buf, sizeof(surge_buf),
          "{\"event\":\"SURGE\",\"count\":%lu,\"casulo\":1}\r\n",
          (unsigned long)surge_counter);
      CDC_Transmit_FS((uint8_t*)surge_buf, len);
    }

    // 6. Janela de amortecimento e isolamento total (Modo Casulo: 3 segundos)
    osDelay(3000);

    // 7. Restabelece a operação normal após o transitório do raio passar
    surge_protection_active = 0;

    // 8. Notifica a dashboard que o modo casulo encerrou
    if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED)
    {
      static char restore_buf[64];
      int len = snprintf(restore_buf, sizeof(restore_buf),
          "{\"event\":\"RESTORE\",\"casulo\":0}\r\n");
      CDC_Transmit_FS((uint8_t*)restore_buf, len);
    }
  }
  /* USER CODE END StartTaskSurge */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
