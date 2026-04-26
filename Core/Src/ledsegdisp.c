//
// Created by Padhia on 2025/10/22.
// Contact me at: luochj2025@lzu.edu.cn
//
// Merged: gn1650_i2c (bit-bang I2C) + ledsegdisp (7-segment display driver)
// The GN1650 I2C primitives are internal (static) to this module.
//

// How to use this module:
/**
  *  For example:
  *  First call ledSegDispOpen(dev_number) to initialize the display,
  *  second call ledSegDispClear(dev_number) to clear the display.
  *
  *  If you want to set DIG1 of device 1 to show number '5' without dot,
  *  call ledSegDispShowSingleNum(1, 1, 5, false);
  *
  *  However, we do not recommend calling these low-level functions directly.
  *
  *  Instead, use the higher-level functions like displayInteger() and displayFloat().
  *
  *  If you want to display an integer number (e.g., -123, 4567) on device 2,
  *  just call displayInteger(2, your_number);
  *  the number should be in the range -999 to 9999.
  *
  *  If you want to display a floating-point number (e.g., -12.3, 5.678) on device 3,
  *  just call displayFloat(3, your_float);
  */

#include "ledsegdisp.h"

/* ========================================================================== */
/* Module Private Definitions                                                 */
/* ========================================================================== */

// Segment data for 7-segment digits 0-9 (Common Anode)
static const uint8_t seg_num[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

// Segment data for sign '-'
static const uint8_t SEG_SIGN = 0x40;

// Segment data for dot '.' (used for bitwise OR)
static const uint8_t SEG_DOT  = 0x80;

// Lookup table replacing pow(10, n) for n = 0..4
static const uint16_t pow10_lut[5] = {1, 10, 100, 1000, 10000};

/* ========================================================================== */
/* GN1650 Bit-Bang I2C — GPIO Pin Definitions (Private)                      */
/* ========================================================================== */

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} GPIOMap;

// GPIO Pin mapping arrays for 3 GN1650 devices
static const GPIOMap SMGx_CLK[3] = {
    {SMG1_CLK_GPIO_Port, SMG1_CLK_Pin},
    {SMG2_CLK_GPIO_Port, SMG2_CLK_Pin},
    {SMG3_CLK_GPIO_Port, SMG3_CLK_Pin}
};

static const GPIOMap SMGx_DAT[3] = {
    {SMG1_DAT_GPIO_Port, SMG1_DAT_Pin},
    {SMG2_DAT_GPIO_Port, SMG2_DAT_Pin},
    {SMG3_DAT_GPIO_Port, SMG3_DAT_Pin}
};

/* ========================================================================== */
/* GN1650 Bit-Bang I2C — Private Helper Functions                            */
/* ========================================================================== */

/**
  * @brief i2c delay.
  * @note  You MUST tune the loop count for your specific MCU clock speed
  *        to meet the I2C timing requirements (e.g., ~4.7us for 100kHz I2C).
  *        Current value is tuned for 48 MHz with -Os optimization.
  */
static void i2cDelay(void) {
    for (volatile uint32_t i = 0; i < 10; i++);
}

/**
  * @brief i2c Start Condition
  */
static void i2cStart(const GPIOMap *clk, const GPIOMap *dat) {
    HAL_GPIO_WritePin(dat->port, dat->pin, GPIO_PIN_SET);   // Release DAT
    HAL_GPIO_WritePin(clk->port, clk->pin, GPIO_PIN_SET);   // Release SCL
    i2cDelay();
    HAL_GPIO_WritePin(dat->port, dat->pin, GPIO_PIN_RESET); // Pull DAT low
    i2cDelay();
    HAL_GPIO_WritePin(clk->port, clk->pin, GPIO_PIN_RESET); // Pull SCL low
    i2cDelay();
}

/**
  * @brief i2c Stop Condition
  */
static void i2cStop(const GPIOMap *clk, const GPIOMap *dat) {
    HAL_GPIO_WritePin(clk->port, clk->pin, GPIO_PIN_RESET); // Pull SCL low
    HAL_GPIO_WritePin(dat->port, dat->pin, GPIO_PIN_RESET); // Pull DAT low
    i2cDelay();
    HAL_GPIO_WritePin(clk->port, clk->pin, GPIO_PIN_SET);   // Release SCL
    i2cDelay();
    HAL_GPIO_WritePin(dat->port, dat->pin, GPIO_PIN_SET);   // Release DAT
    i2cDelay();
}

/**
  * @brief Get ACK from slave
  * @retval HAL_OK if ACK received, HAL_ERROR if NACK
  */
static HAL_StatusTypeDef i2cGetAck(const GPIOMap *clk, const GPIOMap *dat) {
    HAL_GPIO_WritePin(clk->port, clk->pin, GPIO_PIN_RESET); // SCL low
    HAL_GPIO_WritePin(dat->port, dat->pin, GPIO_PIN_SET);    // Release DAT
    i2cDelay();
    HAL_GPIO_WritePin(clk->port, clk->pin, GPIO_PIN_SET);   // SCL high
    i2cDelay();

    GPIO_PinState ack = HAL_GPIO_ReadPin(dat->port, dat->pin);

    HAL_GPIO_WritePin(clk->port, clk->pin, GPIO_PIN_RESET); // SCL low
    i2cDelay();

    return (ack == GPIO_PIN_RESET) ? HAL_OK : HAL_ERROR;
}

/**
  * @brief Send a byte via i2c (MSB first)
  * @retval HAL_OK if ACK received after byte, HAL_ERROR otherwise
  */
static HAL_StatusTypeDef i2cSendByte(const GPIOMap *clk, const GPIOMap *dat, uint8_t data) {
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(clk->port, clk->pin, GPIO_PIN_RESET);

        if (data & 0x80) {
            HAL_GPIO_WritePin(dat->port, dat->pin, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(dat->port, dat->pin, GPIO_PIN_RESET);
        }

        i2cDelay();
        HAL_GPIO_WritePin(clk->port, clk->pin, GPIO_PIN_SET);
        i2cDelay();

        data <<= 1;
    }

    HAL_GPIO_WritePin(clk->port, clk->pin, GPIO_PIN_RESET);
    i2cDelay();

    return i2cGetAck(clk, dat);
}

/* ========================================================================== */
/* Display Internal Helpers                                                   */
/* ========================================================================== */

/**
  * @brief  Validate device number and return pointers to CLK/DAT GPIOMap.
  * @retval HAL_OK if valid, HAL_ERROR if out of range
  */
static HAL_StatusTypeDef getDevPins(const uint8_t dev_number,
                                    const GPIOMap **clk,
                                    const GPIOMap **dat)
{
    if (dev_number < 1 || dev_number > 3) {
        return HAL_ERROR;
    }
    *clk = &SMGx_CLK[dev_number - 1];
    *dat = &SMGx_DAT[dev_number - 1];
    return HAL_OK;
}

/**
  * @brief  Send a 2-byte I2C transaction (address + data) with full error checking.
  *         Always issues a STOP condition regardless of success or failure.
  * @param  dev_number Device number (1-3)
  * @param  address    GN1650 register/digit address byte
  * @param  data       Segment data byte
  * @retval HAL_OK on success, HAL_ERROR on any I2C failure
  */
static HAL_StatusTypeDef ledSegDispSendData(const uint8_t dev_number,
                                            const uint8_t address,
                                            const uint8_t data)
{
    const GPIOMap *clk;
    const GPIOMap *dat;

    if (getDevPins(dev_number, &clk, &dat) != HAL_OK) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status;

    i2cStart(clk, dat);

    status = i2cSendByte(clk, dat, address);
    if (status != HAL_OK) {
        i2cStop(clk, dat);
        return HAL_ERROR;
    }

    status = i2cSendByte(clk, dat, data);
    if (status != HAL_OK) {
        i2cStop(clk, dat);
        return HAL_ERROR;
    }

    i2cStop(clk, dat);
    return HAL_OK;
}

/**
  * @brief  Calculate the GN1650 digit address from position index.
  * @param  DIGx Position (1-4)
  * @retval Address byte: 1->0x68, 2->0x6A, 3->0x6C, 4->0x6E
  */
static inline uint8_t digitAddress(const uint8_t DIGx) {
    return 0x68 + (DIGx - 1) * 2;
}

/* ========================================================================== */
/* Public API Functions                                                       */
/* ========================================================================== */

HAL_StatusTypeDef ledSegDispShowBlank(const uint8_t dev_number, const uint8_t DIGx) {
    if (DIGx < 1 || DIGx > 4) return HAL_ERROR;
    return ledSegDispSendData(dev_number, digitAddress(DIGx), 0x00);
}

HAL_StatusTypeDef ledSegDispShowSign(const uint8_t dev_number, const uint8_t DIGx) {
    if (DIGx < 1 || DIGx > 4) return HAL_ERROR;
    return ledSegDispSendData(dev_number, digitAddress(DIGx), SEG_SIGN);
}

HAL_StatusTypeDef ledSegDispShowSingleNum(const uint8_t dev_number,
                                          const uint8_t DIGx,
                                          const uint8_t number,
                                          const bool show_dot)
{
    if (DIGx < 1 || DIGx > 4) return HAL_ERROR;
    if (number > 9)            return HAL_ERROR;

    uint8_t data_to_send = seg_num[number];
    if (show_dot) {
        data_to_send |= SEG_DOT;
    }

    return ledSegDispSendData(dev_number, digitAddress(DIGx), data_to_send);
}

HAL_StatusTypeDef ledSegDispOpen(const uint8_t dev_number) {
    // GN1650 system command: address 0x48, data 0x01 = display ON
    return ledSegDispSendData(dev_number, 0x48, 0x01);
}

HAL_StatusTypeDef ledSegDispClear(const uint8_t dev_number) {
    for (uint8_t pos = 1; pos <= 4; pos++) {
        HAL_StatusTypeDef status = ledSegDispShowBlank(dev_number, pos);
        if (status != HAL_OK) {
            return status;
        }
    }
    return HAL_OK;
}

HAL_StatusTypeDef displayInteger(const uint8_t dev_number, int number) {
    // Range check
    if (number > 9999 || number < -999) {
        ledSegDispShowSign(dev_number, 1);
        ledSegDispShowSign(dev_number, 2);
        ledSegDispShowSign(dev_number, 3);
        ledSegDispShowSign(dev_number, 4);
        return HAL_ERROR;
    }

    const bool is_negative = (number < 0);

    if (is_negative) {
        number = -number;

        // 负号紧跟最高位数字, 右对齐:
        //   -X   →  _ _ - X    (sign at 3)
        //   -XX  →  _ - X X    (sign at 2)
        //   -XXX →  - X X X    (sign at 1)

        const uint8_t d2 = (number / 100) % 10;
        const uint8_t d3 = (number / 10) % 10;
        const uint8_t d4 = number % 10;

        if (number >= 100) {
            ledSegDispShowSign(dev_number, 1);
            ledSegDispShowSingleNum(dev_number, 2, d2, false);
            ledSegDispShowSingleNum(dev_number, 3, d3, false);
            ledSegDispShowSingleNum(dev_number, 4, d4, false);
        } else if (number >= 10) {
            ledSegDispShowBlank(dev_number, 1);
            ledSegDispShowSign(dev_number, 2);
            ledSegDispShowSingleNum(dev_number, 3, d3, false);
            ledSegDispShowSingleNum(dev_number, 4, d4, false);
        } else {
            ledSegDispShowBlank(dev_number, 1);
            ledSegDispShowBlank(dev_number, 2);
            ledSegDispShowSign(dev_number, 3);
            ledSegDispShowSingleNum(dev_number, 4, d4, false);
        }
    } else {
        // 正数: 4 位, 前导零抑制, 右对齐
        const uint8_t d1 = (number / 1000) % 10;
        const uint8_t d2 = (number / 100) % 10;
        const uint8_t d3 = (number / 10) % 10;
        const uint8_t d4 = number % 10;
        bool show_digit = false;

        if (d1 > 0) {
            ledSegDispShowSingleNum(dev_number, 1, d1, false);
            show_digit = true;
        } else {
            ledSegDispShowBlank(dev_number, 1);
        }

        if (d2 > 0 || show_digit) {
            ledSegDispShowSingleNum(dev_number, 2, d2, false);
            show_digit = true;
        } else {
            ledSegDispShowBlank(dev_number, 2);
        }

        if (d3 > 0 || show_digit) {
            ledSegDispShowSingleNum(dev_number, 3, d3, false);
            show_digit = true;
        } else {
            ledSegDispShowBlank(dev_number, 3);
        }

        ledSegDispShowSingleNum(dev_number, 4, d4, false);
    }

    return HAL_OK;
}

/**
 * @brief  Displays a float with caller-specified decimal places, right-aligned.
 *
 * Examples (positive):
 *   displayFloat(dev, 1.234, 3) →  1.234    (fills all 4)
 *   displayFloat(dev, 1.234, 2) →  _ 1.23
 *   displayFloat(dev, 1.234, 1) →  _ _ 1.2
 *   displayFloat(dev, 1.234, 0) →  _ _ _ 1
 *   displayFloat(dev, 123.4, 1) →  123.4
 *
 * Examples (negative, sign always adjacent to highest digit):
 *   displayFloat(dev, -1.23, 2) →  -1.23
 *   displayFloat(dev, -1.2,  1) →  _ -1.2
 *   displayFloat(dev, -5.0,  0) →  _ _ -5
 *
 * @param dev_number Device number (1-3)
 * @param number     The float to display
 * @param decimals   Number of decimal places (0-3 positive, 0-2 negative).
 *                   Automatically clamped if too large.
 * @retval HAL_OK on success, HAL_ERROR on overflow or invalid param
 */
HAL_StatusTypeDef displayFloat(const uint8_t dev_number, float number, uint8_t decimals) {
    const bool is_negative = (number < 0);
    const float abs_num = is_negative ? -number : number;

    // 可用数字位数 (负数需留 1 位给负号)
    const uint8_t avail = is_negative ? 3 : 4;

    // 限制小数位数: 至少保留 1 位整数
    if (decimals >= avail) {
        decimals = avail - 1;
    }

    // 缩放、四舍五入
    const int scaled = (int)(abs_num * (float)pow10_lut[decimals] + 0.5f);

    // 溢出检查: scaled 不能超过 avail 位数字的最大值
    if (scaled >= (int)pow10_lut[avail]) {
        ledSegDispShowSign(dev_number, 1);
        ledSegDispShowSign(dev_number, 2);
        ledSegDispShowSign(dev_number, 3);
        ledSegDispShowSign(dev_number, 4);
        return HAL_ERROR;
    }

    // 提取各位数字 (最多 4 位)
    const uint8_t digits[4] = {
        (scaled / 1000) % 10,
        (scaled / 100) % 10,
        (scaled / 10) % 10,
        scaled % 10
    };

    // 计算需要显示的位数:
    //   至少 decimals + 1 (1 位整数 + 小数部分)
    //   但整数部分可能超过 1 位
    uint8_t num_digits;
    if (scaled >= 1000)     num_digits = 4;
    else if (scaled >= 100) num_digits = 3;
    else if (scaled >= 10)  num_digits = 2;
    else                    num_digits = 1;

    // 至少显示 decimals + 1 位 (保证 "0.xx" 格式正确)
    if (num_digits < decimals + 1) {
        num_digits = decimals + 1;
    }

    // 总占用位数 (含负号)
    const uint8_t total_used = num_digits + (is_negative ? 1 : 0);

    // 起始位置 (右对齐)
    const uint8_t first_pos = 5 - total_used;

    // 小数点所在的显示位置 (0 表示不显示小数点)
    const uint8_t dot_display_pos = (decimals > 0) ? (4 - decimals) : 0;

    // 绘制: 左侧空白
    for (uint8_t p = 1; p < first_pos; p++) {
        ledSegDispShowBlank(dev_number, p);
    }

    // 绘制: 负号 (紧挨最高位数字)
    uint8_t digit_start_pos = first_pos;
    if (is_negative) {
        ledSegDispShowSign(dev_number, first_pos);
        digit_start_pos++;
    }

    // 绘制: 数字 (从 digits[] 数组右侧取 num_digits 个)
    const uint8_t digit_start_idx = 4 - num_digits;
    for (uint8_t i = 0; i < num_digits; i++) {
        const uint8_t pos = digit_start_pos + i;
        const bool show_dot = (pos == dot_display_pos);
        ledSegDispShowSingleNum(dev_number, pos, digits[digit_start_idx + i], show_dot);
    }

    return HAL_OK;
}