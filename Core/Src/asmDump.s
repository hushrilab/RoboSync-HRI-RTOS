  .syntax unified //This lets us use C like comments!
  .cpu cortex-m4 //Guess what this does
  .thumb //Practically this only matters to the CPU, but it ensures that the correct types of instructions get included

  .global SVC_Handler //We need to indicate to the linker that this function exists
  .thumb_func //We need to ensure that the address of our interrupt function is properly aligned or we hard fault. a LOT
  SVC_Handler: //our function name
	TST LR, 4 //TeST the 3rd bit in LR (4 is 0b1000, so its 3rd bit is 1)
	ITE EQ //If Then Equal
	MRSEQ R0, MSP //If the third bit is set, we are using MSP. Set us up to use that
	MRSNE R0, PSP //Otherwise, use PSP
	B SVC_Handler_Main //Go to the C function, because screw assembly


  .global runFirstThread //Running the first thread requires some special consideration, so it is its own function
  .thumb_func
  runFirstThread:
  	//Restore MSP since we have two things on there that won't go away
  	POP {R7}
  	POP {R7}

  	//Get ready for PSP
  	MRS R0, PSP
  	MOV LR, #0xFFFFFFFD
  	LDMIA R0!,{R4-R11}
  	MSR PSP, R0
  	BX LR

   .global PendSV_Handler //In general, we want to use PendSV for the actual context switching
   .thumb_func
   PendSV_Handler:
	//Restore MSP since we have two things on there that won't go away
	//POP {R7}
	//POP {R7}

	//Perform the switch
	MRS R0, PSP
	STMDB R0!,{R4-R11}
	BL osSched
	MRS R0, PSP
	MOV LR, #0xFFFFFFFD
	LDMIA R0!,{R4-R11}
	MSR PSP, R0
	BX LR
