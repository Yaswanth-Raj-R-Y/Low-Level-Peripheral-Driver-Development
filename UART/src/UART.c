#include "UART.h"

/*************************************** UART Initialization *******************************************
 * @brief  Initializes the UART peripheral according to the specified parameters in uartConfig.
 *
 * @param  UART: Pointer to USART peripheral (USART1, USART2, USART6)
 * @param  uartConfig: Pointer to UART configuration structure
 * @note   Enables peripheral clock, sets baud rate, stop bits, parity, and mode.
 */
void UART_init(USART_TypeDef* UART, UART_Typedef* uartConfig)
{
    // Enable UART clock based on selected USART instance
    if ((void*)UART == (void*)UART1)
    {
        RCC->APB2ENR |= 0x00000010;     // Enable USART1 clock
        #if UART1_INTERRUPT_ENABLE
        __disable_irq();                // Disable global interrupts for safe NVIC config
        NVIC_EnableIRQ(USART1_IRQn);    // Enable USART1 interrupt in NVIC
        __enable_irq();                 // Re-enable global interrupts
        #endif
    }
    else if ((void*)UART == (void*)UART6)
    {
        RCC->APB2ENR |= 0x00000020;     // Enable USART6 clock
        #if UART6_INTERRUPT_ENABLE
        __disable_irq();
        NVIC_EnableIRQ(USART6_IRQn);
        __enable_irq();
        #endif
    }
    else if ((void*)UART == (void*)UART2)
    {
        RCC->APB1ENR |= 0x00020000;     // Enable USART2 clock
        #if UART2_INTERRUPT_ENABLE
        __disable_irq();
        NVIC_EnableIRQ(USART2_IRQn);
        __enable_irq();
        #endif
    }

    UART->CR1 |= 0x00002000;            // Enable USART by setting UE bit

    // Configure number of stop bits
    if (uartConfig->NoStopBit == 2)
    {
        UART->CR2 = USART_CR2_STOP_1;   // 2 stop bits
    }
    else
    {
        UART->CR2 &= ~USART_CR2_STOP;   // 1 stop bit
    }

    // Configure parity settings
    if (uartConfig->ParityEnable)
    {
        UART->CR1 |= 0x00000400;                // Enable parity control
        UART->CR1 |= uartConfig->Parity << 9;   // Set odd or even parity
    }

    UART->CR2 |= 0x00003000;                    // Set stop bit configuration (default: 1 stop bit)

    // Configure baud rate
    UART->BRR = (uartConfig->peripheralClock/uartConfig->baudRate)+1;

    // Configure UART mode: TX, RX, or both
    switch (uartConfig->mode)
    {
        case UART_TX:   UART->CR1 |= 0x00000008;        // Enable Tx
                        break;
        
        case UART_RX:   UART->CR1 |= 0x00000004;        // Enable Rx
                        break;

        case UART_TX_RX:    UART->CR1 |= 0x0000000C;    // Enable both Tx & Rx
                            break;
    }
}

/************************************** Transmit Data ******************************************
 * @brief  Blocking transmit: Waits until TX buffer is empty, then writes data to UART.
 *
 * @param  UART: Pointer to USART peripheral
 * @param  Tx_data: Byte to transmit
 */
void UART_Write(USART_TypeDef* UART, uint8_t Tx_data)
{
    while (!(UART->SR & USART_SR_TXE));    // Wait until transmit buffer is empty
    UART->DR = Tx_data;                    // Write data to data register
}

/************************************** Receive Data *******************************************
 * @brief  Blocking receive: Waits until RX buffer is not empty, then reads data from UART.
 *
 * @param  UART: Pointer to USART peripheral
 * @return Received byte
 */
uint8_t UART_Read(USART_TypeDef* UART)
{
    while (!(UART->SR & USART_SR_RXNE));   // Wait until receive buffer is not empty
    return UART->DR;                       // Read and return received data
}

/************************************* Enable UART Interrupts ***********************************
 * @brief  Enables both transmit and receive interrupts for the given UART.
 *
 * @param  UART: Pointer to USART peripheral
 */
void UART_EnableInterrupts(USART_TypeDef* UART)
{
    UART->CR1 |= USART_CR1_TXEIE | USART_CR1_RXNEIE;    // Enable TXE and RXNE interrupts
}

// Enable only transmit interrupt
void UART_EnableInterrupts_Tx(USART_TypeDef* UART)
{
    UART->CR1 |= USART_CR1_TXEIE;                       // Enable TXE interrupt
}

// Enable only receive interrupt
void UART_EnableInterrupts_Rx(USART_TypeDef* UART)
{
    UART->CR1 |= USART_CR1_RXNEIE;                      // Enable RXNE interrupt
}

/************************************* UART Interrupt Handlers **********************************
 * @brief  UART interrupt handlers for buffered transmit/receive.
 *         Handles TXE (transmit buffer empty) and RXNE (receive buffer not empty).
 *
 * @note   Uses ring buffers for efficient interrupt-driven communication.
 */
#if UART1_INTERRUPT_ENABLE
void USART1_IRQHandler(void)
{
    // Handle transmit buffer empty interrupt
    if (USART1->SR & USART_SR_TXE)
    {
        if (!(ringBuffer_isEmpty(&UART1_Buff)))
        {
            uint8_t Tx_Data = ringBuffer_Read(&UART1_Buff); // Get next byte from buffer
            USART1->DR = Tx_Data;                           // Send data
        }
        else
        {
            USART1->CR1 &= ~USART_CR1_TXEIE;                // Disable TXE interrupt if buffer is empty
        }
    }

    // Handle receive buffer not empty interrupt
    if (USART1->SR & USART_SR_RXNE)
    {
        uint8_t Rx_data = USART1->DR;                       // Read received data
        if (!(ringBuffer_isFull(&UART1_Buff)))
        {
            ringBuffer_Write(&UART1_Buff, Rx_data);         // Store data in buffer
        }
    }
}
#endif

#if UART2_INTERRUPT_ENABLE
void USART2_IRQHandler(void)
{
    if (USART2->SR & USART_SR_TXE)
    {
        if (!(ringBuffer_isEmpty(&UART2_Buff)))
        {
            uint8_t Tx_Data = ringBuffer_Read(&UART2_Buff);
            USART2->DR = Tx_Data;
        }
        else
        {
            USART2->CR1 &= ~USART_CR1_TXEIE;
        }
    }

    if (USART2->SR & USART_SR_RXNE)
    {
        uint8_t Rx_data = USART2->DR;
        if (!(ringBuffer_isFull(&UART2_Buff)))
        {
            ringBuffer_Write(&UART2_Buff, Rx_data);
        }
    }
}
#endif

#if UART6_INTERRUPT_ENABLE
void USART6_IRQHandler(void)
{
    if (USART6->SR & USART_SR_TXE)
    {
        if (!(ringBuffer_isEmpty(&UART6_Buff)))
        {
            uint8_t Tx_Data = ringBuffer_Read(&UART6_Buff);
            USART6->DR = Tx_Data;
        }
        else
        {
            USART6->CR1 &= ~USART_CR1_TXEIE;
        }
    }

    if (USART6->SR & USART_SR_RXNE)
    {
        uint8_t Rx_data = USART6->DR;
        if (!(ringBuffer_isFull(&UART6_Buff)))
        {
            ringBuffer_Write(&UART6_Buff, Rx_data);
        }
    }
}
#endif