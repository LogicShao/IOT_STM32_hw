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
// 系统模式枚举
enum SystemMode {
    MODE_RUNNING, // 正常运行倒计时
    MODE_SETTINGS_RED, // 正在设置红灯时间
    MODE_SETTINGS_GREEN // 正在设置绿灯时间
};

// 交通灯状态枚举
enum TrafficLightState {
    STATE_OFF, // 两个灯都熄灭 (用于设置模式)
    STATE_RED,
    STATE_GREEN
};

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// 数码管设备编号
#define DISP_MAIN   1
#define DISP_RED    2
#define DISP_GREEN  3

// 时间配置
#define DEFAULT_RED_DURATION    30
#define DEFAULT_GREEN_DURATION  20
#define MAX_DURATION            99
#define MIN_DURATION            5

// 蜂鸣器频率 (例如 4kHz，Freq = 40)
#define BUZZER_FREQ             5

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 系统模式状态
volatile enum SystemMode g_system_mode = MODE_RUNNING;

// 交通灯状态 (仅在 MODE_RUNNING 时使用)
volatile enum TrafficLightState g_current_state = STATE_RED;

// 当前倒计时秒数 (仅在 MODE_RUNNING 时使用)
volatile int g_current_countdown = DEFAULT_RED_DURATION;

// 已保存的配置时间
volatile int g_config_red_duration = DEFAULT_RED_DURATION;
volatile int g_config_green_duration = DEFAULT_GREEN_DURATION;

// 在设置模式中临时修改的时间
volatile int g_temp_red_duration;
volatile int g_temp_green_duration;

// 设置模式下 500ms 闪烁的状态
volatile bool g_flash_state = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Update_UI_Running_Mode(void);

static void Update_UI_Settings_Red_Mode(void);

static void Update_UI_Settings_Green_Mode(void);

// 硬件控制辅助函数
static void Set_Traffic_LEDs(enum TrafficLightState state);

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
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_USART4_UART_Init();
  MX_TIM15_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
    // 初始化蜂鸣器
    Buzzer_Init();
    Buzzer_Set_Frequency(BUZZER_FREQ); // 设置一个频率
    Buzzer_Off(); // 确保开机不响

    // 初始化并关闭数码管显示
    for (int i = 1; i <= 3; i++) {
        ledSegDispOpen(i);
        ledSegDispClear(i);
    }

    // 启动定时器中断
    HAL_TIM_Base_Start_IT(&htim6);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        // 根据当前系统模式更新显示和状态
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

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

// ============================================================================
// 中断回调函数
// ============================================================================

/**
 *  @brief GPIO 外部中断回调函数
 *  @param GPIO_Pin
 *  @retval None
 *  @note 根据按键引脚处理不同的按键事件
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    switch (GPIO_Pin) {
        // --- RIGHT_KEY: 切换模式 ---
        case RIGHT_KEY_Pin:
            if (g_system_mode == MODE_RUNNING) {
                // 从运行 -> 进入设置模式
                g_system_mode = MODE_SETTINGS_RED;
                // 立即加载当前配置，准备修改
                g_temp_red_duration = g_config_red_duration;
                g_temp_green_duration = g_config_green_duration;
            } else if (g_system_mode == MODE_SETTINGS_RED) {
                // 切换到设置绿灯
                g_system_mode = MODE_SETTINGS_GREEN;
            } else // g_system_mode == MODE_SETTINGS_GREEN
            {
                // 切换回设置红灯
                g_system_mode = MODE_SETTINGS_RED;
            }
            break;

        // --- LEFT_KEY: 退出并保存 ---
        case LEFT_KEY_Pin:
            // 仅在设置模式下有效
            if (g_system_mode != MODE_RUNNING) {
                // 保存临时设置为永久配置
                g_config_red_duration = g_temp_red_duration;
                g_config_green_duration = g_temp_green_duration;

                // 退出设置模式，返回运行模式
                g_system_mode = MODE_RUNNING;

                // 立即重置为红灯状态，并从新配置加载倒计时
                g_current_state = STATE_RED;
                g_current_countdown = g_config_red_duration;
            }
            break;

        // --- UP_KEY: 增加时间 ---
        case UP_KEY_Pin:
            if (g_system_mode == MODE_SETTINGS_RED) {
                if (g_temp_red_duration < MAX_DURATION) {
                    g_temp_red_duration++;
                }
            } else if (g_system_mode == MODE_SETTINGS_GREEN) {
                if (g_temp_green_duration < MAX_DURATION) {
                    g_temp_green_duration++;
                }
            }
            break;

        // --- DOWN_KEY: 减少时间 ---
        case DOWN_KEY_Pin:
            if (g_system_mode == MODE_SETTINGS_RED) {
                if (g_temp_red_duration > MIN_DURATION) {
                    g_temp_red_duration--;
                }
            } else if (g_system_mode == MODE_SETTINGS_GREEN) {
                if (g_temp_green_duration > MIN_DURATION) {
                    g_temp_green_duration--;
                }
            }
            break;

        default:
            // 意外的中断
            break;
    }
}

/**
 *  @brief 定时器中断回调函数
 *  @param htim 定时器句柄
 *  @retval None
 *  @note 用于 500ms 闪烁和 1 秒倒计时
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    // 静态变量，用于 500ms -> 1s 的计数
    static uint32_t timer_ticks = 0;
    if (htim->Instance == TIM6) {
        timer_ticks++;
        // 切换闪烁状态 (每 500ms 执行一次)
        g_flash_state = !g_flash_state;

        // 检查是否过去了 1 秒 (每 2 次 500ms 中断执行一次)
        if (timer_ticks % 2 == 0) {
            // 1 秒倒计时逻辑
            // 只有在 "运行模式" 下才执行倒计时
            if (g_system_mode == MODE_RUNNING) {
                if (g_current_countdown > 0) {
                    g_current_countdown--;
                } else {
                    // 倒计时归零，切换状态
                    if (g_current_state == STATE_RED) {
                        g_current_state = STATE_GREEN;
                        g_current_countdown = g_config_green_duration; // 加载绿灯时间
                    } else {
                        g_current_state = STATE_RED;
                        g_current_countdown = g_config_red_duration; // 加载红灯时间
                    }
                }
            }
        }
    }
}

// ============================================================================
// 显示更新函数
// ============================================================================
/**
 *  @brief 在 MODE_RUNNING 状态下更新所有显示
 */
static void Update_UI_Running_Mode(void) {
    // 数码管1: 显示当前倒计时
    displayInteger(DISP_MAIN, g_current_countdown);

    // 数码管2: 显示已保存的红灯时间
    displayInteger(DISP_RED, g_config_red_duration);

    // 数码管3: 显示已保存的绿灯时间
    displayInteger(DISP_GREEN, g_config_green_duration);

    // 根据状态设置LED
    Set_Traffic_LEDs(g_current_state);

    // 根据状态控制蜂鸣器
    if (g_current_state == STATE_RED) {
        Buzzer_On(); // 红灯时蜂鸣器响
    } else {
        Buzzer_Off(); // 绿灯时关闭
    }
}

/**
 *  @brief 在 MODE_SETTINGS_RED 状态下更新所有显示
 */
static void Update_UI_Settings_Red_Mode(void) {
    // 关闭LED和蜂鸣器
    Set_Traffic_LEDs(STATE_OFF);
    Buzzer_Off();

    // 数码管1: 显示 "----" 提示
    ledSegDispShowSign(DISP_MAIN, 1);
    ledSegDispShowSign(DISP_MAIN, 2);
    ledSegDispShowSign(DISP_MAIN, 3);
    ledSegDispShowSign(DISP_MAIN, 4);

    // 数码管2 (红灯): 闪烁显示正在编辑的 "temp" 值
    if (g_flash_state) {
        displayInteger(DISP_RED, g_temp_red_duration);
    } else {
        ledSegDispClear(DISP_RED);
    }

    // 数码管3 (绿灯): 保持显示 "temp" 值 (不闪烁)
    displayInteger(DISP_GREEN, g_temp_green_duration);
}

/**
 * @brief 在 MODE_SETTINGS_GREEN 状态下更新所有显示
 */
static void Update_UI_Settings_Green_Mode(void) {
    // 关闭LED和蜂鸣器
    Set_Traffic_LEDs(STATE_OFF);
    Buzzer_Off();

    // 数码管1: 显示 "----" 提示
    ledSegDispShowSign(DISP_MAIN, 1);
    ledSegDispShowSign(DISP_MAIN, 2);
    ledSegDispShowSign(DISP_MAIN, 3);
    ledSegDispShowSign(DISP_MAIN, 4);

    // 数码管2 (红灯): 保持显示 "temp" 值 (不闪烁)
    displayInteger(DISP_RED, g_temp_red_duration);

    // 数码管3 (绿灯): 闪烁显示正在编辑的 "temp" 值
    if (g_flash_state) {
        displayInteger(DISP_GREEN, g_temp_green_duration);
    } else {
        ledSegDispClear(DISP_GREEN);
    }
}


// ============================================================================
// 硬件控制辅助函数
// ============================================================================

/**
 * @brief 控制红绿LED灯的亮灭
 */
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

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
