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
#include <stdbool.h>
#include <buzzer.h>
#include <ledsegdisp.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum SystemState {
    STATE_IDLE, // 空闲 (电梯停靠，等待用户)
    STATE_SELECT_CURRENT, // 选择用户楼层 (用户正在选择"我在几楼")
    STATE_SELECT_TARGET, // 选择目标楼层 (用户正在选择"我去几楼")
    STATE_MOVING_TO_USER, // 移动中 (去接用户)
    STATE_DOOR_OPEN_USER, // 到达用户层 (黄灯亮, 等待3秒)
    STATE_MOVING_TO_TARGET, // 移动中 (送用户去目标)
    STATE_DOOR_OPEN_TARGET, // 到达目标层 (绿灯亮, 等待3秒)
    STATE_RETURNING_HOME // 返回1楼 (任务完成, 自动返回1楼)
};

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// --- 数码管定义 ---
#define DISP_ELEVATOR 1  // 显示电梯【实际】位置
#define DISP_USER     2  // 显示【用户所在】楼层
#define DISP_TARGET   3  // 显示【目标】楼层

// --- 楼层限制 ---
#define FLOOR_MAX       15
#define FLOOR_MIN       1
#define FLOOR_HOME      1  // 初始及返回的楼层

// --- 等待时间 ---
#define DOOR_WAIT_SECONDS 3

// --- 蜂鸣器频率 ---
#define BUZZER_FREQ 40
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// --- 核心状态变量 (必须是 volatile) ---
volatile enum SystemState g_system_state = STATE_IDLE;

// --- 楼层数据 ---
volatile int8_t g_elevator_current_floor = FLOOR_HOME; // 电梯的【实际】位置
volatile int8_t g_user_selected_floor = FLOOR_HOME; // 用户已确认的所在楼层
volatile int8_t g_user_target_floor = FLOOR_HOME; // 用户已确认的目标楼层

// --- 临时选择变量 (用于按键选择) ---
volatile int8_t g_temp_select_floor;

// --- 计时器/标志位 ---
volatile uint8_t g_wait_counter = 0; // 用于3秒倒计时
volatile bool g_flash_state = false; // 用于数码管闪烁 (由1秒定时器翻转)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
static void Update_LEDs(void);

static void Update_Displays(void);

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
    Buzzer_Init(); // 初始化蜂鸣器
    Buzzer_Set_Frequency(BUZZER_FREQ); // 设置蜂鸣器频率
    Buzzer_Off(); // 确保开机不响

    // 初始化三个数码管
    for (int i = 1; i <= 3; i++) {
        ledSegDispOpen(i);
        ledSegDispClear(i);
    }
    HAL_TIM_Base_Start_IT(&htim6);
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        // 更新3个LED灯 (红/黄/绿)
        Update_LEDs();

        // 更新3个数码管
        Update_Displays();
        HAL_Delay(50);
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
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
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

    /** Initializes the CPU, AHB and APB buses clocks
    */
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

// ============================================================================
// 中断回调函数
// ============================================================================

/**
  *  @brief 定时器中断回调函数
  *  @param htim 定时器句柄
  *  @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        // 翻转闪烁标志位 (用于UI)
        g_flash_state = !g_flash_state;

        // 根据状态执行逻辑
        switch (g_system_state) {
            // --- 移动: 去接用户 ---
            case STATE_MOVING_TO_USER:
                if (g_elevator_current_floor < g_user_selected_floor) g_elevator_current_floor++;
                else if (g_elevator_current_floor > g_user_selected_floor) g_elevator_current_floor--;
                else {
                    // 到达!
                    g_system_state = STATE_DOOR_OPEN_USER;
                    g_wait_counter = DOOR_WAIT_SECONDS; // 设置3秒等待
                    Buzzer_On(); // 蜂鸣器响一下
                }
                break;

            // --- 开门: 到达用户层 ---
            case STATE_DOOR_OPEN_USER:
                Buzzer_Off(); // 关闭蜂鸣器
                if (g_wait_counter > 0) g_wait_counter--;
                else {
                    // 3秒结束，去往目标
                    g_system_state = STATE_MOVING_TO_TARGET;
                }
                break;

            // --- 移动: 送用户去目标 ---
            case STATE_MOVING_TO_TARGET:
                if (g_elevator_current_floor < g_user_target_floor) g_elevator_current_floor++;
                else if (g_elevator_current_floor > g_user_target_floor) g_elevator_current_floor--;
                else {
                    // 到达
                    g_system_state = STATE_DOOR_OPEN_TARGET;
                    g_wait_counter = DOOR_WAIT_SECONDS; // 设置3秒等待
                    Buzzer_On(); //
                }
                break;

            // --- 开门: 到达目标层 ---
            case STATE_DOOR_OPEN_TARGET:
                Buzzer_Off(); //
                if (g_wait_counter > 0) g_wait_counter--;
                else {
                    // 3秒结束 (且用户未按键, 状态未被EXTI改变)
                    if (g_elevator_current_floor == FLOOR_HOME) {
                        g_system_state = STATE_IDLE; // 已经在1楼，直接空闲
                    } else {
                        g_system_state = STATE_RETURNING_HOME; // 返回1楼
                    }
                }
                break;

            // --- 移动: 返回1楼 ---
            case STATE_RETURNING_HOME:
                if (g_elevator_current_floor < FLOOR_HOME) g_elevator_current_floor++;
                else if (g_elevator_current_floor > FLOOR_HOME) g_elevator_current_floor--;
                else {
                    // 成功返回1楼
                    g_system_state = STATE_IDLE;
                }
                break;

            case STATE_IDLE:
            case STATE_SELECT_CURRENT:
            case STATE_SELECT_TARGET:
            default:
                // 在这些状态下，1秒定时器只负责闪烁，不执行逻辑
                break;
        }
    }
}

/**
  *  @brief GPIO 外部中断回调函数
  *  @param GPIO_Pin
  *  @retval None
  *  @note 根据按键引脚处理不同的按键事件
  */
void HAL_GPIO_EXTI_Callback(const uint16_t GPIO_Pin) {
    switch (GPIO_Pin) {
        // --- KEY_SELECT (选择键): 切换选择状态 ---
        case RIGHT_KEY_Pin:
            if (g_system_state == STATE_IDLE) {
                // 空闲 -> 选择 "用户楼层"
                g_system_state = STATE_SELECT_CURRENT;
                g_temp_select_floor = g_elevator_current_floor; // 默认从电梯当前楼层开始选
            } else if (g_system_state == STATE_SELECT_CURRENT) {
                // "用户楼层" -> "目标楼层" (Toggle)
                g_user_selected_floor = g_temp_select_floor; // 保存 "用户楼层" 的选择
                g_system_state = STATE_SELECT_TARGET;
                // 切换时，从 "已确认" 的目标楼层开始编辑
                g_temp_select_floor = g_user_target_floor;
            } else if (g_system_state == STATE_SELECT_TARGET) // <<< NEW LOGIC
            {
                // "目标楼层" -> "用户楼层" (Toggle)
                g_user_target_floor = g_temp_select_floor; // 保存 "目标楼层" 的选择
                g_system_state = STATE_SELECT_CURRENT;
                // 切换时，从 "已确认" 的用户楼层开始编辑
                g_temp_select_floor = g_user_selected_floor;
            } else if (g_system_state == STATE_DOOR_OPEN_TARGET) {
                // (关键功能) 在3秒等待期按下了
                // 立即中断等待, 开始新任务, 取消 "返回1楼"
                g_wait_counter = 0; // 清除等待
                g_system_state = STATE_SELECT_CURRENT;
                g_temp_select_floor = g_elevator_current_floor;
            }
            break;

        // --- KEY_CONFIRM (确认键): 启动电梯 ---
        case LEFT_KEY_Pin:
            if (g_system_state == STATE_SELECT_TARGET) {
                // 保存最后的目标楼层选择
                g_user_target_floor = g_temp_select_floor;

                // 检查: 目标和起点相同，无效任务
                if (g_user_target_floor == g_user_selected_floor) {
                    g_system_state = STATE_IDLE; // 返回空闲
                    break;
                }

                // 检查: 电梯已在用户层
                if (g_elevator_current_floor == g_user_selected_floor) {
                    // 直接开门 (黄灯)
                    g_system_state = STATE_DOOR_OPEN_USER;
                    g_wait_counter = DOOR_WAIT_SECONDS; // 设置3秒等待
                } else {
                    // 启动电梯
                    g_system_state = STATE_MOVING_TO_USER;
                }
            }
            break;

        // --- KEY_UP (上键): 增加楼层 ---
        case UP_KEY_Pin:
            if (g_system_state == STATE_SELECT_CURRENT || g_system_state == STATE_SELECT_TARGET) {
                if (g_temp_select_floor < FLOOR_MAX) {
                    g_temp_select_floor++;
                }
            }
            break;

        // --- KEY_DOWN (下键): 减少楼层 ---
        case DOWN_KEY_Pin:
            if (g_system_state == STATE_SELECT_CURRENT || g_system_state == STATE_SELECT_TARGET) {
                if (g_temp_select_floor > FLOOR_MIN) {
                    g_temp_select_floor--;
                }
            }
            break;

        default:
            break;
    }
}

// ============================================================================
// UI 更新函数
// ============================================================================

/**
  * @brief 更新3个LED (红/黄/绿) 的状态
  */
static void Update_LEDs(void) {
    // 默认全部关闭
    HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(YEL_GPIO_Port, YEL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GRE_GPIO_Port, GRE_Pin, GPIO_PIN_RESET);

    switch (g_system_state) {
        case STATE_MOVING_TO_USER:
        case STATE_MOVING_TO_TARGET:
        case STATE_RETURNING_HOME:
            // 只要电梯在动，红灯亮
            HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_SET);
            break;
        case STATE_DOOR_OPEN_USER:
            // 到达用户层，黄灯亮
            HAL_GPIO_WritePin(YEL_GPIO_Port, YEL_Pin, GPIO_PIN_SET);
            break;
        case STATE_DOOR_OPEN_TARGET:
            // 到达目标层，绿灯亮
            HAL_GPIO_WritePin(GRE_GPIO_Port, GRE_Pin, GPIO_PIN_SET);
            break;

        case STATE_IDLE:
        case STATE_SELECT_CURRENT:
        case STATE_SELECT_TARGET:
        default:
            // 空闲或选择状态，所有灯都灭
            break;
    }
}

/**
  * @brief 更新3个数码管的显示
  */
static void Update_Displays(void) {
    // --- 数码管1: 电梯的【实际】位置 (始终常亮) ---
    displayInteger(DISP_ELEVATOR, g_elevator_current_floor);

    // --- 数码管2: 【用户所在】楼层 ---
    if (g_system_state == STATE_SELECT_CURRENT) {
        // 正在选择 "用户楼层" -> 闪烁显示 "temp" 值
        if (g_flash_state) {
            displayInteger(DISP_USER, g_temp_select_floor);
        } else {
            ledSegDispClear(DISP_USER);
        }
    } else {
        // 其他状态 -> 常亮显示 "已确认" 的 g_user_selected_floor
        displayInteger(DISP_USER, g_user_selected_floor);
    }

    // --- 数码管3: 【目标】楼层 ---
    if (g_system_state == STATE_SELECT_TARGET) {
        // Z正在选择 "目标楼层" -> 闪烁显示 "temp" 值
        if (g_flash_state) {
            displayInteger(DISP_TARGET, g_temp_select_floor);
        } else {
            ledSegDispClear(DISP_TARGET);
        }
    } else {
        // 其他状态 -> 常亮显示 "已确认" 的 g_user_target_floor
        displayInteger(DISP_TARGET, g_user_target_floor);
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
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
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
