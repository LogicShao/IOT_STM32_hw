//
// Created by Padhia on 2025/10/24.
//

#ifndef BUZZER_H
#define BUZZER_H

#include "main.h"
#include "tim.h"
#include "stdbool.h"

#define BUZZER_TIM_HANDLE htim15
#define BUZZER_TIM_CHANNEL TIM_CHANNEL_1

HAL_StatusTypeDef Buzzer_Init(void);

HAL_StatusTypeDef Buzzer_DeInit(void);

void Buzzer_On(void);

void Buzzer_Off(void);

void Buzzer_Set_Frequency(uint8_t Freq);

#endif //BUZZER_H
