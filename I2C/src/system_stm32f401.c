#include "main.h"

#define PLLM            (25U)                   // PLLM division factor for PLL input clock
#define PLLN            (252U << 6)             // PLLN multiplication factor for VCO
#define PLLP            (1U << 17)              // PLLP division factor for main system clock
#define PSC_VALUE       41                      // Timer prescaler value for TIM11
#define ARR_VALUE       0xFFFF                  // Timer auto-reload value for TIM11

/************************************* System Clock Configuration **************************************
 * Configures the system clock to 42MHz using HSE and PLL.
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
 * Configures GPIOB pins for I2C1 (PB6, PB7) and I2C2 (PB8, PB9) alternate function.
 */
static void gpioConfig()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;            // Enable GPIOB clock

    // --- I2C1: PB6 (SCL), PB7 (SDA) ---
    GPIOB->MODER &= ~((3U << GPIO_MODER_MODER6_Pos) | (3U << GPIO_MODER_MODER7_Pos));       // Clear mode
    GPIOB->MODER |= (2U << GPIO_MODER_MODER6_Pos) | (2U << GPIO_MODER_MODER7_Pos);          // Set AF mode
    GPIOB->OTYPER |= GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7;                                     // Open-drain
    GPIOB->OSPEEDR |= (3U << GPIO_OSPEEDR_OSPEED6_Pos) | (3U << GPIO_OSPEEDR_OSPEED7_Pos);  // High speed
    GPIOB->PUPDR &= ~((3U << GPIO_PUPDR_PUPD6_Pos) | (3U << GPIO_PUPDR_PUPD7_Pos));         // No pull
    GPIOB->AFR[0] |= (4U << GPIO_AFRL_AFSEL6_Pos) | (4U << GPIO_AFRL_AFSEL7_Pos);           // AF4

    // --- I2C2: PB8 (SCL), PB9 (SDA) ---
    GPIOB->MODER &= ~((3U << GPIO_MODER_MODER8_Pos) | (3U << GPIO_MODER_MODER9_Pos));       // Clear mode
    GPIOB->MODER |= (2U << GPIO_MODER_MODER8_Pos) | (2U << GPIO_MODER_MODER9_Pos);          // Set AF mode
    GPIOB->OTYPER |= GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9;                                     // Open-drain
    GPIOB->OSPEEDR |= (3U << GPIO_OSPEEDR_OSPEED8_Pos) | (3U << GPIO_OSPEEDR_OSPEED9_Pos);  // High speed
    GPIOB->PUPDR &= ~((3U << GPIO_PUPDR_PUPD8_Pos) | (3U << GPIO_PUPDR_PUPD9_Pos));         // No pull
    GPIOB->AFR[1] |= (4U << GPIO_AFRH_AFSEL8_Pos) | (4U << GPIO_AFRH_AFSEL9_Pos);           // AF4
}

/***************************************** Timer11 Configuration ***************************************
 * Configures TIM11 for delay functions.
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
 * Generates a delay in microseconds using TIM11.
 */
void Delay_us(uint16_t us)
{
    TIM11->CNT = 0;                                 // Reset counter
    while (TIM11->CNT < us);
}

/***************************************** Millisecond Delay *******************************************
 * Generates a delay in milliseconds using TIM11.
 */
void Delay_ms(uint16_t ms)
{
    for (uint16_t i=0; i<ms; i++)
    {
        Delay_us(1000);                             // 1ms = 1000us
    }
}

/************************************** System Initialization *******************************************
 * Initializes system clock, GPIO, and timer.
 */
void SystemInit(void)
{
    SysClockConfig_42Mhz();                         // Configure system clock to 42MHz
    gpioConfig();                                   // Configure GPIOB for I2C
    timerConfig();                                  // Configure TIM11 for delays
}

void SystemCoreClockUpdate(void)
{
    // Typically updates SystemCoreClock variable, if used
}