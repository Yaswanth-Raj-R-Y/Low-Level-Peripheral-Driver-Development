#include "I2C.h"

/************************************** I2C Initialization ***************************************
 * @brief  Initializes the I2C peripheral with the specified configuration.
 *
 * @param  I2Cx: Pointer to I2C peripheral (I2C1, I2C2, I2C3)
 * @param  config: Pointer to I2C configuration structure
 * @note   Sets up clock, addressing mode, own address, and enables the peripheral.
 */
void I2C_Init(I2C_TypeDef* I2Cx, I2C_Config_Typedef* config)
{
    // 1. Enable Clock for the selected I2C peripheral
    if (I2Cx == I2C1)
        RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    else if (I2Cx == I2C2)
        RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
    else if (I2Cx == I2C3)
        RCC->APB1ENR |= RCC_APB1ENR_I2C3EN;

    // 2. Disable I2C before configuration (recommended)
    I2Cx->CR1 &= ~I2C_CR1_PE;

    // 3. Perform software reset to ensure a clean start
    I2Cx->CR1 |= I2C_CR1_SWRST;
    I2Cx->CR1 &= ~I2C_CR1_SWRST;

    // 4. Set peripheral clock frequency (CR2), in MHz
    uint32_t pclk1 = 42000000;  // APB1 clock is 42 MHz for STM32F4
    I2Cx->CR2 = (pclk1 / 1000000) & I2C_CR2_FREQ;

    // 5. Configure clock control register (CCR) for desired speed (e.g., 100 kHz)
    I2Cx->CCR = (pclk1 / (config->clockSpeed * 2)) & 0xFFF;

    // 6. Set maximum rise time (TRISE) for standard mode
    I2Cx->TRISE = ((pclk1 / 1000000) + 1) & 0x3F;

    // 7. Configure addressing mode and own address
    if (config->addressingMode == 1)
        I2Cx->OAR1 |= I2C_OAR1_ADDMODE;     // 10-bit addressing
    else
        I2Cx->OAR1 &= ~I2C_OAR1_ADDMODE;    // 7-bit addressing
    I2Cx->OAR1 |= config->ownAddress << 1;  // Set own address (left-aligned for STM32)

    // 8. Enable the I2C peripheral
    I2Cx->CR1 |= I2C_CR1_PE;
}

/************************************** I2C Start Condition ***************************************
 * @brief  Generates a START condition and sends the slave address with R/W bit.
 *
 * @param  I2Cx: Pointer to I2C peripheral
 * @param  slaveAddr: 7-bit slave address
 * @param  isRead: 0 = write, 1 = read
 */
void I2C_Start(I2C_TypeDef* I2Cx, uint8_t slaveAddr, uint8_t isRead)
{
    // Generate START condition
    I2Cx->CR1 |= I2C_CR1_START;

    // Wait until START condition is generated (SB bit set)
    while (!(I2Cx->SR1 & I2C_SR1_SB));

    // Send slave address with R/W bit (write:0, read:1)
    I2Cx->DR = (slaveAddr << 1) | isRead;

    // Wait until address is sent (ADDR bit set)
    while (!(I2Cx->SR1 & I2C_SR1_ADDR));
    (void)I2Cx->SR2;    // Clear ADDR flag by reading SR1 then SR2
}

/************************************** I2C Write Data *********************************************
 * @brief  Writes a single byte to the I2C bus.
 *
 * @param  I2Cx: Pointer to I2C peripheral
 * @param  data: Byte to transmit
 */
void I2C_Write(I2C_TypeDef* I2Cx, uint8_t data)
{
    // Wait until data register is empty (TXE set)
    while (!(I2Cx->SR1 & I2C_SR1_TXE));

    // Write data to data register
    I2Cx->DR = data;

    // Wait until byte transfer is finished (BTF set)
    while (!(I2Cx->SR1 & I2C_SR1_BTF));
}

/************************************** I2C Read Data **********************************************
 * @brief  Reads a single byte from the I2C bus, with ACK or NACK.
 *
 * @param  I2Cx: Pointer to I2C peripheral
 * @param  ack: 1 = send ACK after reception, 0 = send NACK (for last byte)
 * @return Received byte
 */
uint8_t I2C_Read(I2C_TypeDef* I2Cx, uint8_t ack)
{
    // Configure ACK control
    if (ack)
        I2Cx->CR1 |= I2C_CR1_ACK;
    else
        I2Cx->CR1 &= ~I2C_CR1_ACK;

    // Wait until data is received (RXNE set)
    while (!(I2Cx->SR1 & I2C_SR1_RXNE));

    // Return received data
    return I2Cx->DR;
}

/************************************** I2C Stop Condition ******************************************
 * @brief  Generates a STOP condition on the I2C bus.
 * @param  I2Cx: Pointer to I2C peripheral
 */
void I2C_Stop(I2C_TypeDef* I2Cx)
{
    I2Cx->CR1 |= I2C_CR1_STOP;
}
