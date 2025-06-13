#include "main.h"
#include "I2C.h"

#define I2C1_ADDRESS 0x30                       // I2C1 Master Address
#define I2C2_ADDRESS 0x31                       // I2C2 Slave Address
#define I2C_CLOCK_SPEED 100000                  // 100 kHz standard I2C speed

// I2C1: Master configuration
I2C_Config_Typedef i2c1Config = {
    .clockSpeed = I2C_CLOCK_SPEED,
    .addressingMode = 0,                        // 7-bit addressing
    .ownAddress = I2C1_ADDRESS
};

// I2C2: Slave configuration
I2C_Config_Typedef i2c2Config = {
    .clockSpeed = I2C_CLOCK_SPEED,
    .addressingMode = 0,                        // 7-bit addressing
    .ownAddress = I2C2_ADDRESS
};

int main(void)
{
    SystemInit();                               // Initialize system clock, GPIO, timer

    I2C_Init(I2C1, &i2c1Config);                // Initialize I2C1 as master
    I2C_Init(I2C2, &i2c2Config);                // Initialize I2C2 as slave

    Log_s("I2C Communication Initialized");     // Log initialization

    char *message = "Hello from I2C1!";         // Message to transmit

    while (1)
    {
        I2C_Start(I2C1, I2C2_ADDRESS, 0);       // Master starts transmission to slave (write mode)

        for (uint8_t i = 0; message[i] != '\0'; i++)
        {
            I2C_Write(I2C1, message[i]);        // Send each character
        }

        I2C_Stop(I2C1);                         // End transmission

        Delay_ms(1000);                         // Wait before next transmission
    }
}