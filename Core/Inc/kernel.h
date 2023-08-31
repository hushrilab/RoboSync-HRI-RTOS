/*
 * kernel.h
 *
 *  Created on: Mar. 13, 2023
 *      Author: mstachow
 */

#ifndef SRC_KERNEL_H_
#define SRC_KERNEL_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

//Define the assembly functions as extern
extern void SVC_Handler(void);
extern void runFirstThread(void);
extern void PendSV_Handler(void);

//Interrupt priority registers
//Registers used for interrupts
#define SHPR2 *(uint32_t*)0xE000ED1C //for setting SVC priority, bits 31-24
#define SHPR3 *(uint32_t*)0xE000ED20 //systick is bits 31-24, PendSV is bits 23-16
#define _ICSR *(uint32_t*)0xE000ED04

#define STACK_POOL_SIZE 0x4000
#define THREAD_STACK_SIZE 0x800

//stack alignment stuff
#define EIGHT_BYTE_ALIGN 8
#define FIX_ALIGNMENT 4

//system call numbers
#define RUN_FIRST_THREAD 0
#define YIELD 1

//timing
#define ROUND_ROBIN_TIMEOUT 5

//Thread struct
typedef struct k_thread{
	uint32_t* sp;
	void (*fcn)(void*);
	uint32_t deadline;
	uint32_t thread_runtime;
}thread;


uint32_t* getNewThreadStack(); //allocates a new stack pointer, or returns NULL
uint32_t* getMSPInitialLocation();
void osKernelInit();
void osKernelStart();
bool osCreateThread(void (*fcn_ptr)(void*),void* args);
bool osCreateThreadWithDeadline(void (*fcn_ptr)(void*),void* args,uint32_t deadline);
//Notice how my stack allocation function is not here, since it is internal and not part of the user API

void osYield(); //voluntary yield

#endif /* SRC_KERNEL_H_ */
