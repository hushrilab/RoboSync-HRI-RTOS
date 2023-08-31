/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "main.h" //various function declarations. The definitions are in util.c
#include <stdio.h>
#include <stdint.h>
#include "kernel.h"

int x = 0;
void print_continuously(void* args)
{
	int inArg = *(int*)args;
	while(1)
	{
		printf("Hello, PC!%d\r\n",inArg+x);
		//osYield();
	}
}

void print_again(void* args)
{
	while(1)
	{
		x++;
		osYield();
	}
}


int main(void)
{

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  osKernelInit();
  int x = 47;
  osCreateThreadWithDeadline(print_continuously,&x,200);
  osCreateThread(print_again,0);
  osKernelStart();

  while(1){}

}

