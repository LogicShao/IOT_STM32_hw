//
// Created by Padhia on 2025/10/22.
// Contact me at: luochj2025@lzu.edu.cn
//
// 7-Segment LED Display Driver for GN1650 (up to 3 devices, 4 digits each)
//
// Internal I2C bit-bang implementation is private to ledsegdisp.c.
// This header exposes only the display-level public API.
//

#ifndef LEDSEGDISP_H
#define LEDSEGDISP_H

#include "main.h"
#include <stdbool.h>

/**
 * @brief  Initialize and turn on a 7-segment display device.
 * @param  dev_number Device number (1-3)
 * @retval HAL_OK on success, HAL_ERROR on I2C failure or invalid param
 */
HAL_StatusTypeDef ledSegDispOpen(uint8_t dev_number);

/**
 * @brief  Clear (blank) all 4 digits of a display device.
 * @param  dev_number Device number (1-3)
 * @retval HAL_OK on success, HAL_ERROR on first I2C failure
 */
HAL_StatusTypeDef ledSegDispClear(uint8_t dev_number);

/**
 * @brief  Show a blank (off) segment at a given digit position.
 * @param  dev_number Device number (1-3)
 * @param  DIGx       Digit position (1-4)
 */
HAL_StatusTypeDef ledSegDispShowBlank(uint8_t dev_number, uint8_t DIGx);

/**
 * @brief  Show a minus sign '-' at a given digit position.
 * @param  dev_number Device number (1-3)
 * @param  DIGx       Digit position (1-4)
 */
HAL_StatusTypeDef ledSegDispShowSign(uint8_t dev_number, uint8_t DIGx);

/**
 * @brief  Show a single digit (0-9) at a given position, optionally with dot.
 * @param  dev_number Device number (1-3)
 * @param  DIGx       Digit position (1-4)
 * @param  number     Digit value (0-9)
 * @param  show_dot   true to illuminate the decimal point
 */
HAL_StatusTypeDef ledSegDispShowSingleNum(uint8_t dev_number, uint8_t DIGx,
                                          uint8_t number, bool show_dot);

/**
 * @brief  Display an integer on 4 digits with leading-zero suppression.
 * @param  dev_number Device number (1-3)
 * @param  number     Integer value (-999 to 9999). Out-of-range shows "----".
 */
HAL_StatusTypeDef displayInteger(uint8_t dev_number, int number);

/**
 * @brief  Display a float with specified decimal places, right-aligned.
 *         Negative sign is placed adjacent to the highest digit.
 * @param  dev_number Device number (1-3)
 * @param  number     Float value. Out-of-range shows "----".
 * @param  decimals   Number of decimal places (0-3 positive, 0-2 negative).
 *                    Clamped automatically if too large.
 */
HAL_StatusTypeDef displayFloat(uint8_t dev_number, float number, uint8_t decimals);

#endif // LEDSEGDISP_H