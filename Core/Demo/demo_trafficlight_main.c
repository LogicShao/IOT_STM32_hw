/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Traffic Light Demo - 红绿灯控制系统
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
#include "buzzer.h"
#include "ledsegdisp.h"
#include "led.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum SystemMode {
    MODE_RUNNING,
    MODE_SETTINGS_RED,
    MODE_SETTINGS_GREEN
};

enum TrafficLightState {
    STATE_OFF,
    STATE_RED,
    STATE_GREEN
};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DISP_MAIN   1
#define DISP_RED    2
#define DISP_GREEN  3

#define DEFAULT_RED_DURATION    30
#define DEFAULT_GREEN_DURATION  20
#define MAX_DURATION            99
#define MIN_DURATION            5

#define BUZZER_FREQ             5
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
volatile enum SystemMode g_system_mode = MODE_RUNNING;
volatile enum TrafficLightState g_current_state = STATE_RED;
volatile int g_current_countdown = DEFAULT_RED_DURATION;
volatile int g_config_red_duration = DEFAULT_RED_DURATION;
volatile int g_config_green_duration = DEFAULT_GREEN_DURATION;
volatile int g_temp_red_duration;
volatile int g_temp_green_duration;
volatile bool g_flash_state = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Update_UI_Running_Mode(void);
static void Update_UI_Settings_Red_Mode(void);
static void Update_UI_Settings_Green_Mode(void);
static void Set_Traffic_LEDs(enum TrafficLightState state);
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
    Buzzer_Set_Frequency(BUZZER_FREQ);
    Buzzer_Off();

    for (int i = 1; i <= 3; i++) {
        ledSegDispOpen(i);
        ledSegDispClear(i);
    }

    HAL_TIM_Base_Start_IT(&htim6);
    /* USER CODE END 2 */

    /* USER CODE BEGIN WHILE */
    while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        switch (g_system_mode) {
            case MODE_RUNNING:
                Update_UI_Running_Mode();
                break;
            case MODE_SETTINGS_RED:
                Update_UI_Settings_Red_Mode();
                break;
            case MODE_SETTINGS_GREEN:
                Update_UI_Settings_Green_Mode();
                break;
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

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    switch (GPIO_Pin) {
        case RIGHT_KEY_Pin:
            if (g_system_mode == MODE_RUNNING) {
                g_system_mode = MODE_SETTINGS_RED;
                g_temp_red_duration = g_config_red_duration;
                g_temp_green_duration = g_config_green_duration;
            } else if (g_system_mode == MODE_SETTINGS_RED) {
                g_system_mode = MODE_SETTINGS_GREEN;
            } else {
                g_system_mode = MODE_SETTINGS_RED;
            }
            break;

        case LEFT_KEY_Pin:
            if (g_system_mode != MODE_RUNNING) {
                g_config_red_duration = g_temp_red_duration;
                g_config_green_duration = g_temp_green_duration;
                g_system_mode = MODE_RUNNING;
                g_current_state = STATE_RED;
                g_current_countdown = g_config_red_duration;
            }
            break;

        case UP_KEY_Pin:
            if (g_system_mode == MODE_SETTINGS_RED) {
                if (g_temp_red_duration < MAX_DURATION) g_temp_red_duration++;
            } else if (g_system_mode == MODE_SETTINGS_GREEN) {
                if (g_temp_green_duration < MAX_DURATION) g_temp_green_duration++;
            }
            break;

        case DOWN_KEY_Pin:
            if (g_system_mode == MODE_SETTINGS_RED) {
                if (g_temp_red_duration > MIN_DURATION) g_temp_red_duration--;
            } else if (g_system_mode == MODE_SETTINGS_GREEN) {
                if (g_temp_green_duration > MIN_DURATION) g_temp_green_duration--;
            }
            break;

        default:
            break;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    static uint32_t timer_ticks = 0;
    if (htim->Instance == TIM6) {
        timer_ticks++;
        g_flash_state = !g_flash_state;

        if (timer_ticks % 2 == 0) {
            if (g_system_mode == MODE_RUNNING) {
                if (g_current_countdown > 0) {
                    g_current_countdown--;
                } else {
                    if (g_current_state == STATE_RED) {
                        g_current_state = STATE_GREEN;
                        g_current_countdown = g_config_green_duration;
                    } else {
                        g_current_state = STATE_RED;
                        g_current_countdown = g_config_red_duration;
                    }
                }
            }
        }
    }
}

static void Update_UI_Running_Mode(void) {
    displayInteger(DISP_MAIN, g_current_countdown);
    displayInteger(DISP_RED, g_config_red_duration);
    displayInteger(DISP_GREEN, g_config_green_duration);
    Set_Traffic_LEDs(g_current_state);

    if (g_current_state == STATE_RED) {
        Buzzer_On();
    } else {
        Buzzer_Off();
    }
}

static void Update_UI_Settings_Red_Mode(void) {
    Set_Traffic_LEDs(STATE_OFF);
    Buzzer_Off();

    ledSegDispShowSign(DISP_MAIN, 1);
    ledSegDispShowSign(DISP_MAIN, 2);
    ledSegDispShowSign(DISP_MAIN, 3);
    ledSegDispShowSign(DISP_MAIN, 4);

    if (g_flash_state) {
        displayInteger(DISP_RED, g_temp_red_duration);
    } else {
        ledSegDispClear(DISP_RED);
    }

    displayInteger(DISP_GREEN, g_temp_green_duration);
}

static void Update_UI_Settings_Green_Mode(void) {
    Set_Traffic_LEDs(STATE_OFF);
    Buzzer_Off();

    ledSegDispShowSign(DISP_MAIN, 1);
    ledSegDispShowSign(DISP_MAIN, 2);
    ledSegDispShowSign(DISP_MAIN, 3);
    ledSegDispShowSign(DISP_MAIN, 4);

    displayInteger(DISP_RED, g_temp_red_duration);

    if (g_flash_state) {
        displayInteger(DISP_GREEN, g_temp_green_duration);
    } else {
        ledSegDispClear(DISP_GREEN);
    }
}

static void Set_Traffic_LEDs(const enum TrafficLightState state) {
    switch (state) {
        case STATE_RED:
            RED_LED_On();
            GRE_LED_Off();
            break;
        case STATE_GREEN:
            RED_LED_Off();
            GRE_LED_On();
            break;
        case STATE_OFF:
        default:
            RED_LED_Off();
            GRE_LED_Off();
            break;
    }
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
