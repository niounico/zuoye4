/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
#include "usart.h"

/* USER CODE BEGIN 0 */
#include <stdatomic.h>

#define USART1_RX_BUFFER_SIZE 256U

/* USART1 IRQ writes the head. The main loop writes the tail. */
_Static_assert(ATOMIC_SHORT_LOCK_FREE == 2, "USART1 RX indices must be lock-free");
static atomic_ushort rxHead;
static atomic_ushort rxTail;
static uint8_t rxBuffer[USART1_RX_BUFFER_SIZE];
static uint8_t rxByte;

static uint16_t USART1_NextRxIndex(uint16_t index)
{
  return (uint16_t)((index + 1U) % USART1_RX_BUFFER_SIZE);
}

/* USER CODE END 0 */

UART_HandleTypeDef huart1;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  if (HAL_UART_Receive_IT(&huart1, &rxByte, 1U) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
uint16_t USART1_ReadRx(uint8_t *data, uint16_t capacity)
{
  if (data == NULL)
  {
    return 0U;
  }

  uint16_t tail = atomic_load_explicit(&rxTail, memory_order_relaxed);
  const uint16_t head = atomic_load_explicit(&rxHead, memory_order_acquire);
  uint16_t count = 0U;

  while (tail != head && count < capacity)
  {
    data[count++] = rxBuffer[tail];
    tail = USART1_NextRxIndex(tail);
  }
  atomic_store_explicit(&rxTail, tail, memory_order_release);

  return count;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart != &huart1 || huart->ErrorCode != HAL_UART_ERROR_NONE)
  {
    return;
  }

  const uint16_t head = atomic_load_explicit(&rxHead, memory_order_relaxed);
  const uint16_t next = USART1_NextRxIndex(head);

  /* Keep unread bytes in order when the buffer is full. */
  if (next != atomic_load_explicit(&rxTail, memory_order_acquire))
  {
    rxBuffer[head] = rxByte;
    atomic_store_explicit(&rxHead, next, memory_order_release);
  }

  (void)HAL_UART_Receive_IT(huart, &rxByte, 1U);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart != &huart1)
  {
    return;
  }

  /* On STM32F1, reading SR then DR clears the receive error flags. */
  const uint32_t errorFlags = USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE;
  if ((huart->Instance->SR & errorFlags) != 0U)
  {
    __HAL_UART_CLEAR_OREFLAG(huart);
  }

  if (huart->RxState == HAL_UART_STATE_READY)
  {
    (void)HAL_UART_Receive_IT(huart, &rxByte, 1U);
  }
}

/* USER CODE END 1 */
