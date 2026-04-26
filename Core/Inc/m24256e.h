//
// Created by Padhia on 2025/10/26.
// Contact me at: luochj2025@lzu.edu.cn
//

#ifndef M24256E_H_
#define M24256E_H_

#include "main.h"
#include <stdbool.h> // For bool type in M24256E_ReadIDLockStatus

/**
  * @brief  Base 7-bit I2C device addresses.
  * The final address depends on the 3 configurable bits (C2, C1, C0).
  * Final 7-bit address = (BASE_ADDR | cda_bits)
  */
#define M24256E_MEM_BASE_ADDR           0x50 // 0b1010000 [cite: 293]
#define M24256E_ID_BASE_ADDR            0x58 // 0b1011000 [cite: 293]

// Memory organization parameters
#define M24256E_PAGE_SIZE               64       // 64 bytes per page [cite: 25]
#define M24256E_MAX_MEM_ADDR            0x7FFF   // 32 Kbytes = 32767 addresses
#define M24256E_ID_PAGE_SIZE            64       // 64-byte identification page [cite: 26, 214]


// Timing parameters
#define M24256E_WRITE_CYCLE_MAX_MS      5        // Max byte or page write time (tW)
#define M24256E_I2C_TIMEOUT             100      // Default HAL I2C timeout in ms


// Special 16-bit memory addresses for accessing registers via the ID_BASE_ADDR (0x58).
// Address for Configurable Device Address (CDA) Register [cite: 175, 297]
#define M24256E_CDA_REG_ADDR            0xC000   // A15=1, A14=1, A13=0
// Start address for the 64-byte ID Page [cite: 218, 297, 304]
#define M24256E_ID_PAGE_ADDR_START      0x0000   // A10=0, (A15-A13) != 110
// Address for locking the ID Page [cite: 297, 304]
#define M24256E_ID_LOCK_REG_ADDR        0x0400   // A10=1
// Data byte to send to M24256E_ID_LOCK_REG_ADDR to lock the ID page [cite: 534]
#define M24256E_ID_LOCK_DATA_BYTE       0x02     // Binary xxxx xx1x

// Status return codes for driver functions
typedef enum {
    M24256E_OK = 0, // Operation successful
    M24256E_ERROR_I2C, // HAL I2C function returned an error (e.g., NACK)
    M24256E_ERROR_DEVICE_BUSY, // Device is busy with an internal write cycle (tW)
    M24256E_ERROR_INVALID_PARAM // Invalid parameter (e.g., NULL pointer, address out of range)
} M24256E_Status;

// M24256E device handle structure
typedef struct {
    I2C_HandleTypeDef *hi2c; // Pointer to the I2C peripheral handle
    uint8_t cda_bits; // Configurable Device Address (0-7)
    uint16_t i2c_timeout; // Timeout for I2C operations
} M24256E_Handle;

M24256E_Status M24256E_Init(M24256E_Handle *dev, I2C_HandleTypeDef *hi2c, uint8_t cda_bits);

M24256E_Status M24256E_IsDeviceReady(const M24256E_Handle *dev);

M24256E_Status M24256E_WriteByte(const M24256E_Handle *dev, uint16_t mem_addr, uint8_t data);

M24256E_Status M24256E_ReadByte(const M24256E_Handle *dev, uint16_t mem_addr, uint8_t *data);

M24256E_Status M24256E_WriteData(const M24256E_Handle *dev, uint16_t mem_addr, uint8_t *data, uint16_t len);

M24256E_Status M24256E_ReadSequential(const M24256E_Handle *dev, uint16_t mem_addr, uint8_t *data, uint16_t len);

M24256E_Status M24256E_WriteCDARegister(const M24256E_Handle *dev, uint8_t cda_data);

M24256E_Status M24256E_ReadCDARegister(const M24256E_Handle *dev, uint8_t *cda_data);

M24256E_Status M24256E_SetAndLockCDA(M24256E_Handle *dev, uint8_t new_cda_bits);

M24256E_Status M24256E_WriteIDPage(const M24256E_Handle *dev, uint8_t id_addr, uint8_t *data, uint16_t len);

M24256E_Status M24256E_ReadIDPage(const M24256E_Handle *dev, uint8_t id_addr, uint8_t *data, uint16_t len);

M24256E_Status M24256E_LockIDPage(const M24256E_Handle *dev);

M24256E_Status M24256E_ReadIDLockStatus(const M24256E_Handle *dev, bool *is_locked);

#endif /* M24256E_H_ */
