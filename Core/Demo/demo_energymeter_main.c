/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Energy Meter Demo - 直流电能表数据采集
  * @note           : 复制到 Core/Src/main.c 运行
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "led.h"
#include "buzzer.h"
#include "ledsegdisp.h"
#include "rs485.h"
#include "ht7017.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DISP_VOLTAGE 1
#define DISP_CURRENT 2
#define DISP_POWER   3

#define SAMPLE_INTERVAL_TICKS 4
#define BUZZER_FREQ 5
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
HT7017_HandleTypeDef h_ht7017;
volatile int32_t g_raw_voltage = 0;
volatile int32_t g_raw_current = 0;
volatile int32_t g_raw_power = 0;
volatile bool g_sample_ready = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Update_Display(void);
static void Transmit_Data(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

int main(void) {
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    HAL_Init();
    /* USER CODE BEGIN Init */
    /* USER CODE END Init */
    SystemClock_Config();
    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_USART4_UART_Init();
    MX_TIM15_Init();
    MX_TIM6_Init();
    /* USER CODE BEGIN 2 */
    Buzzer_Init();
    Buzzer_Off();

    for (int i = 1; i <= 3; i++) {
        ledSegDispOpen(i);
        ledSegDispClear(i);
    }

    TEST_LED_On();

    HT7017_Init(&h_ht7017, &huart1, 0x0000);
    if (HT7017_WriteReg(&h_ht7017, 0x59, 0x0000) != HAL_OK) {
        TEST_LED_Off();
        Error_Handler();
    }

    Buzzer_Set_Frequency(BUZZER_FREQ);
    Buzzer_On();
    HAL_Delay(200);
    Buzzer_Off();

    HAL_TIM_Base_Start_IT(&htim6);
    /* USER CODE END 2 */

    /* USER CODE BEGIN WHILE */
    while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        if (g_sample_ready) {
            g_sample_ready = false;
            Update_Display();
            Transmit_Data();
        }
    }
    /* USER CODE END 3 */
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
    RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_I2C1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    static uint32_t tick_count = 0;
    if (htim->Instance == TIM6) {
        tick_count++;

        if (tick_count % 2 == 0) {
            HAL_GPIO_TogglePin(TEST_GPIO_Port, TEST_Pin);
        }

        if (tick_count >= SAMPLE_INTERVAL_TICKS) {
            tick_count = 0;

            HT7017_GetSignedSample(&h_ht7017, HT7017_REG_SPL_U, (int32_t *)&g_raw_voltage);
            HT7017_GetSignedSample(&h_ht7017, HT7017_REG_SPL_I1, (int32_t *)&g_raw_current);
            HT7017_GetSignedSample(&h_ht7017, HT7017_REG_POWER_P1, (int32_t *)&g_raw_power);

            g_sample_ready = true;
        }
    }
}

static void Update_Display(void) {
    displayInteger(DISP_VOLTAGE, (int)g_raw_voltage);
    displayInteger(DISP_CURRENT, (int)g_raw_current);
    displayInteger(DISP_POWER, (int)g_raw_power);
}

static void Transmit_Data(void) {
    char buf[64];

    RS485_Transmit((uint8_t *)"U:", 2);
    int len = sprintf(buf, "%ld", (long)g_raw_voltage);
    RS485_Transmit((uint8_t *)buf, (uint16_t)len);

    RS485_Transmit((uint8_t *)" I:", 3);
    len = sprintf(buf, "%ld", (long)g_raw_current);
    RS485_Transmit((uint8_t *)buf, (uint16_t)len);

    RS485_Transmit((uint8_t *)" P:", 3);
    len = sprintf(buf, "%ld", (long)g_raw_power);
    RS485_Transmit((uint8_t *)buf, (uint16_t)len);

    RS485_Transmit((uint8_t *)"\r\n", 2);
}

/* USER CODE END 4 */

void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
