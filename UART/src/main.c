#include "main.h"

#define SysCoreClk 42000000    // System core clock frequency (Hz)

/* UART1 configuration: 9600 baud, TX only, no parity, 1 stop bit */
UART_Typedef UART1_config = {
        .baudRate = 9600,               // UART baud rate
        .peripheralClock = SysCoreClk,  // Peripheral clock frequency
        .mode = UART_TX,                // UART mode: transmit only
        .ParityEnable = 0,              // Parity disabled
        .Parity = 0,                    // Even parity (not used since ParityEnable=0)
        .NoStopBit = 1                  // 1 stop bit
};

/* UART6 configuration: 9600 baud, TX/RX, no parity, 1 stop bit */
UART_Typedef UART6_config = {
        .baudRate = 9600,               // UART baud rate
        .peripheralClock = SysCoreClk,  // Peripheral clock frequency
        .mode = UART_TX_RX,             // UART mode: transmit and receive
        .ParityEnable = 0,              // Parity disabled
        .Parity = 0,                    // Even parity (not used since ParityEnable=0)
        .NoStopBit = 1                  // 1 stop bit
};

ringBuffer_Typedef UART6_Buff;          // Ring buffer for UART6 (for interrupt-driven RX/TX)

/**
 * @brief  Main program entry point.
 *         Initializes system, configures UART1 and UART6, and repeatedly sends a message from UART1.
 */
int main(void)
{
    SystemInit();			            // Initialize system clock, GPIO, and timer[2]

    UART_init(UART1, &UART1_config);    // Initialize UART1 with specified configuration
    UART_init(UART6, &UART6_config);    // Initialize UART6 with specified configuration

    ringBuffer_init(&UART6_Buff);       // Initialize ring buffer for UART6[3]

    while (1)
    {
        UART_EnableInterrupts_Rx(UART6);     // Enable RX interrupt for UART6 to receive incoming data

        char* s = "Hello World from UART 1 :)\n\r"; // Message to send via UART1
        while (*s)
        {
            UART_Write(UART1, *s);          // Transmit each character via UART1
            s++;
        }

        UART_EnableInterrupts_Tx(UART6);    // Enable TX interrupt for UART6 to transmit data (if any in buffer)

        Delay_ms(1000);                     // Delay 1 second before next transmission
    }
}