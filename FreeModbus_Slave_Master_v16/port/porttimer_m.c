/*
 * FreeModbus Libary: RT-Thread Port
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: porttimer_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"
#include "timer.h"
#include "sys.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_m.h"
#include "mbport.h"
#include "mbconfig.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Variables ----------------------------------------*/
static void prvvTIMERExpiredISR(void);

/* ----------------------- static functions ---------------------------------*/
static void prvvTIMERExpiredISR(void);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortTimersInit(USHORT usTimeOut50us)
{
    /* backup T35 ticks */
	TIM3_Int_Init(usTimeOut50us,(SystemCoreClock /20000)-1);//20KHZ
	return TRUE;
}

void vMBMasterPortTimersT35Enable()
{
    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_T35);
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
	TIM_ITConfig( TIM3,TIM_IT_Update,ENABLE); //使能或者失能指定的TIM中断TIM3,
	TIM_SetCounter(TIM3,0);
	TIM_Cmd(TIM3,ENABLE);
}

void vMBMasterPortTimersConvertDelayEnable()
{
    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_CONVERT_DELAY);
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
	TIM_ITConfig( TIM3,TIM_IT_Update,ENABLE); //使能或者失能指定的TIM中断TIM3,
	TIM_SetCounter(TIM3,0);
	TIM_Cmd(TIM3,ENABLE);
}

void vMBMasterPortTimersRespondTimeoutEnable()
{
    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_RESPOND_TIMEOUT);
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
	TIM_ITConfig( TIM3,TIM_IT_Update,ENABLE); //使能或者失能指定的TIM中断TIM3,
	TIM_SetCounter(TIM3,0);
	TIM_Cmd(TIM3,ENABLE);
}

void vMBMasterPortTimersDisable()
{
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
	TIM_ITConfig( TIM3,TIM_IT_Update,DISABLE); //使能或者失能指定的TIM中断TIM3,
	TIM_SetCounter(TIM3,0);
	TIM_Cmd(TIM3,DISABLE);
}

static void prvvTIMERExpiredISR(void)
{
    (void) pxMBMasterPortCBTimerExpired();
}

// void TIM3_IRQHandler(void)   //TIM3中断
// {
// 	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
// 	{
// 		TIM_ClearFlag(TIM3,TIM_FLAG_Update);//清中断标记
// 		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  ////清除定时器T3溢出中断标志位
// 		prvvTIMERExpiredISR();
// 	}
// }
#endif
