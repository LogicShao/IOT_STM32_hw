/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BEEP_Pin GPIO_PIN_2
#define BEEP_GPIO_Port GPIOA
#define YEL_Pin GPIO_PIN_7
#define YEL_GPIO_Port GPIOA
#define GRE_Pin GPIO_PIN_0
#define GRE_GPIO_Port GPIOB
#define RED_Pin GPIO_PIN_1
#define RED_GPIO_Port GPIOB
#define SMG3_DAT_Pin GPIO_PIN_2
#define SMG3_DAT_GPIO_Port GPIOB
#define SMG3_CLK_Pin GPIO_PIN_10
#define SMG3_CLK_GPIO_Port GPIOB
#define SMG2_DAT_Pin GPIO_PIN_12
#define SMG2_DAT_GPIO_Port GPIOB
#define SMG2_CLK_Pin GPIO_PIN_13
#define SMG2_CLK_GPIO_Port GPIOB
#define SMG1_DAT_Pin GPIO_PIN_14
#define SMG1_DAT_GPIO_Port GPIOB
#define SMG1_CLK_Pin GPIO_PIN_15
#define SMG1_CLK_GPIO_Port GPIOB
#define TEST_Pin GPIO_PIN_11
#define TEST_GPIO_Port GPIOA
#define DOWN_KEY_Pin GPIO_PIN_15
#define DOWN_KEY_GPIO_Port GPIOA
#define DOWN_KEY_EXTI_IRQn EXTI4_15_IRQn
#define LEFT_KEY_Pin GPIO_PIN_3
#define LEFT_KEY_GPIO_Port GPIOB
#define LEFT_KEY_EXTI_IRQn EXTI2_3_IRQn
#define UP_KEY_Pin GPIO_PIN_4
#define UP_KEY_GPIO_Port GPIOB
#define UP_KEY_EXTI_IRQn EXTI4_15_IRQn
#define RIGHT_KEY_Pin GPIO_PIN_5
#define RIGHT_KEY_GPIO_Port GPIOB
#define RIGHT_KEY_EXTI_IRQn EXTI4_15_IRQn
#define DIR485_Pin GPIO_PIN_8
#define DIR485_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
