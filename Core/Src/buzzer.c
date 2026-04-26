//
// Created by Padhia on 2025/10/24.
// Contact me at: luochj2025@lzu.edu.cn
//

#include "buzzer.h"

// How to use this module:
/**
  *  For example:
  *  First call Buzzer_Init() to initialize,
  *  then call Buzzer_Set_Frequency(10), to set frequency to 1kHz.
  *
  *  If you want a 2kHz tone, call Buzzer_Set_Frequency(20);
  *  then call Buzzer_On() to start the tone.
  *
  *  If you want to stop the tone, call Buzzer_Off().
  */

/* -------------------------------------------------------------------------- */
/* Module Private Definitions                                                 */
/* -------------------------------------------------------------------------- */

// Based on 48MHz clock and Prescaler (PSC) = 47
// Timer Count Frequency = 48,000,000 / (47 + 1) = 1,000,000 Hz
#define TIMER_COUNT_FREQ_HZ     1000000

// Define 0% pulse as 0
#define BUZZER_PULSE_0_PERCENT  0

/* -------------------------------------------------------------------------- */
/* Public API Functions                                                       */
/* -------------------------------------------------------------------------- */

/** @brief Initialize the buzzer module
 * @param None
 * @return HAL status
 */
HAL_StatusTypeDef Buzzer_Init(void) {
    // This starts the PWM signal generation
    return HAL_TIM_PWM_Start(&BUZZER_TIM_HANDLE, BUZZER_TIM_CHANNEL);
}

/** @brief De-initialize the buzzer module
 * @param None
 * @return HAL status
 */
HAL_StatusTypeDef Buzzer_DeInit(void) {
    // This stops the PWM signal generation
    return HAL_TIM_PWM_Stop(&BUZZER_TIM_HANDLE, BUZZER_TIM_CHANNEL);
}

/** @brief Turn the buzzer on (at the CURRENTLY set frequency)
 * @param None
 * @return None
 */
void Buzzer_On(void) {
    // This function already correctly calculates 50% of the CURRENT period (ARR).

    // Get the timer's current Period (ARR) value
    const uint32_t current_arr = __HAL_TIM_GET_AUTORELOAD(&BUZZER_TIM_HANDLE);

    // Calculate the 50% duty cycle Pulse (CCR) value
    const uint32_t pulse_50_percent = (current_arr + 1) / 2;

    // Set the compare register to the new 50% pulse
    __HAL_TIM_SET_COMPARE(&BUZZER_TIM_HANDLE, BUZZER_TIM_CHANNEL, pulse_50_percent);
}

/** @brief Turn the buzzer off
 * @param None
 * @return None
 */
void Buzzer_Off(void) {
    // Setting the pulse to 0 is always 0% duty cycle
    __HAL_TIM_SET_COMPARE(&BUZZER_TIM_HANDLE, BUZZER_TIM_CHANNEL, BUZZER_PULSE_0_PERCENT);
}


/** @brief Set the buzzer frequency
 * @brief It maintains the current state (On/Off) at the new frequency.
 * @param Freq The frequency in 0.1kHz steps (e.g., 10 for 1kHz, 40 for 4kHz)
 * @return None
 */
void Buzzer_Set_Frequency(const uint8_t Freq)
{
    // Do not allow setting frequency to 0
    if (Freq == 0)
    {
        // If user wants to turn off, they must call Buzzer_Off()
        return;
    }

    // Calculate the real frequency in Hz
    const uint32_t real_frequency_hz = (uint32_t)Freq * 100;

    // Calculate new ARR (Period) value
    const uint32_t new_arr = (TIMER_COUNT_FREQ_HZ / real_frequency_hz) - 1;

    // Check if the buzzer was ON before changing frequency
    // We read the current Compare Register (Pulse)
    const uint32_t current_pulse = __HAL_TIM_GET_COMPARE(&BUZZER_TIM_HANDLE, BUZZER_TIM_CHANNEL);
    const bool was_on = (current_pulse > 0);

    // Apply the new Period (ARR) to the timer
    __HAL_TIM_SET_AUTORELOAD(&BUZZER_TIM_HANDLE, new_arr);

    // --- CRITICAL STEP ---
    // If the buzzer was ON, we must update the Pulse (CCR) to match
    // the new period to maintain the 50% duty cycle.
    if (was_on)
    {
        const uint32_t new_pulse_50_percent = (new_arr + 1) / 2;
        __HAL_TIM_SET_COMPARE(&BUZZER_TIM_HANDLE, BUZZER_TIM_CHANNEL, new_pulse_50_percent);
    }
    // If it was OFF (current_pulse == 0), we do nothing,
    // so it STAYS OFF at the new frequency.
}