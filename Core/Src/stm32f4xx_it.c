/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SPEED_TOLERANCE 10   // +-25 TIM2 ticks (25 us at 1 MHz)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_adc1;
extern DMA_HandleTypeDef hdma_adc2;
/* USER CODE BEGIN EV */
extern volatile print_state_t state;
extern volatile uint32_t edge_count;
extern volatile uint32_t t_start;
extern volatile uint32_t t_end;
extern volatile uint32_t dt_ticks;
extern volatile dir_t current_dir;
extern uint16_t arr;
extern TIM_HandleTypeDef htim9;
extern volatile uint32_t last_edge_time;
extern volatile uint32_t expected_dt;
extern volatile uint8_t edge_sample_state;
extern volatile uint32_t edge_idx_after_idx;
extern volatile uint8_t adc_armed;
extern volatile uint8_t adc_capture_done;
extern volatile uint32_t adc_swstart_count;
/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles DMA2 stream0 global interrupt.
  */
void DMA2_Stream0_IRQHandler(void)
{
    if (DMA2->LISR & DMA_LISR_TCIF0) {
        DMA2->LIFCR = DMA_LIFCR_CTCIF0;
        adc_capture_done |= 0x1;        
    }
}

/**
  * @brief This function handles DMA2 stream2 global interrupt.
  */
void DMA2_Stream2_IRQHandler(void)
{
    if (DMA2->LISR & DMA_LISR_TCIF2) {
        DMA2->LIFCR = DMA_LIFCR_CTCIF2;
        adc_capture_done |= 0x2;        
    }
}

/* USER CODE BEGIN 1 */
void EXTI0_IRQHandler(void)
{
    GPIOG->BSRR = GPIO_PIN_2;

    if (state == IDLE)
    {
        edge_count = 0;

        __HAL_TIM_SET_AUTORELOAD(&htim9, arr);
        __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_2, (arr + 1) / 2);
        __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, 0);

        state = WAIT_FIRST_EDGE;
    }

    EXTI->PR = 0x01;
    GPIOG->BSRR = (GPIO_PIN_2 << 16);
}

void EXTI4_IRQHandler(void)
{
    GPIOG->BSRR = GPIO_PIN_2;

    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;

    if (state == WAIT_FIRST_EDGE || state == PRINTING)
    {
        uint32_t idr = GPIOD->IDR;
        uint8_t a_level = (idr >> 4) & 1;
        uint8_t b_level = (idr >> 5) & 1;

        dir_t new_dir;
        if (a_level == 1) {                  
            new_dir = (b_level == 0) ? DIR_R2L : DIR_L2R;
        } else {                               
            new_dir = (b_level == 0) ? DIR_L2R : DIR_R2L;
        }

        if (state == WAIT_FIRST_EDGE)
        {
            if (new_dir != DIR_R2L)
            {
                state = DONE_ERR;
                adc_armed = 0;
            }
            else
            {
                t_start = TIM2->CNT;
                edge_count  = 1;
                current_dir = DIR_R2L;
                state = PRINTING;
                last_edge_time = t_start;
                edge_sample_state = 1;
                if (adc_armed) {
                    ADC1->CR2 |= ADC_CR2_SWSTART;
                    ADC2->CR2 |= ADC_CR2_SWSTART;
                    adc_swstart_count++;
                }
                edge_idx_after_idx = 1;
            }
        }
        else
        {
            if (new_dir != DIR_R2L)
            {
                state = DONE_ERR;
                adc_armed = 0;
            }
            else
            {
                uint32_t now = TIM2->CNT;
                uint32_t delta = now - last_edge_time;

                if (edge_sample_state == 1)
                {
                    expected_dt = delta;
                    edge_sample_state = 2;
                }
                else if (edge_sample_state == 2)
                {
                    if (delta > expected_dt + SPEED_TOLERANCE ||
                        delta < expected_dt - SPEED_TOLERANCE)
                    {
                        state = DONE_ERR;
                        adc_armed = 0;
                    }
                }
                last_edge_time = now;

                if (state == PRINTING)
                {
                    edge_count++;
                    if (adc_armed && (edge_idx_after_idx & 0x3) == 0) {
                        ADC1->CR2 |= ADC_CR2_SWSTART;
                        ADC2->CR2 |= ADC_CR2_SWSTART;
                        adc_swstart_count++;
                    }
                    edge_idx_after_idx++;
                    if (edge_count == 12000)
                    {
                        t_end = TIM2->CNT;
                        state = DONE_OK;
                    }
                }
            }
        }
    }
    if (state == RETURN_TRIP)
    {
        edge_count++;
        if (edge_count == 12000)
        {
            state = IDLE;
        }
    }

    EXTI->PR = 0x10;
    GPIOG->BSRR = (GPIO_PIN_2 << 16);
}

void EXTI9_5_IRQHandler(void)
{
    GPIOG->BSRR = GPIO_PIN_2;

    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;

    if (state == WAIT_FIRST_EDGE || state == PRINTING)
    {
        uint32_t idr = GPIOD->IDR;
        uint8_t a_level = (idr >> 4) & 1;
        uint8_t b_level = (idr >> 5) & 1;

        dir_t new_dir;
        if (b_level == 1) {                    
            new_dir = (a_level == 0) ? DIR_L2R : DIR_R2L;
        } else {                                
            new_dir = (a_level == 0) ? DIR_R2L : DIR_L2R;
        }

        if (state == WAIT_FIRST_EDGE)
        {
            if (new_dir != DIR_R2L)
            {
                state = DONE_ERR;
                adc_armed = 0;
            }
            else
            {
                t_start = TIM2->CNT;
                edge_count = 1;
                current_dir = DIR_R2L;
                state = PRINTING;
                last_edge_time = t_start;
                edge_sample_state = 1;
                if (adc_armed) {
                    ADC1->CR2 |= ADC_CR2_SWSTART;
                    ADC2->CR2 |= ADC_CR2_SWSTART;
                    adc_swstart_count++;
                }
                edge_idx_after_idx = 1;
            }
        }
        else
        {
            if (new_dir != DIR_R2L)
            {
                state = DONE_ERR;
                adc_armed = 0;
            }
            else
            {
                uint32_t now = TIM2->CNT;
                uint32_t delta = now - last_edge_time;

                if (edge_sample_state == 1)
                {
                    expected_dt = delta;
                    edge_sample_state = 2;
                }
                else if (edge_sample_state == 2)
                {
                    if (delta > expected_dt + SPEED_TOLERANCE ||
                        delta < expected_dt - SPEED_TOLERANCE)
                    {
                        state = DONE_ERR;
                        adc_armed = 0;
                    }
                }
                last_edge_time = now;

                if (state == PRINTING)
                {
                    edge_count++;
                    if (adc_armed && (edge_idx_after_idx & 0x3) == 0) {
                        ADC1->CR2 |= ADC_CR2_SWSTART;
                        ADC2->CR2 |= ADC_CR2_SWSTART;
                        adc_swstart_count++;
                    }
                    edge_idx_after_idx++;
                    if (edge_count == 12000)
                    {
                        t_end = TIM2->CNT;
                        state = DONE_OK;
                    }
                }
            }
        }
    }
    if (state == RETURN_TRIP)
    {
        edge_count++;
        if (edge_count == 12000)
        {
            state = IDLE;
        }
    }

    EXTI->PR = 0x20;
    GPIOG->BSRR = (GPIO_PIN_2 << 16);
}
/* USER CODE END 1 */
