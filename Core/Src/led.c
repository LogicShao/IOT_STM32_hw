//
// Created by Padhia on 2025/10/26.
// Contact me at: luochj2025@lzu.edu.cn
//

#include "led.h"

/* -------------------------------------------------------------------------- */
/* Public API Functions                                                       */
/* -------------------------------------------------------------------------- */

/**
  * @brief Turn on the TEST LED
  * @param None
  * @return None
  */
void TEST_LED_On() {
    HAL_GPIO_WritePin(TEST_GPIO_Port, TEST_Pin, GPIO_PIN_RESET);
}

/**
  * @brief Turn off the TEST LED
  * @param None
  * @return None
  */
void TEST_LED_Off() {
    HAL_GPIO_WritePin(TEST_GPIO_Port, TEST_Pin, GPIO_PIN_SET);
}

/**
  * @brief Toggle the TEST LED
  * @param None
  * @return None
  */
void TEST_LED_Toggle() {
    HAL_GPIO_TogglePin(TEST_GPIO_Port, TEST_Pin);
}

/**
  * @brief Turn on the RED LED
  * @param None
  * @return None
  */
void RED_LED_On() {
    HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_SET);
}

/**
  * @brief Turn off the RED LED
  * @param None
  * @return None
  */
void RED_LED_Off() {
    HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_RESET);
}

/**
  * @brief Toggle the RED LED
  * @param None
  * @return None
  */
void RED_LED_Toggle() {
    HAL_GPIO_TogglePin(RED_GPIO_Port, RED_Pin);
}

/**
  * @brief Turn on the GREEN LED
  * @param None
  * @return None
  */
void GRE_LED_On() {
    HAL_GPIO_WritePin(GRE_GPIO_Port, GRE_Pin, GPIO_PIN_SET);
}

/**
  * @brief Turn off the GREEN LED
  * @param None
  * @return None
  */
void GRE_LED_Off() {
    HAL_GPIO_WritePin(GRE_GPIO_Port, GRE_Pin, GPIO_PIN_RESET);
}

/**
  * @brief Toggle the GREEN LED
  * @param None
  * @return None
  */
void GRE_LED_Toggle() {
    HAL_GPIO_TogglePin(GRE_GPIO_Port, GRE_Pin);
}

/**
  * @brief Turn on the YELLOW LED
  * @param None
  * @return None
  */
void YEL_LED_On() {
    HAL_GPIO_WritePin(YEL_GPIO_Port, YEL_Pin, GPIO_PIN_SET);
}

/**
  * @brief Turn off the YELLOW LED
  * @param None
  * @return None
  */
void YEL_LED_Off() {
    HAL_GPIO_WritePin(YEL_GPIO_Port, YEL_Pin, GPIO_PIN_RESET);
}

/**
  * @brief Toggle the YELLOW LED
  * @param None
  * @return None
  */
void YEL_LED_Toggle() {
    HAL_GPIO_TogglePin(YEL_GPIO_Port, YEL_Pin);
}
