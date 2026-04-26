//
// Created by Padhia on 2025/10/26.
// Contact me at: luochj2025@lzu.edu.cn
//

#include "m24256e.h"
#include <stddef.h> // For NULL

// How to use this module:
/**
  *  For example:
  *  First call M24256E_Init() to initialize the device handle,
  *  then use M24256E_WriteByte() and M24256E_ReadByte() to write/read single bytes.
  *
  *  For multi-byte operations, use M24256E_WriteData() and M24256E_ReadSequential().
  *
  *  To configure the device address, use M24256E_SetAndLockCDA().
  *
  *  To access the ID page, use M24256E_WriteIDPage() and M24256E_ReadIDPage().
  */

/* Private Function Prototypes -----------------------------------------------*/

/**
  * @brief  Waits for the device to finish its internal write cycle (tW).
  * @note   The M24256E-F does not ACK any I2C address while busy writing[cite: 576].
  * This function polls the device by sending its address until it
  * receives an ACK or a timeout is reached.
  * @param  dev: Pointer to the device handle.
  * @retval M24256E_Status: M24256E_OK if device becomes ready,
  * M24256E_ERROR_DEVICE_BUSY on timeout.
  */
static M24256E_Status M24256E_WaitForReady(const M24256E_Handle *dev);

/**
 * @brief  Converts HAL I2C status to M24256E_Status.
 * @param  hal_status: The status returned by a HAL_I2C function.
 * @retval M24256E_Status: The corresponding driver status.
 */
static M24256E_Status M24256E_ConvertHALStatus(HAL_StatusTypeDef hal_status);

/* Public Function Implementations -------------------------------------------*/

M24256E_Status M24256E_Init(M24256E_Handle *dev, I2C_HandleTypeDef *hi2c, const uint8_t cda_bits) {
    if (dev == NULL || hi2c == NULL) {
        return M24256E_ERROR_INVALID_PARAM;
    }

    dev->hi2c = hi2c;
    dev->cda_bits = (cda_bits & 0x07); // Ensure only 3 bits are used
    dev->i2c_timeout = M24256E_I2C_TIMEOUT;

    // Check if device is present and responding
    return M24256E_IsDeviceReady(dev);
}

M24256E_Status M24256E_IsDeviceReady(const M24256E_Handle *dev) {
    if (dev == NULL) {
        return M24256E_ERROR_INVALID_PARAM;
    }

    // Use the 8-bit shifted address for HAL_I2C_IsDeviceReady
    const uint16_t dev_addr = (M24256E_MEM_BASE_ADDR | dev->cda_bits) << 1;

    // Try to contact the device. We use a short timeout and 2 trials.
    if (HAL_I2C_IsDeviceReady(dev->hi2c, dev_addr, 2, 5) == HAL_OK) {
        return M24256E_OK;
    }

    return M24256E_ERROR_DEVICE_BUSY;
}


/* --- Main Memory Array Functions --- */

M24256E_Status M24256E_WriteByte(const M24256E_Handle *dev, uint16_t mem_addr, uint8_t data) {
    if (dev == NULL) {
        return M24256E_ERROR_INVALID_PARAM;
    }
    if (mem_addr > M24256E_MAX_MEM_ADDR) {
        return M24256E_ERROR_INVALID_PARAM;
    }

    const uint16_t dev_addr = (M24256E_MEM_BASE_ADDR | dev->cda_bits) << 1;

    // Perform the I2C write operation
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(dev->hi2c,
                                                 dev_addr,
                                                 mem_addr,
                                                 I2C_MEMADD_SIZE_16BIT,
                                                 &data,
                                                 1,
                                                 dev->i2c_timeout);

    if (status != HAL_OK) {
        return M24256E_ConvertHALStatus(status);
    }

    // After a STOP condition, the device starts its internal write cycle (tW)
    // We must wait for it to complete before sending a new command.
    return M24256E_WaitForReady(dev);
}

M24256E_Status M24256E_ReadByte(const M24256E_Handle *dev, const uint16_t mem_addr, uint8_t *data) {
    if (dev == NULL || data == NULL) {
        return M24256E_ERROR_INVALID_PARAM;
    }
    if (mem_addr > M24256E_MAX_MEM_ADDR) {
        return M24256E_ERROR_INVALID_PARAM;
    }

    uint16_t dev_addr = (M24256E_MEM_BASE_ADDR | dev->cda_bits) << 1;

    // Perform the I2C random read operation
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(dev->hi2c,
                                                dev_addr,
                                                mem_addr,
                                                I2C_MEMADD_SIZE_16BIT,
                                                data,
                                                1,
                                                dev->i2c_timeout);

    return M24256E_ConvertHALStatus(status);
}

M24256E_Status M24256E_WriteData(const M24256E_Handle *dev, uint16_t mem_addr, uint8_t *data, uint16_t len) {
    if (dev == NULL || data == NULL || len == 0) {
        return M24256E_ERROR_INVALID_PARAM;
    }
    // Ensure the write operation does not exceed memory boundaries
    if ((uint32_t) mem_addr + len > (M24256E_MAX_MEM_ADDR + 1)) {
        return M24256E_ERROR_INVALID_PARAM;
    }

    uint16_t bytes_written = 0;
    uint16_t dev_addr = (M24256E_MEM_BASE_ADDR | dev->cda_bits) << 1;
    while (bytes_written < len) {
        // 1. Calculate how many bytes can be written in this chunk.
        // A page write must be contained within a 64-byte aligned page.
        uint16_t current_page_start = mem_addr & ~(M24256E_PAGE_SIZE - 1);
        uint16_t bytes_in_page = M24256E_PAGE_SIZE - (mem_addr - current_page_start);

        // 2. Determine the chunk size
        uint16_t bytes_remaining = len - bytes_written;
        uint16_t chunk_size = (bytes_remaining < bytes_in_page) ? bytes_remaining : bytes_in_page;

        // 3. Perform the I2C page write
        HAL_StatusTypeDef status = HAL_I2C_Mem_Write(dev->hi2c,
                                                     dev_addr,
                                                     mem_addr,
                                                     I2C_MEMADD_SIZE_16BIT,
                                                     data + bytes_written,
                                                     chunk_size,
                                                     dev->i2c_timeout);

        if (status != HAL_OK) {
            return M24256E_ConvertHALStatus(status);
        }

        // 4. Wait for the internal write cycle (tW) to complete
        M24256E_Status wait_status = M24256E_WaitForReady(dev);
        if (wait_status != M24256E_OK) {
            return wait_status;
        }

        // 5. Update counters for the next loop
        bytes_written += chunk_size;
        mem_addr += chunk_size;
    }

    return M24256E_OK;
}

M24256E_Status M24256E_ReadSequential(const M24256E_Handle *dev, uint16_t mem_addr, uint8_t *data, uint16_t len) {
    if (dev == NULL || data == NULL || len == 0) {
        return M24256E_ERROR_INVALID_PARAM;
    }
    if (mem_addr > M24256E_MAX_MEM_ADDR) {
        return M24256E_ERROR_INVALID_PARAM;
    }
    // Note: We allow reads that wrap around the 0x7FFF boundary, as this
    // is the specified behavior of the chip

    uint16_t dev_addr = (M24256E_MEM_BASE_ADDR | dev->cda_bits) << 1;

    // Perform the I2C sequential read [cite: 694]
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(dev->hi2c,
                                                dev_addr,
                                                mem_addr,
                                                I2C_MEMADD_SIZE_16BIT,
                                                data,
                                                len,
                                                dev->i2c_timeout);

    return M24256E_ConvertHALStatus(status);
}


/* --- Configurable Device Address (CDA) Register Functions --- */

M24256E_Status M24256E_WriteCDARegister(const M24256E_Handle *dev, uint8_t cda_data) {
    if (dev == NULL) {
        return M24256E_ERROR_INVALID_PARAM;
    }

    // Use the ID/CDA device address
    uint16_t dev_addr = (M24256E_ID_BASE_ADDR | dev->cda_bits) << 1;

    // Perform the write to the CDA's special address
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(dev->hi2c,
                                                 dev_addr,
                                                 M24256E_CDA_REG_ADDR,
                                                 I2C_MEMADD_SIZE_16BIT,
                                                 &cda_data,
                                                 1,
                                                 dev->i2c_timeout);

    if (status != HAL_OK) {
        // This error also occurs if the register is write-protected (DAL=1) [cite: 416]
        return M24256E_ConvertHALStatus(status);
    }

    // Wait for the internal write cycle (tW)
    return M24256E_WaitForReady(dev);
}

M24256E_Status M24256E_ReadCDARegister(const M24256E_Handle *dev, uint8_t *cda_data) {
    if (dev == NULL || cda_data == NULL) {
        return M24256E_ERROR_INVALID_PARAM;
    }

    // Use the ID/CDA device address
    uint16_t dev_addr = (M24256E_ID_BASE_ADDR | dev->cda_bits) << 1;

    // Perform the read from the CDA's special address
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(dev->hi2c,
                                                dev_addr,
                                                M24256E_CDA_REG_ADDR,
                                                I2C_MEMADD_SIZE_16BIT,
                                                cda_data,
                                                1,
                                                dev->i2c_timeout);

    return M24256E_ConvertHALStatus(status);
}

M24256E_Status M24256E_SetAndLockCDA(M24256E_Handle *dev, uint8_t new_cda_bits) {
    if (dev == NULL) {
        return M24256E_ERROR_INVALID_PARAM;
    }

    // Format the data byte: xxxx C2 C1 C0 DAL
    // We set C2, C1, C0 (bits 3,2,1) and set the DAL bit (bit 0) to 1
    uint8_t cda_data = ((new_cda_bits & 0x07) << 1) | 0x01; // Set DAL bit

    // Write the new value. This is an irreversible action[cite: 185].
    M24256E_Status status = M24256E_WriteCDARegister(dev, cda_data);

    if (status == M24256E_OK) {
        // Update the handle's cda_bits to reflect the new setting
        // The device will now only respond to this new address.
        dev->cda_bits = (new_cda_bits & 0x07);
    }

    return status;
}


/* --- Identification (ID) Page Functions --- */

M24256E_Status M24256E_WriteIDPage(const M24256E_Handle *dev, const uint8_t id_addr, uint8_t *data, uint16_t len) {
    if (dev == NULL || data == NULL || len == 0) {
        return M24256E_ERROR_INVALID_PARAM;
    }
    // The ID page is 64 bytes. Writes cannot cross this boundary[cite: 746].
    if ((uint16_t) id_addr + len > M24256E_ID_PAGE_SIZE) {
        return M24256E_ERROR_INVALID_PARAM;
    }

    // Use the ID/CDA device address
    uint16_t dev_addr = (M24256E_ID_BASE_ADDR | dev->cda_bits) << 1;

    // The 16-bit memory address for the ID page is just the byte offset (0-63)
    // This ensures A10=0[cite: 218, 297, 304].
    uint16_t mem_addr = (uint16_t) id_addr;

    // Perform the write operation [cite: 455]
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(dev->hi2c,
                                                 dev_addr,
                                                 mem_addr,
                                                 I2C_MEMADD_SIZE_16BIT,
                                                 data,
                                                 len,
                                                 dev->i2c_timeout);

    if (status != HAL_OK) {
        // This error also occurs if the page is locked
        return M24256E_ConvertHALStatus(status);
    }

    // Wait for the internal write cycle (tW)
    return M24256E_WaitForReady(dev);
}

M24256E_Status M24256E_ReadIDPage(const M24256E_Handle *dev, const uint8_t id_addr, uint8_t *data, const uint16_t len) {
    if (dev == NULL || data == NULL || len == 0) {
        return M24256E_ERROR_INVALID_PARAM;
    }
    // The ID page is 64 bytes. Reads cannot cross this boundary
    if ((uint16_t) id_addr + len > M24256E_ID_PAGE_SIZE) {
        return M24256E_ERROR_INVALID_PARAM;
    }

    // Use the ID/CDA device address
    uint16_t dev_addr = (M24256E_ID_BASE_ADDR | dev->cda_bits) << 1;

    // The 16-bit memory address for the ID page is just the byte offset (0-63)
    uint16_t mem_addr = (uint16_t) id_addr;

    // Perform the read operation [cite: 741]
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(dev->hi2c,
                                                dev_addr,
                                                mem_addr,
                                                I2C_MEMADD_SIZE_16BIT,
                                                data,
                                                len,
                                                dev->i2c_timeout);

    return M24256E_ConvertHALStatus(status);
}

M24256E_Status M24256E_LockIDPage(const M24256E_Handle *dev) {
    if (dev == NULL) {
        return M24256E_ERROR_INVALID_PARAM;
    }

    // Use the ID/CDA device address
    uint16_t dev_addr = (M24256E_ID_BASE_ADDR | dev->cda_bits) << 1;
    uint8_t lock_data = M24256E_ID_LOCK_DATA_BYTE;

    // Perform the write to the ID Lock address with the specific lock data byte
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(dev->hi2c,
                                                 dev_addr,
                                                 M24256E_ID_LOCK_REG_ADDR,
                                                 I2C_MEMADD_SIZE_16BIT,
                                                 &lock_data,
                                                 1,
                                                 dev->i2c_timeout);

    if (status != HAL_OK) {
        // This error also occurs if the page is already locked
        return M24256E_ConvertHALStatus(status);
    }

    // Wait for the internal write cycle (tW)
    return M24256E_WaitForReady(dev);
}

M24256E_Status M24256E_ReadIDLockStatus(const M24256E_Handle *dev, bool *is_locked) {
    if (dev == NULL || is_locked == NULL) {
        return M24256E_ERROR_INVALID_PARAM;
    }

    // Use the ID/CDA device address
    const uint16_t dev_addr = (M24256E_ID_BASE_ADDR | dev->cda_bits) << 1;
    uint8_t dummy_data = 0x00;

    // This command works by sending a dummy write to the ID Lock address
    // If the page is UNLOCKED, the device will ACK the data byte
    // If the page is LOCKED, the device will NACK the data byte
    // HAL_I2C_Mem_Write will return HAL_OK on ACK, and HAL_ERROR on NACK.
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(dev->hi2c,
                                                 dev_addr,
                                                 M24256E_ID_LOCK_REG_ADDR,
                                                 I2C_MEMADD_SIZE_16BIT,
                                                 &dummy_data,
                                                 1,
                                                 dev->i2c_timeout);

    if (status == HAL_OK) {
        *is_locked = false; // ACK received -> Unlocked
    } else {
        *is_locked = true; // NACK received -> Locked
    }

    // Per datasheet, a START followed by a STOP must be sent
    // *after* this check to reset the device's internal logic.
    // We send a 0-byte transmission, which generates: START | ADDR+W | STOP
    // We use a short timeout and don't care about the return status.uint8_t dummy = 0x00;
    uint8_t dummy = 0x00;
    HAL_I2C_Master_Transmit(dev->hi2c, dev_addr, &dummy, 0, 10);

    // This function always returns OK because the NACK is the expected
    // behavior for a locked device, not a communication failure.
    return M24256E_OK;
}


/* Private Function Implementations ------------------------------------------*/

static M24256E_Status M24256E_WaitForReady(const M24256E_Handle *dev) {
    uint32_t start_tick = HAL_GetTick();
    // We poll for slightly longer than the max write time (tW = 5ms)
    uint32_t timeout = M24256E_WRITE_CYCLE_MAX_MS + 5; // 10ms total timeout

    uint16_t dev_addr = (M24256E_MEM_BASE_ADDR | dev->cda_bits) << 1;

    while (HAL_GetTick() - start_tick < timeout) {
        // HAL_I2C_IsDeviceReady sends the I2C address and checks for an ACK
        // If it gets an ACK, the device is no longer busy with the internal write.
        if (HAL_I2C_IsDeviceReady(dev->hi2c, dev_addr, 1, 5) == HAL_OK) {
            return M24256E_OK;
        }
        // Wait 1ms between polls
        HAL_Delay(1);
    }

    return M24256E_ERROR_DEVICE_BUSY; // Timed out waiting for device
}

static M24256E_Status M24256E_ConvertHALStatus(HAL_StatusTypeDef hal_status) {
    switch (hal_status) {
        case HAL_OK:
            return M24256E_OK;
        case HAL_ERROR:
            return M24256E_ERROR_I2C; // General error, often a NACK
        case HAL_BUSY:
            return M24256E_ERROR_I2C; // I2C peripheral itself is busy
        case HAL_TIMEOUT:
            return M24256E_ERROR_DEVICE_BUSY; // I2C timeout
        default:
            return M24256E_ERROR_I2C;
    }
}
