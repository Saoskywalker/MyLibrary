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

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbrtu.h"
#include "mbframe.h"
#include "mbconfig.h"
#include "mbcrc.h"
#include "mbport.h"

#if MB_RTU_ENABLED > 0

/* ----------------------- Defines ------------------------------------------*/
#define MB_SER_PDU_SIZE_MIN     4       //PDU数据帧最小字节数（1字节地址+1字节命令+2字节校验） 
#define MB_SER_PDU_SIZE_MAX     256     //PDU数据帧最大字节数 
#define MB_SER_PDU_SIZE_CRC     2       //PDU数据帧中CRC字节数
#define MB_SER_PDU_ADDR_OFF     0       //PDU数据帧中地址数据的偏移量
#define MB_SER_PDU_PDU_OFF      1       //PDU数据帧起始偏移量

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_RX_INIT,              //接收机处于初始状态
    STATE_RX_IDLE,              //接收机处于空闲状态
    STATE_RX_RCV,               //接收机处于接收状态
    STATE_RX_ERROR              //接收数据发生错误
} eMBRcvState;

typedef enum
{
    STATE_TX_IDLE,              //发射机处于空闲状态
    STATE_TX_XMIT               //发射机处于传输状态
} eMBSndState;

/* ----------------------- Static variables ---------------------------------*/
static volatile eMBSndState eSndState;
static volatile eMBRcvState eRcvState;

volatile UCHAR  ucRTUBuf[MB_SER_PDU_SIZE_MAX];

static volatile UCHAR *pucSndBufferCur;		//发送ADU数据帧指针
static volatile USHORT usSndBufferCount;	//发送ADCU数据长度

static volatile USHORT usRcvBufferPos;		//接收数据长度

//函数功能：RTU传输模式初始化
//		   1.串口初始化
//		   2.定时器初始化
//参数 ucSlaveAddress：从机地址
//参数 ucPort：使用串口
//参数 ulBaudRate：串口波特率
//参数 eParity：校验方式
eMBErrorCode eMBRTUInit( UCHAR ucSlaveAddress, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    ULONG           usTimerT35_50us;

    ( void )ucSlaveAddress;
    ENTER_CRITICAL_SECTION(  );			//关全局中断

    if( xMBPortSerialInit( ucPort, ulBaudRate, 8, eParity ) != TRUE )	//串口初始化失败
    {
        eStatus = MB_EPORTERR;	//移植层错误	
    }
    else
    {
		//如果baudrate > 19200，那么我们应该使用固定的定时器值t35 = 1750us。否则t35必须是字符时间的3.5倍
        if( ulBaudRate > 19200 )	//串口波特率大于19200bps/s
        {
            usTimerT35_50us = 35;       //35*50=1750us，定时器1750us中断一次
        }
        else
        {
			//定时时间举例：
			//假设波特率 9600波特率1位数据的时间为： 1000000us/9600bit/s=104us
			//所以传输一个字节所需的时间为：    	 104us*10位  =1040us
			//所以 MODBUS确定一个数据帧完成的时间为：1040us*3.5=3.64ms ->10ms
            usTimerT35_50us = ( 7UL * 220000UL ) / ( 2UL * ulBaudRate );//设置定时器中断时间为3.5个字符时间
        }
        if( xMBPortTimersInit( ( USHORT ) usTimerT35_50us ) != TRUE )	//定时器初始化
        {
            eStatus = MB_EPORTERR;		//移植层错误	
        }
    }
    EXIT_CRITICAL_SECTION(  );			//开全局中断

    return eStatus;
}


//函数功能：启动RTU传输
void eMBRTUStart( void )
{
    ENTER_CRITICAL_SECTION(  );				//关全局中断
	
    //最初，接收方处于STATE_RX_INIT状态。我们启动计时器，
	//如果在t3.5中没有接收到字符，我们将更改为STATE_RX_IDLE。
	//这确保我们延迟modbus协议栈的启动，直到总线是空闲的。
    eRcvState = STATE_RX_INIT;				//设置接收机处于初始状态
    vMBPortSerialEnable( TRUE, FALSE );		//接收中断使能，发送中断失能
    vMBPortTimersEnable(  );				//定时器使能
	
    EXIT_CRITICAL_SECTION(  );				//开全局中断
}

//函数功能：停止RTU传输
void eMBRTUStop( void )
{
    ENTER_CRITICAL_SECTION(  );				//关全局中断
    vMBPortSerialEnable( FALSE, FALSE );	//接收中断失能，发送中断失能
    vMBPortTimersDisable(  );				//定时器使能能
    EXIT_CRITICAL_SECTION(  );				//开全局中断
}

//函数功能：将ADU数据帧中的PDU数据帧提取出来
//参数 pucRcvAddress：发送方的地址
//参数 pucFrame：接收发送报文数据缓冲区	 
//参数 pusLength：接收的数据长度（不包括地址和CRC校验码）
eMBErrorCode eMBRTUReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    BOOL            xFrameReceived = FALSE;
    eMBErrorCode    eStatus = MB_ENOERR;

    ENTER_CRITICAL_SECTION(  );						  //关全局中断
    __assert( usRcvBufferPos < MB_SER_PDU_SIZE_MAX );   //检验接收的数据长度

	//RTU数据帧长度和CRC校验
    if( ( usRcvBufferPos >= MB_SER_PDU_SIZE_MIN ) && ( usMBCRC16( ( UCHAR * ) ucRTUBuf, usRcvBufferPos ) == 0 ) )
    {
		//得到发送方的地址
        *pucRcvAddress = ucRTUBuf[MB_SER_PDU_ADDR_OFF];		

		//得到接收数据的长度
        *pusLength = ( USHORT )( usRcvBufferPos - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC );//PDU长度=ADU长度-1字节(地址)-2字节(CRC)

        //指向RTU数据帧首部
        *pucFrame = ( UCHAR * ) & ucRTUBuf[MB_SER_PDU_PDU_OFF];//PDU数据指针
        xFrameReceived = TRUE;			
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
eMBErrorCode eMBRTUSend( UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          usCRC16;

    ENTER_CRITICAL_SECTION(  );		//关全局中断
     
	//检查接收器是否空闲。STATE_RX_IDLE状态在T35定时中断中被设置。若接收器不在空闲状态，
    //说明主机正在向ModBus网络发送另一个帧。我们必须中断本次回应帧的发送。
    if( eRcvState == STATE_RX_IDLE )		//如果接收机处于接收空闲状态
    {
		//因为PDU帧首部是功能码，而ADU帧首部是地址，所以此时把数据帧后移一位，用来存地址
        pucSndBufferCur = ( UCHAR * ) pucFrame - 1;		
        usSndBufferCount = 1;

        //将PDU数据帧放进ADU数据帧
        pucSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;	//存入从机地址
        usSndBufferCount += usLength;

        //计算CRC16校验和
        usCRC16 = usMBCRC16( ( UCHAR * ) pucSndBufferCur, usSndBufferCount );
        ucRTUBuf[usSndBufferCount++] = ( UCHAR )( usCRC16 & 0xFF );
        ucRTUBuf[usSndBufferCount++] = ( UCHAR )( usCRC16 >> 8 );

        //使发射机处于传输状态
        eSndState = STATE_TX_XMIT;
			
		xMBPortSerialPutByte((CHAR)*pucSndBufferCur);//启动发送
		vMBPortSerialEnable( FALSE, TRUE );
		pucSndBufferCur++;
		usSndBufferCount--;	
    }
    else
    {
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION(  );//开全局中断
    return eStatus;
}

//接收处理函数：1.STATE_RX_INIT  初始状态
//			   2.STATE_RX_IDLE	空闲状态
//             3.STATE_RX_RCV	接收状态
//             4.STATE_RX_ERROR	接收错误状态
//此函数为函数指针，在串口接收中断中被调用
BOOL xMBRTUReceiveFSM( void )
{
    BOOL            xTaskNeedSwitch = FALSE;
    UCHAR           ucByte;

    __assert( eSndState == STATE_TX_IDLE );				//确保此时无数据发送，因为modbus不能支持接收和发送同时进行

    ( void )xMBPortSerialGetByte( ( CHAR * ) & ucByte );//从串口数据寄存器读取一个字节数据

    switch ( eRcvState )
    {		
		case STATE_RX_INIT:				//接收机处于初始状态	
			vMBPortTimersEnable(  );	//开启定时器，该定时器3.5T时间后发生中断，如果发生中断，说明已经接收完成一帧数据
			break;
		case STATE_RX_ERROR:			//接收机发生错误
			vMBPortTimersEnable(  );	//重新开启定时器
			break;
		case STATE_RX_IDLE:				//接收机处于空闲状态，说明此时可以开始接收数据
			usRcvBufferPos = 0;			//数据长度清零
			ucRTUBuf[usRcvBufferPos++] = ucByte;	//保存接收到的数据，同时数据长度加1
			eRcvState = STATE_RX_RCV;	//接收机状态设置为接收状态，因为此时已经开始接收数据	
			vMBPortTimersEnable(  );	//每收到一个字节，重启定时器
			break;
		case STATE_RX_RCV:				//接收机处于接收状态
			if( usRcvBufferPos < MB_SER_PDU_SIZE_MAX )  //如果接收长度小于最大数据帧长度
			{
				ucRTUBuf[usRcvBufferPos++] = ucByte;	
			}
			else										//如果接收长度大于最大数据帧长度，则接收机状态设置为发生错误		
			{
				eRcvState = STATE_RX_ERROR;
			}
			vMBPortTimersEnable(  );	//每收到一个字节，重启定时器
			break;
    }
    return xTaskNeedSwitch;
}

//发送处理函数：1.STATE_TX_IDLE 空闲状态，此状态下说明已经发送完成一次数据，所以将串口接收使能，发送失能
//			   2.STATE_TX_XMIT 发送状态，此状态下表明正在发送一帧数据，一帧数据发送完成过后，进入空闲状态失能串口发送
//此函数为函数指针，在串口发送中断中被调用
BOOL xMBRTUTransmitFSM( void )
{
    BOOL            xNeedPoll = FALSE;

    __assert( eRcvState == STATE_RX_IDLE );		//确保此时没有接收数据，因为modbus不能支持接收和发送同时进行

    switch ( eSndState )
    {	
		case STATE_TX_IDLE:							//数据发送完成，发送机进入空闲状态
			vMBPortSerialEnable( TRUE, FALSE );		//使能串口接收，失能串口发送
			break;
		case STATE_TX_XMIT:							//发送机处于发送状态
			if( usSndBufferCount != 0 )				//如果将要发送的数据长度不为0
			{
				xMBPortSerialPutByte( ( CHAR )*pucSndBufferCur );		//发送一字节数据
				pucSndBufferCur++;  				//准备发送下一个字节数据
				usSndBufferCount--;					//发送数据长度减1
			}
			else									//一帧数据发送完成
			{
				xNeedPoll = xMBPortEventPost( EV_FRAME_SENT );
				vMBPortSerialEnable( TRUE, FALSE );	//使能串口接收，失能串口发送
				eSndState = STATE_TX_IDLE;			//发送机设置为空闲状态，准备下一次的发送数据
			}
			break;
    }
    return xNeedPoll;
}

//此函数为函数指针，在定时器中断中被调用
BOOL xMBRTUTimerT35Expired( void )
{
    BOOL  xNeedPoll = FALSE;
	
    switch ( eRcvState )
    {
		//启动阶段完成
		//如果发生中断时，此时接收机处于初始化状态
		case STATE_RX_INIT:		//接收机处于初始化状态
			xNeedPoll = xMBPortEventPost( EV_READY );
			break;
		//表示完整接收到一个帧
		//如果发生中断时，此时接收机处于接收状态
		case STATE_RX_RCV:		//接收机处于接收状态
			xNeedPoll = xMBPortEventPost( EV_FRAME_RECEIVED );//向轮询函数发送事件
			break;
		case STATE_RX_ERROR:	//接收发生错误
			break;		
		default:		//函数在非法状态下调用
			__assert( ( eRcvState == STATE_RX_INIT ) ||
					( eRcvState == STATE_RX_RCV ) || ( eRcvState == STATE_RX_ERROR ) );
    }

    vMBPortTimersDisable(  );	//关闭定时器
    eRcvState = STATE_RX_IDLE;	//接收机处于空闲中断
    return xNeedPoll;
}

#endif

//一、初始化Modbus协议栈
//	1.验证从机地址是否符合要求
//	2.选择Modbus传输模式为RTU帧
//	3.RTU传输模式初始化
//		①串口初始化（波特率为9600kps/s）
//		②定时器初始化（设置中断时间为3.5个字符时间）
//	4.设置协议使能状态（ eMBState = STATE_DISABLED ）
//二、启用Modbus协议栈
//	1.启动RTU传输
//		①设置接收机处于初始状态（ eRcvState = STATE_RX_INIT ）
//		②使能串口接收中断使能，发送中断失能
//		③使能定时器中断，计数清零
//	2.设置协议使能状态（ eMBState = STATE_ENABLED ）
//
/* 注;1.自此协议栈已经开始启动，串口可以开始接收数据 
 *	  2.在串口接收中断函数和发送中断函数里有各自的状态机函数
 *	  3.在定时器中断里有状态机处理函数
 */

//三、接收数据过程
//	1.因为没有接收到数据，定时器第一次发生中断
//		①.此时接收机处于初始状态（ eRcvState = STATE_RX_INIT ）
//		②.发送启动事件（ eEvent=EV_READY ）
//		③.关闭定时器（ 失能定时器中断，计数清零 ）
//		④.设置接收机为空闲状态（ eRcvState = STATE_RX_IDLE ）
//	2.一段时间后，串口接收到数据
//		①.此时接收机处于空闲状态（ eRcvState = STATE_RX_IDLE ）
//		②.此时可以开始接收数据
//		③.设置接收机为空闲状态（ eRcvState = STATE_RX_RCV ）
//		④.重新启动定时器（使能定时中断，计数清零）
//		⑤.继续接收数据，每接收一次，重启一次定时器，直到接收数据长度大于数据帧长度
//		⑥.如果接收数据长度大于数据帧长度发生定时器中断，则数据不处理；否则，发生定时器中断说明一帧数据接收完成，则发送帧接收完成事件（ eEvent=EV_FRAME_RECEIVED ）

//四、发送数据过程
//	1.判断接收机是否处于接收空闲状态
//	2.将PDU数据帧打包成ADU数据帧（地址+功能码+异常码）
//	3.将发送机状态设置为传输状态（ eSndState = STATE_TX_XMIT ）
//	4.使能串口发送中断，失能接收中断
//	5.发送完成后，发送帧发送完成事件（ eEvent=EV_FRAME_SENT ）
//	6.使能串口接收，失能串口发送
//	7.发送机设置为空闲状态，准备下一次的发送数据（ eSndState = STATE_TX_IDLE ） 
