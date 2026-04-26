/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Elevator Demo - 模拟电梯控制系统
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
#include <stdbool.h>
#include <buzzer.h>
#include <ledsegdisp.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum SystemState {
    STATE_IDLE,
    STATE_SELECT_CURRENT,
    STATE_SELECT_TARGET,
    STATE_MOVING_TO_USER,
    STATE_DOOR_OPEN_USER,
    STATE_MOVING_TO_TARGET,
    STATE_DOOR_OPEN_TARGET,
    STATE_RETURNING_HOME
};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DISP_ELEVATOR 1
#define DISP_USER     2
#define DISP_TARGET   3

#define FLOOR_MAX       15
#define FLOOR_MIN       1
#define FLOOR_HOME      1

#define DOOR_WAIT_SECONDS 3
#define BUZZER_FREQ 40
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
volatile enum SystemState g_system_state = STATE_IDLE;
volatile int8_t g_elevator_current_floor = FLOOR_HOME;
volatile int8_t g_user_selected_floor = FLOOR_HOME;
volatile int8_t g_user_target_floor = FLOOR_HOME;
volatile int8_t g_temp_select_floor;
volatile uint8_t g_wait_counter = 0;
volatile bool g_flash_state = false;
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
        Update_LEDs();
        Update_Displays();
        HAL_Delay(50);
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
    if (htim->Instance == TIM6) {
        g_flash_state = !g_flash_state;

        switch (g_system_state) {
            case STATE_MOVING_TO_USER:
                if (g_elevator_current_floor < g_user_selected_floor) g_elevator_current_floor++;
                else if (g_elevator_current_floor > g_user_selected_floor) g_elevator_current_floor--;
                else {
                    g_system_state = STATE_DOOR_OPEN_USER;
                    g_wait_counter = DOOR_WAIT_SECONDS;
                    Buzzer_On();
                }
                break;

            case STATE_DOOR_OPEN_USER:
                Buzzer_Off();
                if (g_wait_counter > 0) g_wait_counter--;
                else {
                    g_system_state = STATE_MOVING_TO_TARGET;
                }
                break;

            case STATE_MOVING_TO_TARGET:
                if (g_elevator_current_floor < g_user_target_floor) g_elevator_current_floor++;
                else if (g_elevator_current_floor > g_user_target_floor) g_elevator_current_floor--;
                else {
                    g_system_state = STATE_DOOR_OPEN_TARGET;
                    g_wait_counter = DOOR_WAIT_SECONDS;
                    Buzzer_On();
                }
                break;

            case STATE_DOOR_OPEN_TARGET:
                Buzzer_Off();
                if (g_wait_counter > 0) g_wait_counter--;
                else {
                    if (g_elevator_current_floor == FLOOR_HOME) {
                        g_system_state = STATE_IDLE;
                    } else {
                        g_system_state = STATE_RETURNING_HOME;
                    }
                }
                break;

            case STATE_RETURNING_HOME:
                if (g_elevator_current_floor < FLOOR_HOME) g_elevator_current_floor++;
                else if (g_elevator_current_floor > FLOOR_HOME) g_elevator_current_floor--;
                else {
                    g_system_state = STATE_IDLE;
                }
                break;

            case STATE_IDLE:
            case STATE_SELECT_CURRENT:
            case STATE_SELECT_TARGET:
            default:
                break;
        }
    }
}

void HAL_GPIO_EXTI_Callback(const uint16_t GPIO_Pin) {
    switch (GPIO_Pin) {
        case RIGHT_KEY_Pin:
            if (g_system_state == STATE_IDLE) {
                g_system_state = STATE_SELECT_CURRENT;
                g_temp_select_floor = g_elevator_current_floor;
            } else if (g_system_state == STATE_SELECT_CURRENT) {
                g_user_selected_floor = g_temp_select_floor;
                g_system_state = STATE_SELECT_TARGET;
                g_temp_select_floor = g_user_target_floor;
            } else if (g_system_state == STATE_SELECT_TARGET) {
                g_user_target_floor = g_temp_select_floor;
                g_system_state = STATE_SELECT_CURRENT;
                g_temp_select_floor = g_user_selected_floor;
            } else if (g_system_state == STATE_DOOR_OPEN_TARGET) {
                g_wait_counter = 0;
                g_system_state = STATE_SELECT_CURRENT;
                g_temp_select_floor = g_elevator_current_floor;
            }
            break;

        case LEFT_KEY_Pin:
            if (g_system_state == STATE_SELECT_TARGET) {
                g_user_target_floor = g_temp_select_floor;

                if (g_user_target_floor == g_user_selected_floor) {
                    g_system_state = STATE_IDLE;
                    break;
                }

                if (g_elevator_current_floor == g_user_selected_floor) {
                    g_system_state = STATE_DOOR_OPEN_USER;
                    g_wait_counter = DOOR_WAIT_SECONDS;
                } else {
                    g_system_state = STATE_MOVING_TO_USER;
                }
            }
            break;

        case UP_KEY_Pin:
            if (g_system_state == STATE_SELECT_CURRENT || g_system_state == STATE_SELECT_TARGET) {
                if (g_temp_select_floor < FLOOR_MAX) {
                    g_temp_select_floor++;
                }
            }
            break;

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

static void Update_LEDs(void) {
    HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(YEL_GPIO_Port, YEL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GRE_GPIO_Port, GRE_Pin, GPIO_PIN_RESET);

    switch (g_system_state) {
        case STATE_MOVING_TO_USER:
        case STATE_MOVING_TO_TARGET:
        case STATE_RETURNING_HOME:
            HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_SET);
            break;
        case STATE_DOOR_OPEN_USER:
            HAL_GPIO_WritePin(YEL_GPIO_Port, YEL_Pin, GPIO_PIN_SET);
            break;
        case STATE_DOOR_OPEN_TARGET:
            HAL_GPIO_WritePin(GRE_GPIO_Port, GRE_Pin, GPIO_PIN_SET);
            break;
        case STATE_IDLE:
        case STATE_SELECT_CURRENT:
        case STATE_SELECT_TARGET:
        default:
            break;
    }
}

static void Update_Displays(void) {
    displayInteger(DISP_ELEVATOR, g_elevator_current_floor);

    if (g_system_state == STATE_SELECT_CURRENT) {
        if (g_flash_state) {
            displayInteger(DISP_USER, g_temp_select_floor);
        } else {
            ledSegDispClear(DISP_USER);
        }
    } else {
        displayInteger(DISP_USER, g_user_selected_floor);
    }

    if (g_system_state == STATE_SELECT_TARGET) {
        if (g_flash_state) {
            displayInteger(DISP_TARGET, g_temp_select_floor);
        } else {
            ledSegDispClear(DISP_TARGET);
        }
    } else {
        displayInteger(DISP_TARGET, g_user_target_floor);
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
