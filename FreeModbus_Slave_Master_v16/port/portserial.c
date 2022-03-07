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
#include "port.h"
#include "led.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbconfig.h"
// #include "rs485.h"
// #include "includes.h"

#if MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED > 0

/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( void );
static void prvvUARTRxISR( void );

/* ----------------------- 串口中断开启关闭 -----------------------------*/
void vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
	if(xRxEnable)
	{
		USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);//串口2接收中断使能
		GPIO_ResetBits(GPIOD,GPIO_Pin_7);//485低电平接收
	}
	else
	{
		USART_ITConfig(USART2,USART_IT_RXNE,DISABLE);//串口2接收中断关闭
		GPIO_SetBits(GPIOD,GPIO_Pin_7);//485高电平发送
	}
	
	if(xTxEnable)
	{
		USART_ITConfig(USART2,USART_IT_TC,ENABLE);//串口2发送中断使能
		GPIO_SetBits(GPIOD,GPIO_Pin_7);				//485高电平发送
	}
	else
	{
		USART_ITConfig(USART2,USART_IT_TC,DISABLE);//串口2发送中断关闭
		GPIO_ResetBits(GPIOD,GPIO_Pin_7);			//485低电平接收
	}
}

void vMBPortClose(void)
{
	USART_ITConfig(USART2, USART_IT_TC | USART_IT_RXNE, DISABLE);
	USART_Cmd(USART2, DISABLE);
}


/* ----------------------- 串口初始化 -----------------------------*/
BOOL xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
	(void)ucPORT;//不修改串口
	(void)ucDataBits;//不修改数据位长度
	(void)eParity;//不修改校验格式
	
	RS485_Init(ulBaudRate);//串口2
	
	return TRUE;
}
/* ----------------------- 发送单字节数据 -----------------------------*/

BOOL xMBPortSerialPutByte( CHAR ucByte )
{
	USART_SendData(USART2,ucByte);//发送一个字节
	//while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET){};
    return TRUE;
}
/* ----------------------- 接收单字节数据 -----------------------------*/
BOOL xMBPortSerialGetByte( CHAR * pucByte )
{
	*pucByte = USART_ReceiveData(USART2);//接收一个字节
    return TRUE;
}


static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );//发送状态机
}


static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );//接收状态机
}


void USART2_IRQHandler(void)                				//串口2中断服务程序
{
#if SYSTEM_SUPPORT_OS 										//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) 	 //接收中断
	{
		prvvUARTRxISR();
		USART_ClearITPendingBit(USART2,USART_IT_RXNE);
	}	
	if(USART_GetITStatus(USART2, USART_IT_TC) == SET)		//发送完成中断	
	{
		prvvUARTTxReadyISR();
		USART_ClearITPendingBit(USART2,USART_IT_TC);	  	
	}; 
#if SYSTEM_SUPPORT_OS 	//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntExit();  											 
#endif
} 
	
#endif
