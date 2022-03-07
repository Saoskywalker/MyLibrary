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
 * File: $Id: portevent_m.c v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_m.h"
#include "mbport.h"
#include "port.h"
#include "mbconfig.h"
// #include "includes.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Defines ------------------------------------------*/
// OS_SEM	SlaveOsEvent;
extern xMBMasterRTUTimerExpired(void);
/* ----------------------- Variables ----------------------------------------*/
static eMBMasterEventType eMasterQueuedEvent;	//排队中的事件类型
static BOOL    xMasterEventInQueue;				//是否有待处理的事件在排队
/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortEventInit( void )
{
	//xMasterEventInQueue = TRUE;
	//eMasterQueuedEvent = FLAG_PRIO;
	// OS_ERR err;
	// //创建一个信号量
	// OSSemCreate ((OS_SEM*	)&SlaveOsEvent,
    //              (CPU_CHAR*	)"SlaveOsEvent",
    //              (OS_SEM_CTR)1,		
    //              (OS_ERR*	)&err);

	xMasterEventInQueue = FALSE;
    return TRUE;
}

BOOL xMBMasterPortEventPost( eMBMasterEventType eEvent )
{
	//xMasterEventInQueue = eEvent;
	//eMasterQueuedEvent = eEvent;
	// OS_ERR err;
	// xMasterEventInQueue = eEvent;
	// OSSemPost(&SlaveOsEvent,OS_OPT_POST_1,&err);//发送信号量
    // return TRUE;

	//有事件标志更新
	xMasterEventInQueue = TRUE;
	//设定事件标志
	eMasterQueuedEvent = eEvent;
	return TRUE;
}

BOOL xMBMasterPortEventGet( eMBMasterEventType * eEvent )
{	
	//OS_ERR err;
	//OSSemPend(&SlaveOsEvent,0,OS_OPT_PEND_BLOCKING,0,&err);
    // switch (xMasterEventInQueue)
    // {
	// 	case EV_MASTER_READY:
	// 			*eEvent = EV_MASTER_READY;			 		
	// 			break;
	// 	case EV_MASTER_FRAME_RECEIVED:
	// 			*eEvent = EV_MASTER_FRAME_RECEIVED;
	// 			break;
	// 	case EV_MASTER_EXECUTE:
	// 			*eEvent = EV_MASTER_EXECUTE;
	// 			break;
	// 	case EV_MASTER_FRAME_SENT:
	// 			*eEvent = EV_MASTER_FRAME_SENT;
	// 			break;
	// 	case EV_MASTER_ERROR_PROCESS:
	// 			*eEvent = EV_MASTER_ERROR_PROCESS;
	// 			break;
    // }
	//return TRUE;

	BOOL xEventHappened = FALSE;

	//若有事件更新
	if( xMasterEventInQueue )
	{
		//获得事件
		*eEvent = eMasterQueuedEvent;
		xMasterEventInQueue = FALSE;
		xEventHappened = TRUE;
	}
	
    return xEventHappened;
}


//BOOL vMBMasterOsResInit( void )
//{
//	return TRUE;
//}

/**
 * This function is take Mobus Master running resource.
 * Note:The resource is define by Operating System.If you not use OS this function can be just return TRUE.
 *
 * @param lTimeOut the waiting time.
 *
 * @return resource taked result
 */
BOOL xMBMasterRunResTake( LONG lTimeOut )
{
	return TRUE;
}

/**
 * This function is release Mobus Master running resource.
 * Note:The resource is define by Operating System.If you not use OS this function can be empty.
 *
 */
BOOL vMBMasterRunResRelease( void )
{
	return TRUE;
}

/**
 * This is modbus master respond timeout error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
BOOL vMBMasterErrorCBRespondTimeout(UCHAR ucDestAddress, const UCHAR* pucPDUData, USHORT ucPDULength) 
{
    xMasterEventInQueue = TRUE;
    eMasterQueuedEvent = EV_MASTER_ERROR_RESPOND_TIMEOUT;
    return TRUE;
}

/**
 * This is modbus master receive data error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
BOOL vMBMasterErrorCBReceiveData(UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) 
{
    xMasterEventInQueue = TRUE;
    eMasterQueuedEvent = EV_MASTER_ERROR_RECEIVE_DATA;
    return TRUE;
}

/**
 * This is modbus master execute function error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
BOOL vMBMasterErrorCBExecuteFunction(UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) 
{
    xMasterEventInQueue = TRUE;
    eMasterQueuedEvent = EV_MASTER_ERROR_EXECUTE_FUNCTION;
    return TRUE;
}

/**
 * This is modbus master request process success callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 */
BOOL vMBMasterCBRequestScuuess( void ) 
{
    xMasterEventInQueue = TRUE;
    eMasterQueuedEvent = EV_MASTER_PROCESS_SUCESS;
    return TRUE;
}

/**
 * This function is wait for modbus master request finish and return result.
 * Waiting result include request process success, request respond timeout,
 * receive data error and execute function error.You can use the above callback function.
 * @note If you are use OS, you can use OS's event mechanism. Otherwise you have to run
 * much user custom delay for waiting.
 *
 * @return request error code
 */

eMBMasterReqErrCode eMBMasterWaitRequestFinish( void ) 
{
    eMBMasterReqErrCode    eErrStatus = MB_MRE_NO_ERR;
	// OS_ERR err;
	// OSSemPend(&SlaveOsEvent,0,OS_OPT_PEND_BLOCKING,0,&err);
    // switch (xMasterEventInQueue)
    // {
	// 	case EV_MASTER_PROCESS_SUCESS:
	// 		break;
			
	// 	case EV_MASTER_ERROR_RESPOND_TIMEOUT:  
	// 		eErrStatus = MB_MRE_TIMEDOUT;
	// 		break;
		
	// 	case EV_MASTER_ERROR_RECEIVE_DATA:   
	// 		eErrStatus = MB_MRE_REV_DATA;
	// 		break;
		
	// 	case EV_MASTER_ERROR_EXECUTE_FUNCTION:
	// 		eErrStatus = MB_MRE_EXE_FUN;
	// 		break;
    // }
    return eErrStatus;
}

#endif
