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

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbconfig.h"
// #include "includes.h"

#if MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED > 0 || MB_TCP_ENABLED > 0

/* ----------------------- Variables ----------------------------------------*/
static eMBEventType eQueuedEvent;//�Ŷ��е��¼�����
static BOOL     xEventInQueue;//�Ƿ��д�������¼����Ŷ�
// OS_SEM	SlaveOsEvent;
/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortEventInit( void )
{
	//xMasterEventInQueue = TRUE;
	//eMasterQueuedEvent = FLAG_PRIO;
	// OS_ERR err;
	// //����һ���ź���
	// OSSemCreate ((OS_SEM*	)&SlaveOsEvent,
    //              (CPU_CHAR*	)"SlaveOsEvent",
    //              (OS_SEM_CTR)1,		
    //              (OS_ERR*	)&err);
	xEventInQueue = FALSE;
    return TRUE;
}
/***********�����¼�***********/
BOOL xMBPortEventPost( eMBEventType eEvent )
{
	//xMasterEventInQueue = eEvent;
	//eMasterQueuedEvent = eEvent;
	// OS_ERR err;
	// xEventInQueue = eEvent;
	// OSSemPost(&SlaveOsEvent,OS_OPT_POST_1,&err);//�����ź���

	// return TRUE;

	//���¼���־����
	xEventInQueue = TRUE;
	//�趨�¼���־
	eQueuedEvent = eEvent;
	return TRUE;
}

BOOL xMBPortTCPPool( void );
BOOL xMBPortEventGet( eMBEventType * eEvent )
{	
	// OS_ERR err;
	// OSSemPend(&SlaveOsEvent,0,OS_OPT_PEND_BLOCKING,0,&err);
    // switch (xEventInQueue)
    // {
	// 	case EV_READY:
	// 			*eEvent = EV_READY;			 		
	// 			break;
	// 	case EV_FRAME_RECEIVED:
	// 			*eEvent = EV_FRAME_RECEIVED;
	// 			break;
	// 	case EV_EXECUTE:
	// 			*eEvent = EV_EXECUTE;
	// 			break;
	// 	case EV_FRAME_SENT:
	// 			*eEvent = EV_FRAME_SENT;
	// 			break;
    // }
	
    // return TRUE;
	
	BOOL xEventHappened = FALSE;
	
    //�����¼�����
	if( xEventInQueue )
	{
		//����¼�
		*eEvent = eQueuedEvent;
		xEventInQueue = FALSE;
		xEventHappened = TRUE;
	}
#if MB_TCP_ENABLED > 0
	else		//TCP������봥������
	{
  	(void)xMBPortTCPPool();
	}
#endif

	return xEventHappened;
}

#endif
