/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
__IO uint32_t freq = 1;
__IO DataPacket dataPacket; // Initialize with start and end bytes

__IO uint32_t counter = 0;
void SysTick_Handler(void) {
  if (counter>0) {
    counter--;
  }
}
void SysTick_Delay(uint32_t ms) {
  counter = ms;
  while (counter > 0);
}

int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
  NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));
  SystemClock_Config();
  MX_GPIO_Init();
  /////////////////////
  // User code below //
  /////////////////////
  #define SysTickFrequency 1000 // 1 ms tick
  SysTick->LOAD = 100000000 / SysTickFrequency - 1;
  SysTick->VAL = 0; // Clear current value
  SysTick->CTRL = 0b111; // Enable SysTick with processor clock and interrupts
  // define pins and ports
  #define LED_PORT GPIOC
  #define LED_PIN 13
  // RCC config
  RCC->AHB1ENR |= 0b111;          // Enable GPIOA, GPIOB, GPIOC
  RCC->AHB1ENR |= 0b1 << 22;      // Enable DMA2
  RCC->APB1ENR |= 0b1111;         // Enable TIM2, TIM3, TIM4, TIM5
  RCC->APB2ENR |= 0b1 << 4;       // Enable USART1
  // IO config
  LED_PORT->MODER |= 1 << 2*LED_PIN;    // Set LED pin to output
  LED_PORT->OTYPER &= ~(1 << LED_PIN);  // Set LED pin to push-pull
  // Alternate function pins config
  GPIOA->MODER |= 2 << 2*6;       // PA6  alternate function
  GPIOA->MODER |= 2 << 2*7;       // PA7  alternate function
  GPIOA->MODER |= 2 << 2*9;       // PA9  alternate function
  GPIOA->MODER |= 2 << 2*10;      // PA10 alternate function
  GPIOB->MODER |= 2 << 2*6;       // PB6  alternate function                                                                                                                                   
  GPIOB->MODER |= 2 << 2*7;       // PB7  alternate function
  // Alternate function config
  GPIOA->AFR[0] = 0;              // Clear GPIOA AFRL
  GPIOA->AFR[0] |= 2 << 4*6;       // PA6 AF2 (TIM3 CH1)
  GPIOA->AFR[0] |= 2 << 4*7;       // PA7 AF2 (TIM3 CH2)
  GPIOA->AFR[1] = 0;              // Clear GPIOA AFRH
  GPIOA->AFR[1] |= 7 << (9-8)*4;        // PA9 AF7 (USART1 TX)
  GPIOA->AFR[1] |= 7 << (10-8)*4;        // PA10 AF7 (USART1 RX)
  GPIOB->AFR[0] = 0;              // Clear GPIOB AFRL
  GPIOB->AFR[0] |= 2 << 4*6;       // PB6 AF2 (TIM4 CH1)
  GPIOB->AFR[0] |= 2 << 4*7;       // PB7 AF2 (TIM4 CH2)
  // USART config
  #define USART1_BAUDRATE 921600
  #define USART1_CLOCK 100000000  // APB2 clock
  uint32_t mantissa = USART1_CLOCK / (16 * USART1_BAUDRATE);
  uint32_t fraction = (uint32_t)((((float)USART1_CLOCK / (16 * USART1_BAUDRATE)) - mantissa) * 16 + 0.5f);
  USART1->BRR = (mantissa << 4) | (fraction & 0x0F);
  USART1->CR1 = 0;  // Reset CR1
  USART1->CR1 |= (1 << 2);        // Enable TX (bit 3)
  USART1->CR1 |= (1 << 3);        // Enable RX (bit 2)
  USART1->CR1 |= (1 << 13);       // Enable USART (bit 13)
  USART1->CR3 = 0;  // Reset CR3
  USART1->CR3 |= (1 << 7);        // Enable DMA for TX (bit 7)
  // DMA config for USART1 TX
  DMA2_Stream7->CR = 0;           // Reset CR and stop DMA2 Stream 7
  DMA2_Stream7->CR |= 4 << 25;    // Channel 4
  DMA2_Stream7->CR |= 0b1 << 6;   // Memory-to-peripheral (DIR bit 6)
  DMA2_Stream7->CR |= 0b1 << 10;  // Enable memory increment
  DMA2_Stream7->CR |= 0b1 << 4;   // Enable transfer complete interrupt (TCIE)
  DMA2_Stream7->PAR = (uint32_t)&USART1->DR; // Peripheral address
  DMA2_Stream7->M0AR = (uint32_t)&dataPacket; // Memory address
  DMA2_Stream7->NDTR = sizeof(DataPacket); // Number of data items to transfer
  DMA2_Stream7->CR |= 0b1;        // Enable DMA2 Stream
  // Timer2 config
  TIM2->PSC = 0;          // Prescaler
  TIM2->ARR = ~(uint32_t)0;
  // Timer3 config (For PWM output to control throttle)
  TIM3->PSC = 100 - 1;   // Prescaler (Freq = 1 MHz)
  TIM3->ARR = 1000 - 1; // Auto-reload (Period = 1 ms)
  TIM3->CCMR1 |= 6 << 4; // Output compare mode: PWM mode 1 (OC1M = 110)
  TIM3->CCER |= 0b1;      // Enable CH1 output (CC1E bit)
  TIM3->CCMR1 |= 6 << 12; // Output compare mode: PWM mode 1 (OC2M = 110)
  TIM3->CCER |= 0b1 << 4; // Enable CH2 output (CC2E bit)
  TIM3->CCR1 = 600;        // Initial duty cycle 0% for CH1
  TIM3->CCR2 = 0;        // Initial duty cycle 0% for CH2
  TIM3->CR1 |= 0b1;     // Enable TIM3 (CEN bit)
  // Timer4 config
  TIM4->PSC = 100-1;           // Prescaler (1us per cycle)
  TIM4->ARR = 50000-1;          // Auto-reload
  // config for trig:
  TIM4->CCMR1 &= 0xffff;        // Clear CCMR1
  TIM4->CCR1 = 50000-100-1; // Capture/Compare register 1 (10 us before ARR)
  // TIM4->CCR1 = 50000-10000-1; // Capture/Compare register 1 (10 ms before ARR)
  TIM4->CCMR1 |= 6 << 4;   // Output compare mode: toggle on match (OC1M = 110)   
  TIM4->CCER |= 0b1;        // Enable CH1 output (CC1E bit)
  // config for echo:
  TIM4->CCMR1 |= 0b1 << 8;   // Input capture on CH2 (TI2)
  TIM4->SMCR |= 6 << 4;     // Trigger on TI2FP2
  TIM4->SMCR |= 4;          // Reset mode
  TIM4->CCER |= 0b1 << 5;   // Capture on falling edge for CH2 (CC2P bit)
  TIM4->CCER |= 0b1 << 4;   // Enable CH2 capture (CC2E bit)
  // TIM4->DIER |= 0b1 << 10;  // Enable trigger interrupt (TIE bit)
  TIM4->DIER |= 0b1 << 2;   // Enable capture/compare 2 interrupt (CC2IE bit)
  TIM4->DIER |= 0b1 << 0;   // Enable capture/compare 2 interrupt (CC2IE bit)
  TIM4->CR1 |= 0b1;         // Enable TIM4 (CEN bit)
  // Timer5 config
  TIM5->PSC = 100000 - 1;          // Prescaler 1kHz
  TIM5->ARR = 50 - 1;          // Auto-reload
  TIM5->DIER |= 0b1;             // Enable update interrupt
  TIM5->CR1 |= 0b1;              // Enable timer
  /// Timer9 config (for calculating sonar sample rate each second)
  TIM9->PSC = 10000 - 1;          // Prescaler
  TIM9->ARR = 1000 - 1;          // Auto-reload
  TIM9->CR1 |= 0b1;              // Enable timer
  // Interrupt config
  NVIC_SetPriority(DMA2_Stream7_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 4, 0));
  NVIC_EnableIRQ(DMA2_Stream7_IRQn); // Enable DMA2 Stream 7 interrupt in NVIC
  NVIC_SetPriority(TIM4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 1));
  NVIC_EnableIRQ(TIM4_IRQn);      // Enable TIM4 interrupt
  NVIC_SetPriority(TIM5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
  NVIC_EnableIRQ(TIM5_IRQn);      // Enable TIM5 interrupt
  // Program init
  dataPacket.start_bytes[0] = 0xAF;
  dataPacket.start_bytes[1] = 0xFA;
  dataPacket.start_bytes[2] = 0x55;
  dataPacket.end_bytes[0] = 0x77;
  dataPacket.end_bytes[1] = 0xAA;
  dataPacket.steering = 0;
  dataPacket.user_throttle = 0;
  dataPacket.true_throttle = 0;
  dataPacket.brake = 0;
  dataPacket.speed = 0;
  dataPacket.PWM1 = 0;
  dataPacket.PWM2 = 0;
  dataPacket.distance = 0;
  /* Infinite loop */
  while (1)
  {
    // LED_PORT->ODR ^= 1 << LED_PIN; // Toggle LED
    // SysTick_Delay(500); // Delay 500 ms
  }
}

// void sendChar(char c) {
//   while ((USART1->SR & (1 << 7)) == 0); // Wait until TXE (Transmit Data Register Empty) is set
//   USART1->DR=c;
// }

// void sendString(const char* str) {
//   while (*str) {
//     sendChar(*str++);
//   }
// }

void motorWrite(int value) {
  if (value > 1000) value = 1000;
  if (value < -1000) value = -1000;
  if (value >= 0) {
    TIM3->CCR1 = value; // Set duty cycle for CH1
    TIM3->CCR2 = 0;     // Ensure CH2 is off
  } else {
    TIM3->CCR1 = 0;     // Ensure CH1 is off
    TIM3->CCR2 = -value; // Set duty cycle for CH2 (negative value)
  }
}

void sendPacket() {
  while ((USART1->SR & (1 << 6)) == 0);       // Wait for TC flag (transmission complete)
  DMA2_Stream7->CR &= ~0b1;                   // Disable DMA2 Stream 7
  while (DMA2_Stream7->CR & 0b1);             // Wait for EN bit to be cleared (DMA disabled)
  DMA2->HIFCR |= 0b111101001;                 // Clear all flags (DMA2 Stream 7)
  DMA2_Stream7->NDTR = sizeof(DataPacket);    // Reset NDTR to transfer data
  DMA2_Stream7->CR |= 0b1;                    // Re-enable DMA2 Stream 7
}

void TIM4_IRQHandler(void) {
  if (TIM4->SR & 0b100) { // Check capture/compare 2 interrupt flag
    dataPacket.distance = TIM4->CCR2; // Read captured value
    LED_PORT->ODR ^= 1 << LED_PIN; // Toggle LED
    TIM4->SR &= ~0b100; // Clear capture/compare 2 interrupt flag
  }
}

void TIM5_IRQHandler(void) {
  if (TIM5->SR & 0b1) {
    // LED_PORT->ODR ^= 1 << LED_PIN; // Toggle LED
    sendPacket();
    TIM5->SR &= ~0b1; // Clear update interrupt flag
    }
}

uint32_t sonar_sample_count = 0;
void TIM9_IRQHandler(void) {
  if (TIM9->SR & 0b1) {
    sonar_sample_count = 0; // Reset sample count every second
    TIM9->SR &= ~0b1; // Clear update interrupt flag
    }
}

void DMA2_Stream7_IRQHandler(void) {
  // Check for transfer complete interrupt flag (bit 27 for Stream 7 in HISR)
  if ((DMA2->HISR & (1 << 27))) {
    // Clear the transfer complete flag
    DMA2->HIFCR |= (1 << 27);
    // Optional: Add code to handle post-transfer tasks
  }
  
  // Check for error flags and clear them
  if ((DMA2->HISR & (1 << 25))) {  // Check TE flag
    DMA2->HIFCR |= (1 << 25);
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_3)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_HSE_EnableCSS();
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 400, LL_RCC_PLLP_DIV_4);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  while (LL_PWR_IsActiveFlag_VOS() == 0)
  {
  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_Init1msTick(100000000);
  LL_SetSystemCoreClock(100000000);
  LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOH);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
