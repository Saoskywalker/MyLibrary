/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * File: $Id: mbrtu_m.c,v 1.60 2013/08/20 11:18:10 Armink Add Master Functions $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_m.h"
#include "mbconfig.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbfunc.h"
#include "mbport.h"
#if MB_MASTER_RTU_ENABLED == 1
#include "mbrtu.h"
#endif
#if MB_MASTER_ASCII_ENABLED == 1
#include "mbascii.h"
#endif
#if MB_MASTER_TCP_ENABLED == 1
#include "mbtcp.h"
#endif

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0

#ifndef MB_PORT_HAS_CLOSE
#define MB_PORT_HAS_CLOSE 0
#endif

/* ----------------------- Static variables ---------------------------------*/
static UCHAR    ucMBMasterDestAddress;
static BOOL     xMBRunInMasterMode = FALSE;
static eMBMasterErrorEventType eMBMasterCurErrorType;

static enum
{
    STATE_ENABLED,
    STATE_DISABLED,
    STATE_NOT_INITIALIZED
} eMBState = STATE_NOT_INITIALIZED;

/* Functions pointer which are initialized in eMBInit( ). Depending on the
 * mode (RTU or ASCII) the are set to the correct implementations.
 * Using for Modbus Master,Add by Armink 20130813
 */
static peMBFrameSend peMBMasterFrameSendCur;
static pvMBFrameStart pvMBMasterFrameStartCur;
static pvMBFrameStop pvMBMasterFrameStopCur;
static peMBFrameReceive peMBMasterFrameReceiveCur;
static pvMBFrameClose pvMBMasterFrameCloseCur;

/* Callback functions required by the porting layer. They are called when
 * an external event has happend which includes a timeout or the reception
 * or transmission of a character.
 * Using for Modbus Master,Add by Armink 20130813
 */
BOOL( *pxMBMasterFrameCBByteReceived ) ( void );
BOOL( *pxMBMasterFrameCBTransmitterEmpty ) ( void );
BOOL( *pxMBMasterPortCBTimerExpired ) ( void );

BOOL( *pxMBMasterFrameCBReceiveFSMCur ) ( void );
BOOL( *pxMBMasterFrameCBTransmitFSMCur ) ( void );

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */
static xMBFunctionHandler xMasterFuncHandlers[MB_FUNC_HANDLERS_MAX] = {
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
	//TODO Add Master function define
    {MB_FUNC_OTHER_REPORT_SLAVEID, eMBFuncReportSlaveID},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBMasterFuncReadInputRegister},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBMasterFuncReadHoldingRegister},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBMasterFuncWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBMasterFuncWriteHoldingRegister},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBMasterFuncReadWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBMasterFuncReadCoils},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBMasterFuncWriteCoil},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBMasterFuncWriteMultipleCoils},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBMasterFuncReadDiscreteInputs},
#endif
};

/* ----------------------- Start implementation -----------------------------*/
//函数功能：初始化Modbus协议栈
//参数 eMode：串行传输模式
//参数 ucSlaveAddress：从机地址
//参数 ucPort：端口地址
//参数 ulBaudRate：波特率
//参数 eParity：用于串行传输的奇偶校验
eMBErrorCode eMBMasterInit( eMBMode eMode, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;

	switch (eMode)
	{
#if MB_MASTER_RTU_ENABLED > 0
	case MB_RTU:
		pvMBMasterFrameStartCur = eMBMasterRTUStart;
		pvMBMasterFrameStopCur = eMBMasterRTUStop;
		peMBMasterFrameSendCur = eMBMasterRTUSend;
		peMBMasterFrameReceiveCur = eMBMasterRTUReceive;
		pvMBMasterFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBMasterPortClose : NULL;
		pxMBMasterFrameCBByteReceived = xMBMasterRTUReceiveFSM;
		pxMBMasterFrameCBTransmitterEmpty = xMBMasterRTUTransmitFSM;
		pxMBMasterPortCBTimerExpired = xMBMasterRTUTimerExpired;

		eStatus = eMBMasterRTUInit(ucPort, ulBaudRate, eParity);
		break;
#endif
#if MB_MASTER_ASCII_ENABLED > 0
		case MB_ASCII:
		pvMBMasterFrameStartCur = eMBMasterASCIIStart;
		pvMBMasterFrameStopCur = eMBMasterASCIIStop;
		peMBMasterFrameSendCur = eMBMasterASCIISend;
		peMBMasterFrameReceiveCur = eMBMasterASCIIReceive;
		pvMBMasterFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBMasterPortClose : NULL;
		pxMBMasterFrameCBByteReceived = xMBMasterASCIIReceiveFSM;
		pxMBMasterFrameCBTransmitterEmpty = xMBMasterASCIITransmitFSM;
		pxMBMasterPortCBTimerExpired = xMBMasterASCIITimerT1SExpired;

		eStatus = eMBMasterASCIIInit(ucPort, ulBaudRate, eParity );
		break;
#endif
	default:
		eStatus = MB_EINVAL;
		break;
	}

	if (eStatus == MB_ENOERR)
	{
		if (!xMBMasterPortEventInit())
		{
			eStatus = MB_EPORTERR;
		}
		else
		{
			eMBState = STATE_DISABLED;
		}
		/* initialize the OS resource for modbus master. */
		//vMBMasterOsResInit();
	}
	return eStatus;
}

//函数功能：释放协议栈使用的资源
eMBErrorCode eMBMasterClose( void )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( eMBState == STATE_DISABLED )
    {
        if( pvMBMasterFrameCloseCur != NULL )
        {
            pvMBMasterFrameCloseCur(  );
        }
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

//函数功能：启用Modbus协议栈
eMBErrorCode eMBMasterEnable( void )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( eMBState == STATE_DISABLED )
    {
        /* Activate the protocol stack. */
        pvMBMasterFrameStartCur(  );
        eMBState = STATE_ENABLED;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

//函数功能：禁用Modbus协议栈
eMBErrorCode eMBMasterDisable( void )
{
    eMBErrorCode    eStatus;

    if( eMBState == STATE_ENABLED )
    {
        pvMBMasterFrameStopCur(  );
        eMBState = STATE_DISABLED;
        eStatus = MB_ENOERR;
    }
    else if( eMBState == STATE_DISABLED )
    {
        eStatus = MB_ENOERR;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode eMBMasterPoll( void )
{	
    static UCHAR   *ucMBFrame;		//接收发送报文数据缓冲区
    static UCHAR    ucRcvAddress;	//modbus从机地址
    static UCHAR    ucFunctionCode;	//功能码
    static USHORT   usLength;		//报文长度
    static eMBException eException;	//错误码响应枚举
	//char buf[2];
    int             i , j;
    eMBErrorCode    eStatus = MB_ENOERR;
    eMBMasterEventType    eEvent;	//事件标志枚举
    eMBMasterErrorEventType errorType;

    //检查协议栈是否准备好
    if( eMBState != STATE_ENABLED )
    {
        return MB_EILLSTATE;		//协议未使能
    }
	printf("poll\r\n");
	//获取事件 
    if( xMBMasterPortEventGet( &eEvent ) == TRUE )
    {
		printf("event: %d\r\n", eEvent);
		switch ( eEvent )			//判断事件类型
		{
			case EV_MASTER_READY:	//启动完成
				break;
			case EV_MASTER_FRAME_RECEIVED:	//帧接收完成
				eStatus = peMBMasterFrameReceiveCur( &ucRcvAddress, &ucMBFrame, &usLength );
				//检查数据是否是给我们，如果不是，则忽略此数据
				if ( ( eStatus == MB_ENOERR ) && ( ucRcvAddress == ucMBMasterGetDestAddress() ) )
				{
					printf("new event\r\n");
					( void ) xMBMasterPortEventPost( EV_MASTER_EXECUTE );
				}
				else
				{
					vMBMasterSetErrorType(EV_ERROR_RECEIVE_DATA);
					( void ) xMBMasterPortEventPost( EV_MASTER_ERROR_PROCESS );
				}
				break;
			case EV_MASTER_EXECUTE:									//执行功能码
				ucFunctionCode = ucMBFrame[MB_PDU_FUNC_OFF];		//得到响应数据帧中的功能码
				eException = MB_EX_ILLEGAL_FUNCTION;				//赋错误码为无效的功能码

				for (i = 0; i < 8; i++)
					printf("%#X, ", ucMBFrame[i]);
				printf("\r\n");
				//主机发送数据帧过后，从机会返回一个响应帧，如果响应帧中功能码的最高位为1，说明发送异常
				if(ucFunctionCode >> 7) 							//如果响应数据帧发生异常
				{
					eException = (eMBException)ucMBFrame[MB_PDU_DATA_OFF];	//得到异常类型
				}
				else												//如果响应数据帧正常
				{
					for (i = 0; i < MB_FUNC_HANDLERS_MAX; i++)
					{
						/* No more function handlers registered. Abort. */
						if (xMasterFuncHandlers[i].ucFunctionCode == 0)	
						{
							break;
						}
						else if (xMasterFuncHandlers[i].ucFunctionCode == ucFunctionCode) 	//执行函数功能码与响应帧功能码相同
						{
							vMBMasterSetCBRunInMasterMode(TRUE);
							//如果主请求被广播,主服务器需要执行所有从服务器的执行功能
							if ( xMBMasterRequestIsBroadcast() ) 	
							{
								usLength = usMBMasterGetPDUSndLength();
								for(j = 1; j <= MB_MASTER_TOTAL_SLAVE_NUM; j++)
								{
									vMBMasterSetDestAddress(j);
									eException = xMasterFuncHandlers[i].pxHandler(ucMBFrame, &usLength);
								}
							}
							else 
							{
								eException = xMasterFuncHandlers[i].pxHandler(ucMBFrame, &usLength);
							}
							vMBMasterSetCBRunInMasterMode(FALSE);
							break;
						}
					}
				}
				/* If master has exception ,Master will send error process.Otherwise the Master is idle.*/
				if (eException != MB_EX_NONE) 
				{
					vMBMasterSetErrorType(EV_ERROR_EXECUTE_FUNCTION);
					( void ) xMBMasterPortEventPost( EV_MASTER_ERROR_PROCESS );
				}
				else 
				{
					vMBMasterCBRequestScuuess( );
					vMBMasterRunResRelease( );
				}
				break;
			case EV_MASTER_FRAME_SENT:
				/* Master is busy now. */
				vMBMasterGetPDUSndBuf( &ucMBFrame );
				for (i = 0; i < 8; i++)
					printf("%#X, ", ucMBFrame[i]);
				printf("...  MS send\r\n");
				eStatus = peMBMasterFrameSendCur( ucMBMasterGetDestAddress(), ucMBFrame, usMBMasterGetPDUSndLength() );
				break;

			case EV_MASTER_ERROR_PROCESS:
				/* Execute specified error process callback function. */
				errorType = eMBMasterGetErrorType();
				vMBMasterGetPDUSndBuf( &ucMBFrame );
				for (i = 0; i < 8; i++)
					printf("%#X, ", ucMBFrame[i]);
				printf("...%d, error\r\n", errorType);
				switch (errorType) 
				{
					case EV_ERROR_RESPOND_TIMEOUT:
						//vMBMasterErrorCBRespondTimeout(ucMBMasterGetDestAddress(), ucMBFrame, usMBMasterGetPDUSndLength());
						break;
					case EV_ERROR_RECEIVE_DATA:
						//vMBMasterErrorCBReceiveData(ucMBMasterGetDestAddress(), ucMBFrame, usMBMasterGetPDUSndLength());
						break;
					case EV_ERROR_EXECUTE_FUNCTION:
						//vMBMasterErrorCBExecuteFunction(ucMBMasterGetDestAddress(), ucMBFrame, usMBMasterGetPDUSndLength());
						break;
				}
				vMBMasterRunResRelease();
				break;
        }
    }
    return MB_ENOERR;
}

/* Get whether the Modbus Master is run in master mode.*/
BOOL xMBMasterGetCBRunInMasterMode( void )
{
	return xMBRunInMasterMode;
}
/* Set whether the Modbus Master is run in master mode.*/
void vMBMasterSetCBRunInMasterMode( BOOL IsMasterMode )
{
	xMBRunInMasterMode = IsMasterMode;
}
/* Get Modbus Master send destination address. */
UCHAR ucMBMasterGetDestAddress( void )
{
	return ucMBMasterDestAddress;
}
/* Set Modbus Master send destination address. */
void vMBMasterSetDestAddress( UCHAR Address )
{
	ucMBMasterDestAddress = Address;
}
/* Get Modbus Master current error event type. */
eMBMasterErrorEventType eMBMasterGetErrorType( void )
{
	return eMBMasterCurErrorType;
}
/* Set Modbus Master current error event type. */
void vMBMasterSetErrorType( eMBMasterErrorEventType errorType )
{
	eMBMasterCurErrorType = errorType;
}



#endif
