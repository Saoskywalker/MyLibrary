;/*
; * FreeRTOS Kernel V10.4.3 LTS Patch 1
; * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
; *
; * Permission is hereby granted, free of charge, to any person obtaining a copy of
; * this software and associated documentation files (the "Software"), to deal in
; * the Software without restriction, including without limitation the rights to
; * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
; * the Software, and to permit persons to whom the Software is furnished to do so,
; * subject to the following conditions:
; *
; * The above copyright notice and this permission notice shall be included in all
; * copies or substantial portions of the Software.
; *
; * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
; * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
; * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
; * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
; * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
; *
; * https://www.FreeRTOS.org
; * https://github.com/FreeRTOS
; *
; * 1 tab == 4 spaces!
; */

	INCLUDE portmacro.inc

	IMPORT	vTaskSwitchContext
	IMPORT	xTaskIncrementTick
	IMPORT  vFreeRTOS_ISR

	EXPORT	vPortYieldProcessor
	EXPORT	vPortStartFirstTask
	EXPORT	vPortYield
	EXPORT	vPortIrqProcessor

	ARM
	AREA	PORT_ASM, CODE, READONLY



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Starting the first task is done by just restoring the context
; setup by pxPortInitialiseStack
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
vPortStartFirstTask

	PRESERVE8

	portRESTORE_CONTEXT

vPortYield

	PRESERVE8

	SVC 0
	bx lr

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Interrupt service routine for the SWI interrupt.  The vector table is
; configured in the startup.s file.
;
; vPortYieldProcessor() is used to manually force a context switch.  The
; SWI interrupt is generated by a call to taskYIELD() or portYIELD().
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

vPortYieldProcessor

	PRESERVE8

	; Within an IRQ ISR the link register has an offset from the true return
	; address, but an SWI ISR does not.  Add the offset manually so the same
	; ISR return code can be used in both cases.
	ADD	LR, LR, #4

	; Perform the context switch.
	portSAVE_CONTEXT					; Save current task context
	LDR R0, =vTaskSwitchContext			; Get the address of the context switch function
	MOV LR, PC							; Store the return address
	BX	R0								; Call the contedxt switch function
	portRESTORE_CONTEXT					; restore the context of the selected task

vPortIrqProcessor

	PRESERVE8

	portSAVE_CONTEXT ; Save current task context
	bl vFreeRTOS_ISR ; call isr
	portRESTORE_CONTEXT ; restore the context of the selected task

	END
