#include "SPI.h"

/*************************************** Setup SPI *******************************************
 * @brief  Initializes the SPI peripheral according to the specified parameters in SPIconfig.
 *
 * @param  SPI: Pointer to SPI peripheral (SPI1, SPI2, SPI3)
 * @param  SPIconfig: Pointer to configuration structure
 * @note   Enables peripheral clock, configures mode, data size, NSS, and interrupt settings.
 */
void SPI_init(SPI_TypeDef* SPI, SPIconfig_Typedef* SPIconfig)
{
    // Enable the appropriate SPI peripheral clock
    if ((void*)SPI == (void*)SPI1)
    {
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;         // Enable SPI1 clock
        #if SPI1_INTERRUPT_ENABLE
        __disable_irq();                            // Disable global interrupts for safe NVIC config
        NVIC_EnableIRQ(SPI1_IRQn);                  // Enable SPI1 interrupt in NVIC
        __enable_irq();                             // Re-enable global interrupts
        #endif
    }
    else if ((void*)SPI == (void*)SPI2)
    {
        RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;         // Enable SPI2 clock
        #if SPI2_INTERRUPT_ENABLE
        __disable_irq();
        NVIC_EnableIRQ(SPI2_IRQn);
        __enable_irq();
        #endif
    }
    else if ((void*)SPI == (void*)SPI3)
    {
        RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;         // Enable SPI3 clock
        #if SPI3_INTERRUPT_ENABLE
        __disable_irq();
        NVIC_EnableIRQ(SPI3_IRQn);
        __enable_irq();
        #endif
    }

    // Disable SPI before configuration to avoid spurious transfers
    SPI->CR1 &= ~SPI_CR1_SPE;

    // Configure baud rate prescaler (only relevant in master mode)
    if (SPIconfig->operationMode)                       // Master mode
    {
        SPI->CR1 |= (SPIconfig->baudRatePrescaler << 3);
    }

    // Configure data frame format: 8-bit or 16-bit
    if (SPIconfig->dataFrameFormat)
    {
        SPI->CR1 |= SPI_CR1_DFF;                        // 16-bit data frame
    }
    else
    {
        SPI->CR1 &= ~(SPI_CR1_DFF);                     // 8-bit data frame
    }

    // Set SPI mode (CPOL and CPHA bits)
    SPI->CR1 |= SPIconfig->SPImode;

    // Set data order: MSB or LSB first
    if (SPIconfig->dataOrder)
    {
        SPI->CR1 |= SPI_CR1_LSBFIRST;                   // LSB first
    }
    else
    {
        SPI->CR1 &= ~(SPI_CR1_LSBFIRST);                // MSB first
    }

    // Select SPI protocol: TI mode or Motorola mode
    if (SPIconfig->TIenable)
    {
        SPI->CR2 |= SPI_CR2_FRF;                        // TI mode
    }
    else
    {
        SPI->CR2 &= ~(SPI_CR2_FRF);                     // Motorola mode
    }

    // Configure NSS (slave select) management
    if (SPIconfig->softwareNSS)
    {
        SPI->CR1 |= (SPIconfig->softwareNSS << SPI_CR1_SSM_Pos);    // Software NSS management
        SPI->CR1 |= (SPIconfig->NSSactiveHigh << SPI_CR1_SSI_Pos);  // NSS active high/low
    }
    else
    {
        SPI->CR2 |= SPI_CR2_SSOE;                       // Hardware NSS output enable (master mode)
    }

    // Enable SPI interrupts for transmit and receive, if configured
    #if SPI1_INTERRUPT_ENABLE || SPI2_INTERRUPT_ENABLE || SPI3_INTERRUPT_ENABLE
    SPI->CR2 |= SPI_CR2_TXEIE | SPI_CR2_RXNEIE;         // Enable TXE and RXNE interrupts
    #endif

    // Set master or slave mode
    if (SPIconfig->operationMode)
    {
        SPI->CR1 |= SPI_CR1_MSTR;                       // Master mode
    }
    else
    {
        SPI->CR1 &= ~(SPI_CR1_MSTR);                    // Slave mode
    }
}

/************************************ Enable/Disable SPI **************************************
 * @brief  Enables or disables the SPI peripheral.
 *
 * @param  SPI: Pointer to SPI peripheral
 */
void SPI_Enable(SPI_TypeDef* SPI)
{
    SPI->CR1 |= SPI_CR1_SPE;                // Enable SPI (set SPE bit)
}
void SPI_Disable(SPI_TypeDef* SPI)
{
    while (SPI->SR & SPI_SR_BSY);           // Wait until SPI is not busy
    SPI->CR1 &= ~SPI_CR1_SPE;               // Disable SPI (clear SPE bit)
}

/************************************* Transmit data ******************************************
 * @brief  Blocking transmit: Waits until TX buffer is empty, then writes data to SPI.
 *
 * @param  SPI: Pointer to SPI peripheral
 * @param  data: Data to transmit (8 or 16 bits depending on configuration)
 */
void SPI_Write(SPI_TypeDef* SPI, uint16_t data)
{
    while (!(SPI->SR & SPI_SR_TXE));        // Wait until transmit buffer is empty
    SPI->DR = data;                         // Write data to data register
}

/************************************* Receive data *******************************************
 * @brief  Blocking receive: Waits until RX buffer is not empty, then reads data from SPI.
 *
 * @param  SPI: Pointer to SPI peripheral
 * @return Received data (8 or 16 bits depending on configuration)
 */
uint16_t SPI_Read(SPI_TypeDef* SPI)
{
    while (!(SPI->SR & SPI_SR_RXNE));       // Wait until receive buffer is not empty
    return SPI->DR;                         // Read and return received data
}

/****************************************** ISR ***********************************************
 * @brief  SPI interrupt handlers for buffered transmit/receive.
 *         Handles TXE (transmit buffer empty), RXNE (receive buffer not empty), and OVR (overrun error).
 *
 * @note   Uses ring buffers for efficient interrupt-driven communication.
 */
#if SPI1_INTERRUPT_ENABLE
void SPI_IRQHandler(void) 
{
    // Handle transmit buffer empty interrupt
    if (SPI1->SR & SPI_SR_TXE)
    {
        if (!(ringBuffer_isEmpty(&SPI1_Buff)))
        {
            uint8_t Tx_Data = ringBuffer_Read(&SPI1_Buff);
            SPI1->DR = Tx_Data;                         // Send next byte from buffer
        }
        else
        {
            SPI1->CR2 &= ~SPI_CR2_TXEIE;                // Disable TXE interrupt if buffer is empty
        }
    }

    // Handle receive buffer not empty interrupt
    if (SPI1->SR & SPI_SR_RXNE)
    {
        uint8_t Rx_data = SPI1->DR;                     // Read received data
        if (!(ringBuffer_isFull(&SPI1_Buff)))
        {
            ringBuffer_Write(&SPI1_Buff, Rx_data);      // Store data in buffer
        }
        else
        {
            SPI1->CR2 &= ~SPI_CR2_RXNEIE;               // Disable RXNE interrupt if buffer is full
        }
    }

    // Handle overrun error interrupt
    if (SPI1->SR & SPI_SR_OVR)
    {
        uint8_t data = SPI1->DR;                        // Clear OVR by reading DR and SR
        data = SPI1->SR;
        (void)data;
    }
}
#endif

#if SPI2_INTERRUPT_ENABLE
void SPI_IRQHandler(void) 
{
    if (SPI2->SR & SPI_SR_TXE)
    {
        if (!(ringBuffer_isEmpty(&SPI2_Buff)))
        {
            uint8_t Tx_Data = ringBuffer_Read(&SPI2_Buff);
            SPI2->DR = Tx_Data;
        }
        else
        {
            SPI2->CR2 &= ~SPI_CR2_TXEIE;
        }
    }

    if (SPI2->SR & SPI_SR_RXNE)
    {
        uint8_t Rx_data = SPI2->DR;
        if (!(ringBuffer_isFull(&SPI2_Buff)))
        {
            ringBuffer_Write(&SPI2_Buff, Rx_data);
        }
        else
        {
            SPI2->CR2 &= ~SPI_CR2_RXNEIE;
        }
    }

    if (SPI2->SR & SPI_SR_OVR)
    {
        uint8_t data = SPI2->DR;
        data = SPI2->SR;
        (void)data;
    }
}
#endif

#if SPI3_INTERRUPT_ENABLE
void SPI_IRQHandler(void) 
{
    if (SPI3->SR & SPI_SR_TXE)
    {
        if (!(ringBuffer_isEmpty(&SPI3_Buff)))
        {
            uint8_t Tx_Data = ringBuffer_Read(&SPI3_Buff);
            SPI3->DR = Tx_Data;
        }
        else
        {
            SPI3->CR2 &= ~SPI_CR2_TXEIE;
        }
    }

    if (SPI3->SR & SPI_SR_RXNE)
    {
        uint8_t Rx_data = SPI3->DR;
        if (!(ringBuffer_isFull(&SPI3_Buff)))
        {
            ringBuffer_Write(&SPI3_Buff, Rx_data);
        }
        else
        {
            SPI3->CR2 &= ~SPI_CR2_RXNEIE;
        }
    }

    if (SPI3->SR & SPI_SR_OVR)
    {
        uint8_t data = SPI3->DR;
        data = SPI3->SR;
        (void)data;
    }
}
#endif