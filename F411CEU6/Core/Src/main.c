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
#define setFreq2(f) TIM2->ARR = (uint32_t)(50000/f) - 1;

__IO uint32_t freq = 1;

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

  // define pins and ports
  #define LED_PORT GPIOC
  #define LED_PIN 13
  #define KEY_PORT GPIOA
  #define KEY_PIN 0
  #define TRIG_PORT GPIOB
  #define TRIG_PIN 7
  #define ECHO_PORT GPIOB
  #define ECHO_PIN 6
  // define timer
  #define BLINK_TIMER TIM10
  #define BLINK_TIMER_IRQn TIM1_UP_TIM10_IRQn
  #define SONAR_TIMER TIM4
  #define SONAR_TIMER_IRQn TIM4_IRQn
  // RCC config
  RCC->AHB1ENR |= 0b111;
  RCC->APB1ENR |= 0b1111;
  RCC->APB2ENR |= 0b1 | 0b1 << 5 | 0b101 << 12 | 0b111 << 16;
  // IO config
  KEY_PORT->MODER |= 0b0 << 2*KEY_PIN;
  KEY_PORT->PUPDR |= 0b1 << 2*KEY_PIN;
  LED_PORT->MODER |= 0b1 << 2*LED_PIN;
  LED_PORT->OTYPER &= ~(1 << LED_PIN);
  LED_PORT->ODR |= 1 << LED_PIN;
  // Enable EXTI0 interrupt
  SYSCFG->EXTICR[0] |= 0b0000; // PA0
  EXTI->IMR |= 1 << 0; // Unmask EXTI0
  EXTI->FTSR |= 1 << 0; // Falling edge trigger
  NVIC_EnableIRQ(EXTI0_IRQn);
  // Timer config
  // TIM2
  BLINK_TIMER->PSC = 1000 - 1; // 100kHz
  BLINK_TIMER->ARR = 50000 - 1; // 1Hz 50% PWM
  BLINK_TIMER->DIER |= 0b1; // Enable update interrupt
  BLINK_TIMER->CR1 |= 0b1; // Enable timer
  // BLINK_TIMER->CR1 |= 0b1 << 4; // Downcounting mode
  NVIC_SetPriority(BLINK_TIMER_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 4, 0));
  NVIC_EnableIRQ(BLINK_TIMER_IRQn);
  // SONAR_TIMER
  SONAR_TIMER->PSC = 100 - 1; // 1MHz
  SONAR_TIMER->ARR = 60000 - 1; // 60ms auto-reload
  SONAR_TIMER->CR1 |= 0b1; // Enable timer

  /* Infinite loop */
  while (1)
  {
  }
}
void EXTI0_IRQHandler(void) {
  if (EXTI->PR & 0b1) {
    freq++;
    setFreq2(freq);
    TIM2->EGR |= 0b1; // Generate update event to apply new ARR value immediately
    EXTI->PR |= 1 << 0; // Clear pending bit
  }
}

void TIM1_UP_TIM10_IRQHandler(void) {
  if (TIM10->SR & 0b1) {
    TIM10->SR &= ~0b1; // Clear update interrupt flag
    GPIOC->ODR ^= 0b1 << 13;
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
