/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
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
 * File: $Id$
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbconfig.h"
// #include "timer.h"
// #include "sys.h"

#if MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED > 0

/* ----------------------- static functions ---------------------------------*/
// extern void TIM3_Int_Init(u16 arr,u16 psc); 
static void prvvTIMERExpiredISR( void );
//extern void TIM3_Int_Init(uint16_t arr,uint16_t psc); 
/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortTimersInit( USHORT usTim1Timerout50us )
{
   // return FALSE;
	TIM3_Int_Init(usTim1Timerout50us,(SystemCoreClock /20000)-1);//20KHZ
	return TRUE;
}

void
vMBPortTimersEnable(  )
{
    /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
	TIM_ITConfig( TIM3,TIM_IT_Update,ENABLE); //ʹ�ܻ���ʧ��ָ����TIM�ж�TIM3,
	TIM_SetCounter(TIM3,0);
	TIM_Cmd(TIM3,ENABLE);
}

void
vMBPortTimersDisable(  )
{
    /* Disable any pending timers. */
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
	TIM_ITConfig( TIM3,TIM_IT_Update,DISABLE); //ʹ�ܻ���ʧ��ָ����TIM�ж�TIM3,
	TIM_SetCounter(TIM3,0);
	TIM_Cmd(TIM3,DISABLE);
}

/* Create an ISR which is called whenever the timer has expired. This function
 * must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
 * the timer has expired.
 */
static void prvvTIMERExpiredISR( void )
{
    ( void )pxMBPortCBTimerExpired(  );
}

// void TIM3_IRQHandler(void)   //TIM3�ж�
// {
// 	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //���ָ����TIM�жϷ������:TIM �ж�Դ 
// 	{
// 		TIM_ClearFlag(TIM3,TIM_FLAG_Update);//���жϱ��
// 		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  ////�����ʱ��T3����жϱ�־λ
// 		prvvTIMERExpiredISR();
// 	}
// }

#endif
