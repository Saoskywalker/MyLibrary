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

/* ----------------------- �����жϿ����ر� -----------------------------*/
void vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
	if(xRxEnable)
	{
		USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);//����2�����ж�ʹ��
		GPIO_ResetBits(GPIOD,GPIO_Pin_7);//485�͵�ƽ����
	}
	else
	{
		USART_ITConfig(USART2,USART_IT_RXNE,DISABLE);//����2�����жϹر�
		GPIO_SetBits(GPIOD,GPIO_Pin_7);//485�ߵ�ƽ����
	}
	
	if(xTxEnable)
	{
		USART_ITConfig(USART2,USART_IT_TC,ENABLE);//����2�����ж�ʹ��
		GPIO_SetBits(GPIOD,GPIO_Pin_7);				//485�ߵ�ƽ����
	}
	else
	{
		USART_ITConfig(USART2,USART_IT_TC,DISABLE);//����2�����жϹر�
		GPIO_ResetBits(GPIOD,GPIO_Pin_7);			//485�͵�ƽ����
	}
}

void vMBPortClose(void)
{
	USART_ITConfig(USART2, USART_IT_TC | USART_IT_RXNE, DISABLE);
	USART_Cmd(USART2, DISABLE);
}


/* ----------------------- ���ڳ�ʼ�� -----------------------------*/
BOOL xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
	(void)ucPORT;//���޸Ĵ���
	(void)ucDataBits;//���޸�����λ����
	(void)eParity;//���޸�У���ʽ
	
	RS485_Init(ulBaudRate);//����2
	
	return TRUE;
}
/* ----------------------- ���͵��ֽ����� -----------------------------*/

BOOL xMBPortSerialPutByte( CHAR ucByte )
{
	USART_SendData(USART2,ucByte);//����һ���ֽ�
	//while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET){};
    return TRUE;
}
/* ----------------------- ���յ��ֽ����� -----------------------------*/
BOOL xMBPortSerialGetByte( CHAR * pucByte )
{
	*pucByte = USART_ReceiveData(USART2);//����һ���ֽ�
    return TRUE;
}


static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );//����״̬��
}


static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );//����״̬��
}


void USART2_IRQHandler(void)                				//����2�жϷ������
{
#if SYSTEM_SUPPORT_OS 										//���SYSTEM_SUPPORT_OSΪ�棬����Ҫ֧��OS.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) 	 //�����ж�
	{
		prvvUARTRxISR();
		USART_ClearITPendingBit(USART2,USART_IT_RXNE);
	}	
	if(USART_GetITStatus(USART2, USART_IT_TC) == SET)		//��������ж�	
	{
		prvvUARTTxReadyISR();
		USART_ClearITPendingBit(USART2,USART_IT_TC);	  	
	}; 
#if SYSTEM_SUPPORT_OS 	//���SYSTEM_SUPPORT_OSΪ�棬����Ҫ֧��OS.
	OSIntExit();  											 
#endif
} 
	
#endif
