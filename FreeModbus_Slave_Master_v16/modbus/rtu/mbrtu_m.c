/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2013 China Beijing Armink <armink.ztl@gmail.com>
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
 * File: $Id: mbrtu_m.c,v 1.60 2013/08/17 11:42:56 Armink Add Master Functions $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_m.h"
#include "mbrtu.h"
#include "mbframe.h"
#include "mbcrc.h"
#include "mbport.h"
#include "mbconfig.h"

#if MB_MASTER_RTU_ENABLED > 0
/* ----------------------- Defines ------------------------------------------*/
#define MB_SER_PDU_SIZE_MIN     4       /*!< Minimum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_MAX     256     /*!< Maximum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_CRC     2       /*!< Size of CRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_M_RX_INIT,              /*!< Receiver is in initial state. */
    STATE_M_RX_IDLE,              /*!< Receiver is in idle state. */
    STATE_M_RX_RCV,               /*!< Frame is beeing received. */
    STATE_M_RX_ERROR,              /*!< If the frame is invalid. */
} eMBMasterRcvState;

typedef enum
{
    STATE_M_TX_IDLE,              /*!< Transmitter is in idle state. */
    STATE_M_TX_XMIT,              /*!< Transmitter is in transfer state. */
    STATE_M_TX_XFWR,              /*!< Transmitter is in transfer finish and wait receive state. */
} eMBMasterSndState;

/* ----------------------- Static variables ---------------------------------*/
static volatile eMBMasterSndState eSndState;
static volatile eMBMasterRcvState eRcvState;

volatile UCHAR  ucMasterRTUSndBuf[MB_PDU_SIZE_MAX];
volatile UCHAR  ucMasterRTURcvBuf[MB_SER_PDU_SIZE_MAX];
volatile USHORT usMasterSendPDULength;

static volatile UCHAR *pucMasterSndBufferCur;
static volatile USHORT usMasterSndBufferCount;

static volatile USHORT usMasterRcvBufferPos;
static volatile BOOL   xFrameIsBroadcast = FALSE;

static volatile eMBMasterTimerMode eMasterCurTimerMode;

/* ----------------------- Start implementation -----------------------------*/
//函数功能：RTU传输模式初始化
//		   1.串口初始化
//		   2.定时器初始化
//参数 ucSlaveAddress：从机地址
//参数 ucPort：使用串口
//参数 ulBaudRate：串口波特率
//参数 eParity：校验方式
eMBErrorCode eMBMasterRTUInit(UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    ULONG           usTimerT35_50us;

    ENTER_CRITICAL_SECTION(  );

    if( xMBMasterPortSerialInit( ucPort, ulBaudRate, 8, eParity ) != TRUE )	//串口初始化失败
    {
        eStatus = MB_EPORTERR;
    }
    else
    {
       //如果baudrate > 19200，那么我们应该使用固定的定时器值t35 = 1750us。否则t35必须是字符时间的3.5倍
        if( ulBaudRate > 19200 )		//串口波特率大于19200bps/s
        {
            usTimerT35_50us = 35;      //35*50=1750us，定时器1750us中断一次
        }
        else
        {
			//定时时间举例：
			//假设波特率 9600波特率1位数据的时间为： 1000000us/9600bit/s=104us
			//所以传输一个字节所需的时间为：    	 104us*10位  =1040us
			//所以 MODBUS确定一个数据帧完成的时间为：1040us*3.5=3.64ms ->10ms
            usTimerT35_50us = ( 7UL * 220000UL ) / ( 2UL * ulBaudRate );	//设置定时器中断时间为3.5个字符时间
        }
        if( xMBMasterPortTimersInit( ( USHORT ) usTimerT35_50us ) != TRUE )
        {
            eStatus = MB_EPORTERR;
        }
    }
    EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

//函数功能：启动RTU传输
void eMBMasterRTUStart( void )
{
	ENTER_CRITICAL_SECTION(  );					//关全局中断
    //最初，接收方处于STATE_RX_INIT状态。我们启动计时器，
	//如果在t3.5中没有接收到字符，我们将更改为STATE_RX_IDLE。
	//这确保我们延迟modbus协议栈的启动，直到总线是空闲的。
    eRcvState = STATE_M_RX_INIT;				//设置接收机处于初始状态
    vMBMasterPortSerialEnable( TRUE, FALSE );	//接收中断使能，发送中断失能
    vMBMasterPortTimersT35Enable(  );			//定时器使能

   EXIT_CRITICAL_SECTION(  );					//开全局中断
}

//函数功能：停止RTU传输
void eMBMasterRTUStop( void )
{
	ENTER_CRITICAL_SECTION(  );					//关全局中断
    vMBMasterPortSerialEnable( FALSE, FALSE );	//接收中断失能，发送中断失能
    vMBMasterPortTimersDisable(  );				//定时器使能能
    EXIT_CRITICAL_SECTION(  );					//开全局中断			
}

//函数功能：将ADU数据帧中的PDU数据帧提取出来
//参数 pucRcvAddress：发送方的地址
//参数 pucFrame：接收发送报文数据缓冲区	 
//参数 pusLength：接收的数据长度（不包括地址和CRC校验码）
eMBErrorCode eMBMasterRTUReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    ENTER_CRITICAL_SECTION(  );									//关全局中断
    __assert( usMasterRcvBufferPos < MB_SER_PDU_SIZE_MAX ); //检验接收的数据长度

    //RTU数据帧长度和CRC校验
    if( ( usMasterRcvBufferPos >= MB_SER_PDU_SIZE_MIN )
        && ( usMBCRC16( ( UCHAR * ) ucMasterRTURcvBuf, usMasterRcvBufferPos ) == 0 ) )
    {
        //得到发送方的地址
        *pucRcvAddress = ucMasterRTURcvBuf[MB_SER_PDU_ADDR_OFF];
		
		//得到接收数据的长度
        *pusLength = ( USHORT )( usMasterRcvBufferPos - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC );	//PDU长度=ADU长度-1字节(地址)-2字节(CRC)

        //指向RTU数据帧首部
        *pucFrame = ( UCHAR * ) & ucMasterRTURcvBuf[MB_SER_PDU_PDU_OFF];
    }
    else
    {
        eStatus = MB_EIO;
    }

    EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

//函数功能;将PDU数据帧打包成ADU数据帧
//参数 ucSlaveAddress：从机地址
//参数 pucFrame：数据帧首部
//参数 usLength：数据长度
eMBErrorCode eMBMasterRTUSend( UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          usCRC16;

    if ( ucSlaveAddress > MB_MASTER_TOTAL_SLAVE_NUM ) return MB_EINVAL;
    ENTER_CRITICAL_SECTION(  );		//关全局中断

	//检查接收器是否空闲。STATE_RX_IDLE状态在T35定时中断中被设置。若接收器不在空闲状态，
    //说明主机正在向ModBus网络发送另一个帧。我们必须中断本次回应帧的发送。
    if( eRcvState == STATE_M_RX_IDLE )			//如果接收机处于接收空闲状态
    {
        //因为PDU帧首部是功能码，而ADU帧首部是地址，所以此时把数据帧后移一位，用来存地址
        pucMasterSndBufferCur = ( UCHAR * ) pucFrame - 1;
        usMasterSndBufferCount = 1;

        //将PDU数据帧放进ADU数据帧
        pucMasterSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
        usMasterSndBufferCount += usLength;

        //计算CRC16校验和
        usCRC16 = usMBCRC16( ( UCHAR * ) pucMasterSndBufferCur, usMasterSndBufferCount );
        ucMasterRTUSndBuf[usMasterSndBufferCount++] = ( UCHAR )( usCRC16 & 0xFF );
        ucMasterRTUSndBuf[usMasterSndBufferCount++] = ( UCHAR )( usCRC16 >> 8 );

        //使发射机处于传输状态
        eSndState = STATE_M_TX_XMIT;
		xMBMasterPortSerialPutByte((CHAR)*pucMasterSndBufferCur);//启动发送	
        vMBMasterPortSerialEnable( FALSE, TRUE );
		pucMasterSndBufferCur++;
		usMasterSndBufferCount--;		
    }
    else
    {
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

//接收处理函数：1.STATE_M_RX_INIT  	初始状态
//			   2.STATE_M_RX_IDLE	空闲状态
//             3.STATE_M_RX_RCV		接收状态
//             4.STATE_M_RX_ERROR	接收错误状态
//此函数为函数指针，在串口接收中断中被调用
BOOL xMBMasterRTUReceiveFSM( void )
{
    BOOL            xTaskNeedSwitch = FALSE;
    UCHAR           ucByte;

	//确保此时无数据发送，因为modbus不能支持接收和发送同时进行
    __assert(( eSndState == STATE_M_TX_IDLE ) || ( eSndState == STATE_M_TX_XFWR ));

    ( void )xMBMasterPortSerialGetByte( ( CHAR * ) & ucByte );		//从串口数据寄存器读取一个字节数据

    switch ( eRcvState )
    {
		case STATE_M_RX_INIT:						//接收机处于初始状态	
			vMBMasterPortTimersT35Enable( );		//设置定时器超时模式为：3.5帧字符超时
			break;
		case STATE_M_RX_ERROR:						//接收机发生错误
			vMBMasterPortTimersT35Enable( );		//设置定时器超时模式为：3.5帧字符超时
			break;
		case STATE_M_RX_IDLE:						//接收机处于空闲状态，说明此时可以开始接收数据
			vMBMasterPortTimersDisable( );			//关闭定时器
			eSndState = STATE_M_TX_IDLE;			//发送状态机设置为空闲状态

			usMasterRcvBufferPos = 0;				//数据长度清零
			ucMasterRTURcvBuf[usMasterRcvBufferPos++] = ucByte;	//保存接收到的数据，同时数据长度加1
			eRcvState = STATE_M_RX_RCV;				//接收机状态设置为接收状态，因为此时已经开始接收数据	

			vMBMasterPortTimersT35Enable( );		//设置定时器超时模式为：3.5帧字符
			break;
		case STATE_M_RX_RCV:									//接收机处于接收状态
			if( usMasterRcvBufferPos < MB_SER_PDU_SIZE_MAX ) 	//如果接收长度小于最大数据帧长度
			{
				ucMasterRTURcvBuf[usMasterRcvBufferPos++] = ucByte;
			}
			else												//如果接收长度大于最大数据帧长度，则接收机状态设置为发生错误
			{
				eRcvState = STATE_M_RX_ERROR;
			}
			vMBMasterPortTimersT35Enable();						//设置定时器超时模式为：3.5帧字符超时
			break;
    }
    return xTaskNeedSwitch;
}

//发送处理函数：1.STATE_M_TX_IDLE 空闲状态，此状态下说明已经发送完成一次数据，所以将串口接收使能，发送失能
//			   2.STATE_M_TX_XMIT 发送状态，此状态下表明正在发送一帧数据，一帧数据发送完成过后，进入空闲状态失能串口发送
//此函数为函数指针，在串口发送中断中被调用
BOOL xMBMasterRTUTransmitFSM( void )
{
    BOOL            xNeedPoll = FALSE;

    __assert( eRcvState == STATE_M_RX_IDLE );//确保此时没有接收数据，因为modbus不能支持接收和发送同时进行

    switch ( eSndState )
    {
		case STATE_M_TX_IDLE:									//数据发送完成，发送机进入空闲状态
			vMBMasterPortSerialEnable( TRUE, FALSE );			//使能串口接收，失能串口发送
			break;
		case STATE_M_TX_XMIT:									//发送机处于发送状态
			if( usMasterSndBufferCount != 0 )					//如果将要发送的数据长度不为0
			{
				xMBMasterPortSerialPutByte( ( CHAR )*pucMasterSndBufferCur );//发送一字节数据
				pucMasterSndBufferCur++;	 					//准备发送下一个字节数据
				usMasterSndBufferCount--;						//发送数据长度减1
			}
			else
			{
				//判断发送地址是广播地址还是从机地址
				xFrameIsBroadcast = ( ucMasterRTUSndBuf[MB_SER_PDU_ADDR_OFF] == MB_ADDRESS_BROADCAST ) ? TRUE : FALSE;
				vMBMasterPortSerialEnable( TRUE, FALSE );		//使能串口接收，失能串口发送
				eSndState = STATE_M_TX_XFWR;					//设置发送机为传送结束，此时等待接收响应状态
				if ( xFrameIsBroadcast == TRUE )
				{
					vMBMasterPortTimersConvertDelayEnable( ); //设置定时器超时模式为：转换延时
				}
				else
				{
					vMBMasterPortTimersRespondTimeoutEnable( );	//设置定时器超时模式为：响应超时
				}
			}
			break;
    }

    return xNeedPoll;
}

//此函数为函数指针，在定时器中断中被调用
BOOL xMBMasterRTUTimerExpired(void)
{
	BOOL xNeedPoll = FALSE;

	switch (eRcvState)
	{
		//启动阶段完成
		case STATE_M_RX_INIT:	//接收机处于初始化状态
			xNeedPoll = xMBMasterPortEventPost(EV_MASTER_READY);
			break;
		//表示完整接收到一个帧
		case STATE_M_RX_RCV:	//接收机处于接收状态
			xNeedPoll = xMBMasterPortEventPost(EV_MASTER_FRAME_RECEIVED);
			break;
		case STATE_M_RX_ERROR:	//接收发生错误
			vMBMasterSetErrorType(EV_ERROR_RECEIVE_DATA);					//设置发生错误类型
			xNeedPoll = xMBMasterPortEventPost( EV_MASTER_ERROR_PROCESS );	
			break;
		default:		//函数在非法状态下调用
			__assert(
					( eRcvState == STATE_M_RX_INIT )  || ( eRcvState == STATE_M_RX_RCV ) ||
					( eRcvState == STATE_M_RX_ERROR ) || ( eRcvState == STATE_M_RX_IDLE ));
			break;
	}
	eRcvState = STATE_M_RX_IDLE;  //接收机处于空闲中断

	switch (eSndState)
	{
			/* A frame was send finish and convert delay or respond timeout expired.
			 * If the frame is broadcast,The master will idle,and if the frame is not
			 * broadcast.Notify the listener process error.*/
		case STATE_M_TX_XFWR:
			if ( xFrameIsBroadcast == FALSE ) 		//如果不是广播地址
			{
				vMBMasterSetErrorType(EV_ERROR_RESPOND_TIMEOUT);	//设置错误类型为接收响应超时
				xNeedPoll = xMBMasterPortEventPost(EV_MASTER_ERROR_PROCESS);
			}
			break;
		default:
			__assert(( eSndState == STATE_M_TX_XFWR ) || ( eSndState == STATE_M_TX_IDLE ));
			break;
	}
	eSndState = STATE_M_TX_IDLE;		//发送机处于空闲中断

	vMBMasterPortTimersDisable( );		//关闭定时器
	if (eMasterCurTimerMode == MB_TMODE_CONVERT_DELAY)		//如果定时器超时模式为：转换延时
	{
		xNeedPoll = xMBMasterPortEventPost( EV_MASTER_EXECUTE );
	}

	return xNeedPoll;
}

/* Get Modbus Master send RTU's buffer address pointer.*/
void vMBMasterGetRTUSndBuf( UCHAR ** pucFrame )
{
	*pucFrame = ( UCHAR * ) ucMasterRTUSndBuf;
}

//获取Modbus主发送PDU的缓冲区地址指针
void vMBMasterGetPDUSndBuf( UCHAR ** pucFrame )
{
	*pucFrame = ( UCHAR * ) &ucMasterRTUSndBuf[MB_SER_PDU_PDU_OFF];
}

/* Set Modbus Master send PDU's buffer length.*/
void vMBMasterSetPDUSndLength( USHORT SendPDULength )
{
	usMasterSendPDULength = SendPDULength;
}

/* Get Modbus Master send PDU's buffer length.*/
USHORT usMBMasterGetPDUSndLength( void )
{
	return usMasterSendPDULength;
}

/* Set Modbus Master current timer mode.*/
void vMBMasterSetCurTimerMode( eMBMasterTimerMode eMBTimerMode )
{
	eMasterCurTimerMode = eMBTimerMode;
}

/* The master request is broadcast? */
BOOL xMBMasterRequestIsBroadcast( void ){
	return xFrameIsBroadcast;
}
#endif

