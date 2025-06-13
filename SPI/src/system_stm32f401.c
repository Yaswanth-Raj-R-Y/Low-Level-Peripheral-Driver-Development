#include "main.h"

#define PLLM            (25U)          // PLLM division factor for PLL input clock
#define PLLN            (252U << 6)    // PLLN multiplication factor for VCO
#define PLLP            (1U << 17)     // PLLP division factor for main system clock
#define PSC_VALUE       41             // Timer prescaler value for TIM11
#define ARR_VALUE       0xFFFF         // Timer auto-reload value for TIM11

/************************************* System Clock Configuration **************************************
 * @brief  Configures the system clock to 42MHz using HSE and PLL.
 *         Sets up PLL parameters, flash latency, and bus prescalers for STM32F4.
 */
static void SysClockConfig_42Mhz()
{
    RCC->CR |= RCC_CR_HSEON;                        // Enable High Speed External (HSE) oscillator
    while (!(RCC->CR & RCC_CR_HSERDY));             // Wait for HSE to be ready
    
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSE;         // Set HSE as PLL clock source

    RCC->APB1ENR |= RCC_APB1ENR_PWREN;              // Enable Power interface clock
    PWR->CR |= PWR_CR_VOS_1;                        // Set voltage scaling (Scale 2 mode)

    FLASH->ACR |= FLASH_ACR_LATENCY_2WS             // Set Flash latency to 2 wait states
                  | FLASH_ACR_DCEN                  // Enable data cache 
                  | FLASH_ACR_PRFTEN                // Enable prefetch
                  | FLASH_ACR_ICEN;                 // Enable instruction cache

    RCC->PLLCFGR |= PLLM | PLLN | PLLP;             // Set PLLM, PLLN, PLLP for desired frequency

    RCC->CFGR |= RCC_CFGR_HPRE_DIV1                 // Set AHB prescaler to 1
                 | RCC_CFGR_PPRE1_DIV1              // Set APB1 prescaler to 1
                 | RCC_CFGR_PPRE2_DIV1;             // Set APB2 prescaler to 1

    RCC->CR |= RCC_CR_PLLON;                        // Enable PLL
    while (!(RCC->CR & RCC_CR_PLLRDY));             // Wait for PLL to be ready

    RCC->CFGR |= RCC_CFGR_SW_PLL;                   // Select PLL as system clock source
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_1);   // Wait until PLL is used as system clock
}

/****************************************** GPIO Configuration *****************************************
 * @brief  Configures GPIOA and GPIOB for SPI1, SPI2, and UART1 alternate functions.
 *         Sets mode, speed, pull-up, and alternate function registers for each pin.
 */
static void gpioConfig()
{
    RCC->AHB1ENR |= 1;                      // Enable GPIOA clock
    RCC->AHB1ENR |= (1 << 1);               // Enable GPIOB clock
   
    // --- SPI1 (GPIOA) Pin Configuration ---
    // Set PA4 (NSS), PA5 (SCK), PA6 (MISO), PA7 (MOSI) to Alternate Function mode
    GPIOA->MODER |= GPIO_MODER_MODE4_1 | GPIO_MODER_MODE5_1 | GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1;
    // Set pull-up for MISO and NSS
    GPIOA->PUPDR |= GPIO_PUPDR_PUPD6_1 | GPIO_PUPDR_PUPD4_0;
    // Set high speed for SPI pins
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR4_1 | GPIO_OSPEEDER_OSPEEDR5_1 | GPIO_OSPEEDER_OSPEEDR6_1 | GPIO_OSPEEDER_OSPEEDR7_1;
    // Set AF5 (SPI1) for the pins
    GPIOA->AFR[0] |= (5 << GPIO_AFRL_AFSEL4_Pos) | (5 << GPIO_AFRL_AFSEL5_Pos) | (5 << GPIO_AFRL_AFSEL6_Pos) | (5 << GPIO_AFRL_AFSEL7_Pos);

    // --- SPI2 (GPIOB) Pin Configuration ---
    // Set PB12 (NSS), PB13 (SCK), PB14 (MISO), PB15 (MOSI) to Alternate Function mode
    GPIOB->MODER |= GPIO_MODER_MODE12_1 | GPIO_MODER_MODE13_1 | GPIO_MODER_MODE14_1 | GPIO_MODER_MODE15_1;
    // Set pull-up for MISO and NSS
    GPIOB->PUPDR |= GPIO_PUPDR_PUPD14_1 | GPIO_PUPDR_PUPD12_0;
    // Set high speed for SPI pins
    GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR12_1 | GPIO_OSPEEDER_OSPEEDR13_1 | GPIO_OSPEEDER_OSPEEDR14_1 | GPIO_OSPEEDER_OSPEEDR15_1;
    // Set AF5 (SPI2) for the pins
    GPIOB->AFR[1] |= (5 << GPIO_AFRH_AFSEL12_Pos) | (5 << GPIO_AFRH_AFSEL13_Pos) | (5 << GPIO_AFRH_AFSEL14_Pos) | (5 << GPIO_AFRH_AFSEL15_Pos);

    // --- UART1 (GPIOA) Pin Configuration ---
    // Set PA9 (TX), PA10 (RX) to Alternate Function mode and high speed
    GPIOA->MODER |= (0xA << 18);            // Alternate function for PA9, PA10
    GPIOA->OSPEEDR |= (0xA << 18);          // High speed for PA9, PA10
    GPIOA->AFR[1] |= (0x77 << 4);           // AF7 (USART1) for PA9, PA10
}

/***************************************** Timer11 Configuration ***************************************
 * @brief  Configures TIM11 for delay generation (microsecond/millisecond delays).
 *         Sets prescaler, auto-reload, and enables the timer.
 */
static void timerConfig()
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM11EN;            // Enable TIM11 clock

    TIM11->PSC = PSC_VALUE;                         // Set prescaler for desired timer frequency
    TIM11->ARR = ARR_VALUE;                         // Set auto-reload (max count)
    TIM11->CR1 |= 0x1;                              // Enable counter

    while (!(TIM11->SR & 1));                       // Wait for update event flag
}

/***************************************** Microsecond Delay *******************************************
 * @brief  Generates a delay in microseconds using TIM11.
 *
 * @param  us: Number of microseconds to delay
 */
void Delay_us(uint16_t us)
{
    TIM11->CNT = 0;                                 // Reset counter
    while (TIM11->CNT < us);
}

/***************************************** Millisecond Delay *******************************************
 * @brief  Generates a delay in milliseconds using TIM11.
 *
 * @param  ms: Number of milliseconds to delay
 */
void Delay_ms(uint16_t ms)
{
    for (uint16_t i=0; i<ms; i++)
    {
        Delay_us(1000);                             // 1ms = 1000us
    }
}

/************************************** System Initialization *******************************************
 * @brief  Initializes system clock, GPIO, and timer.
 *         Call this in main() before peripheral initialization.
 */
void SystemInit(void)
{
    SysClockConfig_42Mhz();                         // Configure system clock to 42MHz
    gpioConfig();                                   // Configure GPIOA/B for SPI and UART
    timerConfig();                                  // Configure TIM11 for delays
}

/************************************ SystemCoreClockUpdate ********************************************
 * @brief  Stub for updating SystemCoreClock variable (if used).
 */
void SystemCoreClockUpdate(void)
{
    // Typically updates SystemCoreClock variable, if used by CMSIS or HAL
}