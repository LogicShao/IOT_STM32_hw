//
// Created by Padhia on 2025/10/28.
// Contact me at: luochj2025@lzu.edu.cn
//

#include "ht7017.h"

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief  计算校验和: ~(sum of all bytes)
 *         手册 §4.1.5: CHKSUM = ~(HEAD + CMD + DATAn + ... + DATA1)
 */
static uint8_t HT7017_CalcChecksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint8_t)(~sum);
}

/**
 * @brief  底层写寄存器 (不含写保护逻辑, 供内部使用)
 *         帧格式: HEAD + CMD(W) + DATA1 + DATA0 + CHECKSUM → ACK
 *         手册 §4.1.6: 写操作固定 6 字节 (5 发送 + 1 ACK)
 */
static HAL_StatusTypeDef HT7017_WriteReg_Internal(HT7017_HandleTypeDef *hdev,
                                                   uint8_t reg_addr,
                                                   uint16_t val)
{
    uint8_t tx_buf[5];  // HEAD + CMD + DATA1 + DATA0 + CHECKSUM
    uint8_t ack_byte;

    // 构造写命令帧
    tx_buf[0] = HT7017_FRAME_HEAD;
    tx_buf[1] = (reg_addr & HT7017_READ_CMD_MASK) | HT7017_WRITE_CMD_MASK;
    tx_buf[2] = (uint8_t)((val >> 8) & 0xFF);  // DATA1 (高字节)
    tx_buf[3] = (uint8_t)(val & 0xFF);          // DATA0 (低字节)
    tx_buf[4] = HT7017_CalcChecksum(tx_buf, 4);

    // 发送 5 字节
    if (HAL_UART_Transmit(hdev->huart, tx_buf, 5, HT7017_TX_TIMEOUT) != HAL_OK) {
        return HAL_ERROR;
    }

    // 接收 ACK
    if (HAL_UART_Receive(hdev->huart, &ack_byte, 1, HT7017_RX_TIMEOUT) != HAL_OK) {
        return HAL_TIMEOUT;
    }

    return (ack_byte == HT7017_ACK_OK) ? HAL_OK : HAL_ERROR;
}

// ============================================================================
// Public API
// ============================================================================

HAL_StatusTypeDef HT7017_Init(HT7017_HandleTypeDef *hdev,
                              UART_HandleTypeDef *huart,
                              uint16_t adccon_val)
{
    if (hdev == NULL || huart == NULL) {
        return HAL_ERROR;
    }

    hdev->huart     = huart;
    hdev->I1_Sample = 0;
    hdev->I2_Sample = 0;
    hdev->U_Sample  = 0;

    // ---------------------------------------------------------------
    // 计量可靠性机制
    // "上电启动或发生复位后, 用户需要对 ADC 通道增益寄存器(59H ADCCON)
    //  进行一次写操作后, 能量才会计量。"
    //
    // 必须先开启写保护(ADCCON 属于 50H-7CH 范围), 再写 ADCCON
    // ---------------------------------------------------------------
    HAL_StatusTypeDef status;

    status = HT7017_WriteEnable(hdev, HT7017_WPCFG_ENABLE_CAL);
    if (status != HAL_OK) return status;

    status = HT7017_WriteReg_Internal(hdev, HT7017_REG_ADCCON, adccon_val);
    if (status != HAL_OK) return status;

    status = HT7017_WriteDisable(hdev);
    return status;
}

HAL_StatusTypeDef HT7017_WriteEnable(HT7017_HandleTypeDef *hdev, uint8_t key) {
    if (hdev == NULL) return HAL_ERROR;
    if (key != HT7017_WPCFG_ENABLE_CAL && key != HT7017_WPCFG_ENABLE_CFG) {
        return HAL_ERROR;
    }
    // WPCFG(32H) 是单字节寄存器, 高字节补 0
    return HT7017_WriteReg_Internal(hdev, HT7017_REG_WPCFG, (uint16_t)key);
}

HAL_StatusTypeDef HT7017_WriteDisable(HT7017_HandleTypeDef *hdev) {
    if (hdev == NULL) return HAL_ERROR;
    return HT7017_WriteReg_Internal(hdev, HT7017_REG_WPCFG, HT7017_WPCFG_DISABLE);
}

HAL_StatusTypeDef HT7017_ReadReg(HT7017_HandleTypeDef *hdev,
                                 uint8_t reg_addr,
                                 uint32_t *pVal)
{
    if (hdev == NULL || pVal == NULL) return HAL_ERROR;

    uint8_t tx_buf[2];
    uint8_t rx_buf[4];  // DATA2 + DATA1 + DATA0 + CHECKSUM

    // 构造读命令帧: HEAD + CMD(R)
    tx_buf[0] = HT7017_FRAME_HEAD;
    tx_buf[1] = reg_addr & HT7017_READ_CMD_MASK;

    // 发送 2 字节
    if (HAL_UART_Transmit(hdev->huart, tx_buf, 2, HT7017_TX_TIMEOUT) != HAL_OK) {
        return HAL_ERROR;
    }

    // 接收 4 字节: DATA2(H) + DATA1(M) + DATA0(L) + CHECKSUM
    if (HAL_UART_Receive(hdev->huart, rx_buf, 4, HT7017_RX_TIMEOUT) != HAL_OK) {
        return HAL_TIMEOUT;
    }

    // 校验 Checksum
    // CHKSUM = ~(HEAD + CMD + DATA2 + DATA1 + DATA0)
    uint8_t csum_input[5];
    csum_input[0] = tx_buf[0];  // HEAD
    csum_input[1] = tx_buf[1];  // CMD
    csum_input[2] = rx_buf[0];  // DATA2 (High)
    csum_input[3] = rx_buf[1];  // DATA1 (Mid)
    csum_input[4] = rx_buf[2];  // DATA0 (Low)

    uint8_t expected_csum = HT7017_CalcChecksum(csum_input, 5);
    if (expected_csum != rx_buf[3]) {
        return HAL_ERROR;  // 校验和错误
    }

    // 组装 24-bit 数据 (大端: 高字节在前)
    *pVal = ((uint32_t)rx_buf[0] << 16) |
            ((uint32_t)rx_buf[1] << 8)  |
            (uint32_t)rx_buf[2];

    return HAL_OK;
}

HAL_StatusTypeDef HT7017_WriteReg(HT7017_HandleTypeDef *hdev,
                                  uint8_t reg_addr,
                                  uint16_t val)
{
    if (hdev == NULL) return HAL_ERROR;
    return HT7017_WriteReg_Internal(hdev, reg_addr, val);
}

HAL_StatusTypeDef HT7017_GetSignedSample(HT7017_HandleTypeDef *hdev,
                                         uint8_t reg_addr,
                                         int32_t *pSignedVal)
{
    if (hdev == NULL || pSignedVal == NULL) return HAL_ERROR;

    uint32_t raw_val = 0;
    HAL_StatusTypeDef status = HT7017_ReadReg(hdev, reg_addr, &raw_val);
    if (status != HAL_OK) return status;

    // 24-bit 补码符号扩展到 32-bit
    if (raw_val & 0x800000) {
        *pSignedVal = (int32_t)(raw_val | 0xFF000000);
    } else {
        *pSignedVal = (int32_t)raw_val;
    }

    // 缓存采样数据 (仅限 SPL 寄存器)
    if (reg_addr == HT7017_REG_SPL_I1) {
        hdev->I1_Sample = raw_val;
    } else if (reg_addr == HT7017_REG_SPL_I2) {
        hdev->I2_Sample = raw_val;
    } else if (reg_addr == HT7017_REG_SPL_U) {
        hdev->U_Sample = raw_val;
    }

    return HAL_OK;
}

HAL_StatusTypeDef HT7017_SoftReset(HT7017_HandleTypeDef *hdev) {
    if (hdev == NULL) return HAL_ERROR;
    // 向 SRSTREG(33H) 写入 0x55 触发软件复位
    // 注意: 复位后需等待 20ms, 且必须重新调用 HT7017_Init
    return HT7017_WriteReg_Internal(hdev, HT7017_REG_SRSTREG, 0x0055);
}