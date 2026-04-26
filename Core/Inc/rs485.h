//
// Created by Padhia on 2025/10/26.
// Contact me at: luochj2025@lzu.edu.cn
//

#ifndef RS485_H
#define RS485_H

#include "main.h"
#include "usart.h"

#define RS485_USART huart4  // Define the USART used for RS485 communication

void RS485_Set_Transmit_Mode(void);

void RS485_Set_Receive_Mode(void);

HAL_StatusTypeDef RS485_Transmit(const uint8_t *pData, uint16_t Size);

#endif //RS485_H
