#include "kernel.h"
#include <stdio.h>

//Kernel internal data structures
int curThread;
int stacksAllocated;
bool kernelStarted = false;

thread threadArray[STACK_POOL_SIZE/THREAD_STACK_SIZE];

/*
	Obtains the initial location of MSP by looking it up in the vector table.
	Remember the vector table starts at address 0 and that is where MSP is
*/
uint32_t* getMSPInitialLocation()
{
	uint32_t* MSR_Original = 0; //0 being the pointer location
	return (uint32_t*)*MSR_Original;
}
/*
	Returns the address of a new PSP with the offset "offset" bytes from MSP. Remember,
	the stack grows downwards, so this offset must be subtracted from MSP, not added

	I am also ensuring that the stack is divisible by 8, which is required by the ARM
	Cortex (and is why PRESERVE8 is in our assembly file)
*/
uint32_t* getNewThreadStack()
{
	//generate the offset and return NULL if too many stacks are allocated
	int offset = (++stacksAllocated)*THREAD_STACK_SIZE; //automatically increments
	if(offset >= STACK_POOL_SIZE)
		return (uint32_t*)0;

	uint32_t MSPOriginal = (uint32_t)getMSPInitialLocation();

	uint32_t newStackLocation = MSPOriginal-offset;
	if(newStackLocation%EIGHT_BYTE_ALIGN)
		newStackLocation+=FIX_ALIGNMENT;
	return (uint32_t*)(MSPOriginal-offset);
}

//Sets up the kernel's internal data structures
void osKernelInit()
{
	//set the priority of PendSV to almost the weakest
	SHPR3 |= 0xFE << 16;
	SHPR3 |= 0xFFU << 24; //Set the priority of SysTick to be the weakest

	SHPR2 |= 0xFDU << 24; //Set the priority of SVC the be the strongest of the three
	curThread = 0;
	stacksAllocated = 0;
}

//start the kernel via a system call
void osKernelStart()
{
	kernelStarted = true;
	__asm("SVC #0"); //asm is not subject to preprocessor macros
}

//initializes a stack. Now that there are two createThread functions, it makes sense to compartmentalize
uint32_t* osInitializeThreadStack(void (*fcn_ptr)(void*),void* args)
{
	//get a stack pointer
		uint32_t* stackPtr = getNewThreadStack();
		if(stackPtr == 0)
			return 0; //No stacks!

		//Otherwise, rock this
		*(--stackPtr) = 1<<24; //xpsr
		*(--stackPtr) = (uint32_t)fcn_ptr;
		*(--stackPtr) = 0xE;
		*(--stackPtr) = 0xC;
		*(--stackPtr) = 0x3;
		*(--stackPtr) = 0x2;
		*(--stackPtr) = 0x1;
		*(--stackPtr) = (uint32_t)args; //This is R0

		*(--stackPtr) = 0xE;
		*(--stackPtr) = 0xE;
		*(--stackPtr) = 0xE;
		*(--stackPtr) = 0xE;
		*(--stackPtr) = 0xE;
		*(--stackPtr) = 0xE;
		*(--stackPtr) = 0xE;
		*(--stackPtr) = 0xE;

		return stackPtr;

}

//Sets up the thread stack to run
bool osCreateThread(void (*fcn_ptr)(void*),void* args)
{
	//get a stack pointer
	uint32_t* stackPtr = osInitializeThreadStack(fcn_ptr,args);
	if(stackPtr == 0)
		return false; //No stacks!

	threadArray[stacksAllocated-1].fcn = fcn_ptr;
	threadArray[stacksAllocated-1].sp = stackPtr;
	threadArray[stacksAllocated-1].deadline = ROUND_ROBIN_TIMEOUT;
	threadArray[stacksAllocated-1].thread_runtime = ROUND_ROBIN_TIMEOUT;

	return true;
}


//Sets up the thread stack to run
bool osCreateThreadWithDeadline(void (*fcn_ptr)(void*),void* args,uint32_t deadlineMS)
{
	//get a stack pointer
		uint32_t* stackPtr = osInitializeThreadStack(fcn_ptr,args);
		if(stackPtr == 0)
			return false; //No stacks!

	threadArray[stacksAllocated-1].fcn = fcn_ptr;
	threadArray[stacksAllocated-1].sp = stackPtr;
	threadArray[stacksAllocated-1].deadline = deadlineMS;
	threadArray[stacksAllocated-1].thread_runtime = deadlineMS;

	return true;
}
void osYield()
{
	__asm("SVC #1");
}

void osSched()
{
	//Don't do anything if there is only one thread running
	if(stacksAllocated > 1)
	{
		//Save where we restore PSP to later on
		threadArray[curThread].sp = (uint32_t*)(__get_PSP() - 8*4);

		curThread = (curThread+1)%stacksAllocated;
		__set_PSP((uint32_t)threadArray[curThread].sp);
	}
}

//This is super duper not exposed to the user, so it's not in kernel.h
void SVC_Handler_Main( unsigned int *svc_args )
{
  //unsigned int svc_number;
  uint8_t *pInstruction = (uint8_t*)(svc_args[6]);
  pInstruction -= 2;
  uint32_t svc_number = *pInstruction;
  /*
  * Stack contains:
  * r0, r1, r2, r3, r12, r14, the return address and xPSR
  * First argument (r0) is svc_args[0]
  */
  switch(svc_number)
  {
    case RUN_FIRST_THREAD:
    	__set_PSP((uint32_t)threadArray[curThread].sp);
    	runFirstThread();
    	break;
    case YIELD:
    	//reset the thread's time slice or this thread may eventually stop running
    	threadArray[curThread].deadline = threadArray[curThread].thread_runtime;
		//Pend an interrupt to do the context switch
		_ICSR |= 1<<28;
		__asm("isb");
    	//contextSwitch();
    	break;
    default:    /* unknown SVC */
      break;
  }
}
