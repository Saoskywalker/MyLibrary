/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006-2018 Christian Walter <cwalter@embedded-solutions.at>
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
 */
/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"
#include "stdio.h"	

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbconfig.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbfunc.h"
#include "mbport.h"
#if MB_RTU_ENABLED == 1
#include "mbrtu.h"
#endif
#if MB_ASCII_ENABLED == 1
#include "mbascii.h"
#endif
#if MB_TCP_ENABLED == 1
#include "mbtcp.h"
#endif

#if MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED > 0 || MB_TCP_ENABLED > 0

#ifndef MB_PORT_HAS_CLOSE
#define MB_PORT_HAS_CLOSE 0
#endif

/* ----------------------- Static variables ---------------------------------*/

static UCHAR    ucMBAddress;
static eMBMode  eMBCurrentMode;
extern USHORT   usSRegHoldBuf[100]; 
static enum
{
    STATE_ENABLED,
    STATE_DISABLED,
    STATE_NOT_INITIALIZED
} eMBState = STATE_NOT_INITIALIZED;

/* Functions pointer which are initialized in eMBInit( ). Depending on the
 * mode (RTU or ASCII) the are set to the correct implementations.
 */
static peMBFrameSend peMBFrameSendCur;			//发送数据函数指针
static pvMBFrameStart pvMBFrameStartCur;		//启动传输函数指针
static pvMBFrameStop pvMBFrameStopCur;			//停止传输函数指针
static peMBFrameReceive peMBFrameReceiveCur;	//接收数据函数指针
static pvMBFrameClose pvMBFrameCloseCur;		//关闭传输函数指针

/* Callback functions required by the porting layer. They are called when
 * an external event has happend which includes a timeout or the reception
 * or transmission of a character.
 */
BOOL( *pxMBFrameCBByteReceived ) ( void );
BOOL( *pxMBFrameCBTransmitterEmpty ) ( void );
BOOL( *pxMBPortCBTimerExpired ) ( void );

BOOL( *pxMBFrameCBReceiveFSMCur ) ( void );
BOOL( *pxMBFrameCBTransmitFSMCur ) ( void );

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */
static xMBFunctionHandler xFuncHandlers[MB_FUNC_HANDLERS_MAX] = {
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, eMBFuncReportSlaveID},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBFuncReadInputRegister},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBFuncReadHoldingRegister},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBFuncWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBFuncWriteHoldingRegister},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBFuncReadWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBFuncReadCoils},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBFuncWriteCoil},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBFuncWriteMultipleCoils},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBFuncReadDiscreteInputs},
#endif
};

/* ----------------------- Start implementation -----------------------------*/
/*函数功能： 
*1:实现RTU模式和ASCALL模式的协议栈初始化; 
*2:完成协议栈核心函数指针的赋值，包括Modbus协议栈的使能和禁止、报文的接收和响应、3.5T定时器中断回调函数、串口发送和接收中断回调函数; 
*3:eMBRTUInit完成RTU模式下串口和3.5T定时器的初始化，需用户自己移植; 
*4:设置Modbus协议栈的模式eMBCurrentMode为MB_RTU，设置Modbus协议栈状态eMBState为STATE_DISABLED; 
*/  
//函数功能：初始化Modbus协议栈
//参数 eMode：串行传输模式
//参数 ucSlaveAddress：从机地址
//参数 ucPort：端口地址
//参数 ulBaudRate：波特率
//参数 eParity：用于串行传输的奇偶校验
eMBErrorCode eMBInit( eMBMode eMode, UCHAR ucSlaveAddress, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;

   //验证从机地址
    if( ( ucSlaveAddress == MB_ADDRESS_BROADCAST ) ||
        ( ucSlaveAddress < MB_ADDRESS_MIN ) || ( ucSlaveAddress > MB_ADDRESS_MAX ) )
    {
		//从机地址错误
        eStatus = MB_EINVAL;			
    }
    else
    {
        ucMBAddress = ucSlaveAddress;

        switch ( eMode )
        {
#if MB_RTU_ENABLED > 0
        case MB_RTU:		//RTU传输模式
            pvMBFrameStartCur = eMBRTUStart;
            pvMBFrameStopCur = eMBRTUStop;
            peMBFrameSendCur = eMBRTUSend;
            peMBFrameReceiveCur = eMBRTUReceive;
            pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;
            pxMBFrameCBByteReceived = xMBRTUReceiveFSM;			//接收状态机，串口接受中断最终调用此函数接收数据
            pxMBFrameCBTransmitterEmpty = xMBRTUTransmitFSM;	//发送状态机，串口发送中断最终调用此函数发送数据
            pxMBPortCBTimerExpired = xMBRTUTimerT35Expired; 	//报文到达间隔检查，定时器中断函数最终调用次函数完成定时器中断

            eStatus = eMBRTUInit( ucMBAddress, ucPort, ulBaudRate, eParity );
            break;
#endif
#if MB_ASCII_ENABLED > 0
        case MB_ASCII:		//ASCII传输模式
            pvMBFrameStartCur = eMBASCIIStart;
            pvMBFrameStopCur = eMBASCIIStop;
            peMBFrameSendCur = eMBASCIISend;
            peMBFrameReceiveCur = eMBASCIIReceive;
            pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;
            pxMBFrameCBByteReceived = xMBASCIIReceiveFSM;
            pxMBFrameCBTransmitterEmpty = xMBASCIITransmitFSM;
            pxMBPortCBTimerExpired = xMBASCIITimerT1SExpired;

            eStatus = eMBASCIIInit( ucMBAddress, ucPort, ulBaudRate, eParity );
            break;
#endif
        default:			//非法参数
            eStatus = MB_EINVAL;
        }

        if( eStatus == MB_ENOERR )
        {
            if( !xMBPortEventInit(  ) )
            {
                /* port dependent event module initalization failed. */
                eStatus = MB_EPORTERR;
            }
            else
            {
                eMBCurrentMode = eMode;
                eMBState = STATE_DISABLED;
            }
        }
    }
    return eStatus;
}

#if MB_TCP_ENABLED > 0
eMBErrorCode
eMBTCPInit( USHORT ucTCPPort )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( ( eStatus = eMBTCPDoInit( ucTCPPort ) ) != MB_ENOERR )
    {
        eMBState = STATE_DISABLED;
    }
    else if( !xMBPortEventInit(  ) )
    {
        /* Port dependent event module initalization failed. */
        eStatus = MB_EPORTERR;
    }
    else
    {
        pvMBFrameStartCur = eMBTCPStart;
        pvMBFrameStopCur = eMBTCPStop;
        peMBFrameReceiveCur = eMBTCPReceive;
        peMBFrameSendCur = eMBTCPSend;
        pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBTCPPortClose : NULL;
        ucMBAddress = MB_TCP_PSEUDO_ADDRESS;
        eMBCurrentMode = MB_TCP;
        eMBState = STATE_DISABLED;
    }
    return eStatus;
}
#endif

eMBErrorCode
eMBRegisterCB( UCHAR ucFunctionCode, pxMBFunctionHandler pxHandler )
{
    int             i;
    eMBErrorCode    eStatus;

    if( ( 0 < ucFunctionCode ) && ( ucFunctionCode <= 127 ) )
    {
        ENTER_CRITICAL_SECTION(  );
        if( pxHandler != NULL )
        {
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                if( ( xFuncHandlers[i].pxHandler == NULL ) ||
                    ( xFuncHandlers[i].pxHandler == pxHandler ) )
                {
                    xFuncHandlers[i].ucFunctionCode = ucFunctionCode;
                    xFuncHandlers[i].pxHandler = pxHandler;
                    break;
                }
            }
            eStatus = ( i != MB_FUNC_HANDLERS_MAX ) ? MB_ENOERR : MB_ENORES;
        }
        else
        {
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                if( xFuncHandlers[i].ucFunctionCode == ucFunctionCode )
                {
                    xFuncHandlers[i].ucFunctionCode = 0;
                    xFuncHandlers[i].pxHandler = NULL;
                    break;
                }
            }
            /* Remove can't fail. */
            eStatus = MB_ENOERR;
        }
        EXIT_CRITICAL_SECTION(  );
    }
    else
    {
        eStatus = MB_EINVAL;
    }
    return eStatus;
}

//函数功能：释放协议栈使用的资源
eMBErrorCode eMBClose( void )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( eMBState == STATE_DISABLED )
    {
        if( pvMBFrameCloseCur != NULL )
        {
            pvMBFrameCloseCur(  );
        }
    }
    else
    {
        eStatus = MB_EILLSTATE;			//协议栈处于非法状态
    }
    return eStatus;
}

//函数功能：启用Modbus协议栈
eMBErrorCode eMBEnable( void )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( eMBState == STATE_DISABLED )
    {
        pvMBFrameStartCur(  );		//启动RTU传输
        eMBState = STATE_ENABLED;
    }
    else
    {
        eStatus = MB_EILLSTATE;		//协议栈处于非法状态
    }
    return eStatus;
}

//函数功能：禁用Modbus协议栈
eMBErrorCode eMBDisable( void )
{
    eMBErrorCode    eStatus;

    if( eMBState == STATE_ENABLED )
    {
        pvMBFrameStopCur(  );
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
/*函数功能: 
*1:检查协议栈状态是否使能，eMBState初值为STATE_NOT_INITIALIZED，在eMBInit()函数中被赋值为STATE_DISABLED,在eMBEnable函数中被赋值为STATE_ENABLE; 
*2:轮询EV_FRAME_RECEIVED事件发生，若EV_FRAME_RECEIVED事件发生，接收一帧报文数据，上报EV_EXECUTE事件，解析一帧报文，响应(发送)一帧数据给主机; 
*/  
//函数功能：总线侦听
//EV_RDY：表示系统刚刚进入侦听状态，则什么都不做
//EV_FRAME_RECEIVED：接收到完整的帧
//EV_EXECUTE：函数列表中检查有没有与命令字段相符合的函数来解析相应则执行该函数
eMBErrorCode eMBPoll( void )
{
    static UCHAR   *ucMBFrame;		//接收发送报文数据缓冲区
    static UCHAR    ucRcvAddress;	//modbus从机地址
    static UCHAR    ucFunctionCode;	//功能码
    static USHORT   usLength;		//报文长度
    static eMBException eException;	//错误码响应枚举
    int             i;
    eMBErrorCode    eStatus = MB_ENOERR;
    eMBEventType    eEvent;//事件标志枚举
	
    //检查协议栈是否准备好
    if( eMBState != STATE_ENABLED )
    {
        return MB_EILLSTATE;	//协议未使能
    }
    printf("poll\r\n");
	//获取事件 
    if( xMBPortEventGet( &eEvent ) == TRUE )
    {	
        printf("event: %d\r\n", eEvent);		 
        switch ( eEvent )				//判断事件类型
        {
			case EV_READY:				//启动完成
				break;
			case EV_FRAME_RECEIVED:		//帧接收完成
				eStatus = peMBFrameReceiveCur( &ucRcvAddress, &ucMBFrame, &usLength );	
				if( eStatus == MB_ENOERR )//报文长度和CRC校验正确
				{
					//检查数据是否是给我们，如果不是，则忽略此数据
					if( ( ucRcvAddress == ucMBAddress ) || ( ucRcvAddress == MB_ADDRESS_BROADCAST ) )
					{
                        printf("new event\r\n");
						( void )xMBPortEventPost( EV_EXECUTE );//将事件状态设置为执行功能码
					}
				}
				break;			
			case EV_EXECUTE:	//执行功能码
				ucFunctionCode = ucMBFrame[MB_PDU_FUNC_OFF];	//获取PDU中第一个字节为功能码
				eException = MB_EX_ILLEGAL_FUNCTION;			//赋错误码为无效的功能码
			
//				up_value=usSRegHoldBuf[0];
//				dn_value=usSRegHoldBuf[1];
//				set_value=usSRegHoldBuf[2];
                for( i = 0; i < 8; i++ )
                    printf("%#X, ", ucMBFrame[i]);
                printf("\r\n");
				for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
				{
                    printf(">");
					//没有此功能码
					if( xFuncHandlers[i].ucFunctionCode == 0 )
					{
						break;
					}
					//匹配到合适的功能码
					else if( xFuncHandlers[i].ucFunctionCode == ucFunctionCode )
					{
						eException = xFuncHandlers[i].pxHandler( ucMBFrame, &usLength );//调用相关功能
						break;
					}
				}
				//如果地址不是广播地址，我们将返回一个响应 
				if( ucRcvAddress != MB_ADDRESS_BROADCAST )
				{								
					if( eException != MB_EX_NONE )//报文有错误
					{
						usLength = 0;			
						ucMBFrame[usLength++] = ( UCHAR )( ucFunctionCode | MB_FUNC_ERROR );// 功能码|0x80则表示异常
						ucMBFrame[usLength++] = eException;//异常码
					}
					if( ( eMBCurrentMode == MB_ASCII ) && MB_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS )
					{
						vMBPortTimersDelay( MB_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS );							
					} 
                    for( i = 0; i < 8; i++ )
                        printf("%#X, ", ucMBFrame[i]);
                    printf("...%d, %d, %d\r\n", ucMBAddress, usLength, eException);               
					eStatus = peMBFrameSendCur( ucMBAddress, ucMBFrame, usLength );//将数据封装成帧，发送响应帧给主机（地址+功能码+异常码+CRC校验码）									
				}
				break;
			case EV_FRAME_SENT:		//执行帧发送
				break;
        }
    }
    return MB_ENOERR;
}

#endif
