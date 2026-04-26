//
// Created by Padhia on 2025/10/26.
// Contact me at: luochj2025@lzu.edu.cn
//

// How to use RS485 communication with transmit and receive mode control.
/**
  *  Use RS485_Set_Transmit_Mode() to set RS485 to transmit mode.
  *
  *  Use RS485_Set_Receive_Mode() to set RS485 to receive mode.
  *
  *  Use RS485_Transmit(pData, Size) to transmit data over RS485.
  *
  *  For example:
  *  If you want to send "Hello" over RS485, you can do:
  *  data [] = "Hello";
  *  RS485_Transmit(data, sizeof(data)-1);
  */

#include "rs485.h"

/**
  * @brief Set RS485 to Transmit Mode
  * @param None
  * @return None
  */
void RS485_Set_Transmit_Mode() {
    // Set DE pin low to enable transmit mode
    HAL_GPIO_WritePin(DIR485_GPIO_Port, DIR485_Pin, GPIO_PIN_RESET);
}

/**
  * @brief Set RS485 to Receive Mode
  * @param None
  * @return None
  */
void RS485_Set_Receive_Mode() {
    // Set DE pin high to enable receive mode
    HAL_GPIO_WritePin(DIR485_GPIO_Port, DIR485_Pin, GPIO_PIN_SET);
}

/**
  * @brief Transmit data over RS485
  * @param pData Pointer to data buffer
  * @param Size Size of data to transmit
  * @return HAL status
  */
HAL_StatusTypeDef RS485_Transmit(const uint8_t *pData, const uint16_t Size) {
    RS485_Set_Transmit_Mode();  // Set to transmit mode
    const HAL_StatusTypeDef status = HAL_UART_Transmit(&RS485_USART, pData, Size, HAL_MAX_DELAY);
    RS485_Set_Receive_Mode();   // Set back to receive mode
    return status;
}
