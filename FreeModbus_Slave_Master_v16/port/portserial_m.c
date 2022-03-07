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
 * File: $Id: portserial_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbport.h"
#include "mbconfig.h"
#include "usart.h"
#include "rs485.h"
#include "includes.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0

/* ----------------------- Defines ------------------------------------------*/
/* serial transmit event */
#define EVENT_SERIAL_TRANS_START    (1<<0)

/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR(void);
static void prvvUARTRxISR(void);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity)
{
	(void)ucPORT;//不修改串口
	(void)ucDataBits;//不修改数据位长度
	(void)eParity;//不修改校验格式
	
	RS485_Init(ulBaudRate);//串口2

    return TRUE;
}

void vMBMasterPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
	if(xRxEnable)
	{
		USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);//串口2接收中断使能
		GPIO_ResetBits(GPIOG,GPIO_Pin_8);//485低电平接收
	}
	else
	{
		USART_ITConfig(USART2,USART_IT_RXNE,DISABLE);//串口2接收中断关闭
		GPIO_SetBits(GPIOG,GPIO_Pin_8);//485高电平发送
	}
	
	if(xTxEnable)
	{
		USART_ITConfig(USART2,USART_IT_TC,ENABLE);//串口2发送中断使能
		GPIO_SetBits(GPIOG,GPIO_Pin_8);				//485高电平发送
	}
	else
	{
		USART_ITConfig(USART2,USART_IT_TC,DISABLE);//串口2发送中断关闭
		GPIO_ResetBits(GPIOG,GPIO_Pin_8);			//485低电平接收
	}
}

void vMBMasterPortClose(void)
{
	USART_ITConfig(USART2, USART_IT_TC | USART_IT_RXNE, DISABLE);
	USART_Cmd(USART2, DISABLE);
}

BOOL xMBMasterPortSerialPutByte(CHAR ucByte)
{
	USART_SendData(USART2,ucByte);//发送一个字节
	//while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET){};
    return TRUE;
}

BOOL xMBMasterPortSerialGetByte(CHAR * pucByte)
{
	*pucByte = USART_ReceiveData(USART2);//接收一个字节
    return TRUE;
}


static void prvvUARTTxReadyISR(void)
{
    pxMBMasterFrameCBTransmitterEmpty();//发送状态机
}

static void prvvUARTRxISR(void)
{
    pxMBMasterFrameCBByteReceived();//接收状态机
}


// void USART2_IRQHandler(void)                				//串口2中断服务程序
// {
// #if SYSTEM_SUPPORT_OS 										//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
// 	OSIntEnter();    
// #endif
// 	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) 	 //接收中断
// 	{
// 		prvvUARTRxISR();
// 		USART_ClearITPendingBit(USART2,USART_IT_RXNE);
// 	}	
// 	if(USART_GetITStatus(USART2, USART_IT_TC) == SET)		//发送完成中断	
// 	{
// 		prvvUARTTxReadyISR();
// 		USART_ClearITPendingBit(USART2,USART_IT_TC);	  	
// 	}; 
// #if SYSTEM_SUPPORT_OS 	//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
// 	OSIntExit();  											 
// #endif
// } 

#endif
