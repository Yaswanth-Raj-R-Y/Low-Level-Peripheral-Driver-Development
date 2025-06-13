#include "main.h"

#define SysCoreClk 42000000     // System core clock frequency in Hz

// SPI1 configuration: Master, 8-bit, MSB first, Mode 2 (CPOL=1, CPHA=0)
SPIconfig_Typedef SPI1_config = {
    .baudRatePrescaler = PRE_256,       // Baud rate prescaler: divide clock by 256
    .operationMode = MASTER,            // SPI operates as master
    .dataOrder = 0,                     // MSB first
    .dataFrameFormat = Bit_8,           // 8-bit data frame
    .SPImode = MODE_2,                  // SPI mode 2 (CPOL=1, CPHA=0)
    .TIenable = 0,                      // Motorola SPI protocol (not TI mode)
    .softwareNSS = 0                    // Hardware NSS management
};

// SPI2 configuration: Slave, 8-bit, MSB first, Mode 2 (CPOL=1, CPHA=0)
SPIconfig_Typedef SPI2_config = {
    .operationMode = SLAVE,             // SPI operates as slave
    .dataOrder = 0,                     // MSB first
    .dataFrameFormat = Bit_8,           // 8-bit data frame
    .SPImode = MODE_2,                  // SPI mode 2 (CPOL=1, CPHA=0)
    .TIenable = 0,                      // Motorola SPI protocol (not TI mode)
    .softwareNSS = 0                    // Hardware NSS management
};

/*
UART_Typedef UART1_config = {
    .baudRate = 9600,
    .peripheralClock = SysCoreClk,
    .mode = UART_TX,
    .NoStopBit = 1
};
   // UART1 can be used to print received SPI data to a serial monitor for debugging
*/

int main(void)
{
    SystemInit();                           // Initialize system clock, GPIO, and timer

    //UART_init(UART1, &UART1_config);      // Optional: Initialize UART1 for serial output

    SPI_init(SPI_1, &SPI1_config);          // Initialize SPI1 as master
    SPI_init(SPI_2, &SPI2_config);          // Initialize SPI2 as slave

    uint8_t data;                           // Variable to hold received data

    while (1)
    {
        SPI_Enable(SPI_1);                  // Enable SPI1 (master)
        SPI_Enable(SPI_2);                  // Enable SPI2 (slave)

        char* str = "Hello World\n\r";      // String to transmit over SPI

        while (*str)
        {
            SPI_Write(SPI_1, *str++);       // Master transmits data byte
            SPI_Read(SPI_1);                // Read to clear overrun flag in master

            // Check if NSS (slave select) pin goes high (master deselects slave)
            if (GPIOB->IDR & GPIO_IDR_ID12)
            {
                break;
            }

            SPI_Write(SPI_2, 0xFF);         // Slave sends dummy byte (required for full-duplex SPI)
            data = SPI_Read(SPI_2);         // Slave receives data from master

            //UART_Write(UART1, data);        // Optional: Print received data via UART1
        }

        SPI_Disable(SPI_1);                 // Disable SPI1 after transmission
        SPI_Disable(SPI_2);                 // Disable SPI2 after transmission

        Delay_ms(1000);                     // Wait 1 second before next transmission
    }
}