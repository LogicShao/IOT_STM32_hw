//
// Created by Padhia on 2025/10/28.
// Contact me at: luochj2025@lzu.edu.cn
//

#ifndef __HT7017_H__
#define __HT7017_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// ============================================================================
// 计量参数寄存器地址 (Read Only, 参考手册 Page 21, 表5-1)
// ============================================================================
#define HT7017_REG_SPL_I1       0x00  // 电流通道1 ADC采样数据 (3B, signed 19-bit)
#define HT7017_REG_SPL_I2       0x01  // 电流通道2 ADC采样数据 (3B, signed 19-bit)
#define HT7017_REG_SPL_U        0x02  // 电压通道 ADC采样数据   (3B, signed 19-bit)
#define HT7017_REG_I_DC         0x03  // 电流直流均值 (3B, signed)
#define HT7017_REG_U_DC         0x04  // 电压直流均值 (3B, signed)
#define HT7017_REG_RMS_I1       0x06  // 电流通道1 有效值 (3B, unsigned)
#define HT7017_REG_RMS_I2       0x07  // 电流通道2 有效值 (3B, unsigned)
#define HT7017_REG_RMS_U        0x08  // 电压通道 有效值   (3B, unsigned)
#define HT7017_REG_FREQ_U       0x09  // 电压频率 (2B, unsigned)
#define HT7017_REG_POWER_P1     0x0A  // 通道1 有功功率 (3/4B, signed)
#define HT7017_REG_POWER_Q1     0x0B  // 通道1 无功功率 (3/4B, signed)
#define HT7017_REG_POWER_S      0x0C  // 视在功率       (3/4B, unsigned)
#define HT7017_REG_ENERGY_P     0x0D  // 有功能量 (3B)
#define HT7017_REG_ENERGY_Q     0x0E  // 无功能量 (3B)
#define HT7017_REG_POWER_P2     0x10  // 通道2 有功功率 (3/4B, signed)
#define HT7017_REG_POWER_Q2     0x11  // 通道2 无功功率 (3/4B, signed)
#define HT7017_REG_EMUSR        0x19  // EMU 状态寄存器 (2B)
#define HT7017_REG_SYSSTA       0x1A  // 系统状态寄存器 (1B)
#define HT7017_REG_CHIP_ID      0x1B  // ChipID, default 0x7053B0
#define HT7017_REG_DEVICE_ID    0x1C  // DeviceID, default 0x705321

// ============================================================================
// 校表参数寄存器地址 (Read/Write, 参考手册 Page 30, 表5-2)
// ============================================================================
#define HT7017_REG_EMUIE        0x30  // EMU 中断使能 (2B)
#define HT7017_REG_EMUIF        0x31  // EMU 中断标志 (3B)
#define HT7017_REG_WPCFG        0x32  // 写保护寄存器 (1B)
#define HT7017_REG_SRSTREG      0x33  // 软件复位寄存器 (1B)

#define HT7017_REG_EMUCFG       0x40  // EMU 配置寄存器 (2B)
#define HT7017_REG_FREQCFG      0x41  // 时钟/频率配置 (2B)
#define HT7017_REG_MODULE_EN    0x42  // EMU 模块使能 (2B)
#define HT7017_REG_ANAEN        0x43  // ADC 开关寄存器 (1B)
#define HT7017_REG_IOCFG        0x45  // IO 输出配置 (2B)

#define HT7017_REG_ADCCON       0x59  // ADC 通道增益选择 (2B) — 上电后必须写一次!

// ============================================================================
// UART 协议常量
// ============================================================================
#define HT7017_FRAME_HEAD       0x6A  // 固定帧头
#define HT7017_READ_CMD_MASK    0x7F  // 读命令掩码 (CMD[7]=0)
#define HT7017_WRITE_CMD_MASK   0x80  // 写命令掩码 (CMD[7]=1)

// ============================================================================
// 写保护密钥 (手册 §5.2.2.3)
// ============================================================================
#define HT7017_WPCFG_ENABLE_CAL  0xA6  // 开放 50H-7CH 校表参数寄存器的写权限
#define HT7017_WPCFG_ENABLE_CFG  0xBC  // 开放 40H-45H 配置寄存器的写权限
#define HT7017_WPCFG_DISABLE     0x00  // 关闭所有写权限

// ============================================================================
// UART 通信超时 (手册要求字节间隔 < 20ms, 4800bps 下单字节约 2.3ms)
// ============================================================================
#define HT7017_TX_TIMEOUT       50   // 发送超时 (ms)
#define HT7017_RX_TIMEOUT       50   // 接收超时 (ms)

// ACK 响应值
#define HT7017_ACK_OK           0x54
#define HT7017_ACK_FAIL         0x63

// ============================================================================
// 数据结构
// ============================================================================
typedef struct {
    UART_HandleTypeDef *huart;
    uint32_t           I1_Sample;    // 最近一次读取的 I1 采样值
    uint32_t           I2_Sample;    // 最近一次读取的 I2 采样值
    uint32_t           U_Sample;     // 最近一次读取的 U 采样值
} HT7017_HandleTypeDef;

// ============================================================================
// 函数声明
// ============================================================================

/**
 * @brief  初始化 HT7017 句柄，并写入 ADCCON 使能能量计量
 * @note   手册 §3.10: 上电/复位后必须对 ADCCON(59H) 进行一次写操作
 * @param  hdev: 设备句柄
 * @param  huart: HAL UART 句柄 (需预配置为 4800bps, 9-bit, even parity)
 * @param  adccon_val: ADCCON 初始值 (通常为 0x0000, 即所有通道 PGA=1, DG=1)
 */
HAL_StatusTypeDef HT7017_Init(HT7017_HandleTypeDef *hdev,
                              UART_HandleTypeDef *huart,
                              uint16_t adccon_val);

/**
 * @brief  开启写保护 (允许写入指定范围的寄存器)
 * @param  hdev: 设备句柄
 * @param  key: HT7017_WPCFG_ENABLE_CAL (50H-7CH)
 *              HT7017_WPCFG_ENABLE_CFG (40H-45H)
 */
HAL_StatusTypeDef HT7017_WriteEnable(HT7017_HandleTypeDef *hdev, uint8_t key);

/**
 * @brief  关闭写保护
 */
HAL_StatusTypeDef HT7017_WriteDisable(HT7017_HandleTypeDef *hdev);

/**
 * @brief  读取 HT7017 寄存器 (默认3字节数据模式)
 * @param  hdev: 设备句柄
 * @param  reg_addr: 寄存器地址 (0x00-0x7F)
 * @param  pVal: 输出 24-bit 数据
 */
HAL_StatusTypeDef HT7017_ReadReg(HT7017_HandleTypeDef *hdev,
                                 uint8_t reg_addr,
                                 uint32_t *pVal);

/**
 * @brief  写入 HT7017 寄存器 (固定2字节数据)
 * @note   写校表/配置寄存器前必须先调用 HT7017_WriteEnable()
 * @param  hdev: 设备句柄
 * @param  reg_addr: 寄存器地址
 * @param  val: 16-bit 写入值 (单字节寄存器只使用低8位, 高字节自动补0)
 */
HAL_StatusTypeDef HT7017_WriteReg(HT7017_HandleTypeDef *hdev,
                                  uint8_t reg_addr,
                                  uint16_t val);

/**
 * @brief  获取带符号的采样值 (自动处理 24-bit 补码 → int32_t)
 * @param  hdev: 设备句柄
 * @param  reg_addr: 采样/功率寄存器地址 (如 0x00, 0x01, 0x02, 0x0A, 0x0B 等)
 * @param  pSignedVal: 输出带符号 32-bit 值
 * @note   仅适用于有符号寄存器(SPL/DC/Power), 不适用于无符号寄存器(RMS/Freq)
 */
HAL_StatusTypeDef HT7017_GetSignedSample(HT7017_HandleTypeDef *hdev,
                                         uint8_t reg_addr,
                                         int32_t *pSignedVal);

/**
 * @brief  软件复位 HT7017 (向 SRSTREG 写入 0x55)
 * @note   复位后需等待 20ms 再操作寄存器, 且必须重新调用 Init
 */
HAL_StatusTypeDef HT7017_SoftReset(HT7017_HandleTypeDef *hdev);

#ifdef __cplusplus
}
#endif

#endif /* __HT7017_H__ */