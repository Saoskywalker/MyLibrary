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
//�������ܣ�RTU����ģʽ��ʼ��
//		   1.���ڳ�ʼ��
//		   2.��ʱ����ʼ��
//���� ucSlaveAddress���ӻ���ַ
//���� ucPort��ʹ�ô���
//���� ulBaudRate�����ڲ�����
//���� eParity��У�鷽ʽ
eMBErrorCode eMBMasterRTUInit(UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    ULONG           usTimerT35_50us;

    ENTER_CRITICAL_SECTION(  );

    if( xMBMasterPortSerialInit( ucPort, ulBaudRate, 8, eParity ) != TRUE )	//���ڳ�ʼ��ʧ��
    {
        eStatus = MB_EPORTERR;
    }
    else
    {
       //���baudrate > 19200����ô����Ӧ��ʹ�ù̶��Ķ�ʱ��ֵt35 = 1750us������t35�������ַ�ʱ���3.5��
        if( ulBaudRate > 19200 )		//���ڲ����ʴ���19200bps/s
        {
            usTimerT35_50us = 35;      //35*50=1750us����ʱ��1750us�ж�һ��
        }
        else
        {
			//��ʱʱ�������
			//���貨���� 9600������1λ���ݵ�ʱ��Ϊ�� 1000000us/9600bit/s=104us
			//���Դ���һ���ֽ������ʱ��Ϊ��    	 104us*10λ  =1040us
			//���� MODBUSȷ��һ������֡��ɵ�ʱ��Ϊ��1040us*3.5=3.64ms ->10ms
            usTimerT35_50us = ( 7UL * 220000UL ) / ( 2UL * ulBaudRate );	//���ö�ʱ���ж�ʱ��Ϊ3.5���ַ�ʱ��
        }
        if( xMBMasterPortTimersInit( ( USHORT ) usTimerT35_50us ) != TRUE )
        {
            eStatus = MB_EPORTERR;
        }
    }
    EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

//�������ܣ�����RTU����
void eMBMasterRTUStart( void )
{
	ENTER_CRITICAL_SECTION(  );					//��ȫ���ж�
    //��������շ�����STATE_RX_INIT״̬������������ʱ����
	//�����t3.5��û�н��յ��ַ������ǽ�����ΪSTATE_RX_IDLE��
	//��ȷ�������ӳ�modbusЭ��ջ��������ֱ�������ǿ��еġ�
    eRcvState = STATE_M_RX_INIT;				//���ý��ջ����ڳ�ʼ״̬
    vMBMasterPortSerialEnable( TRUE, FALSE );	//�����ж�ʹ�ܣ������ж�ʧ��
    vMBMasterPortTimersT35Enable(  );			//��ʱ��ʹ��

   EXIT_CRITICAL_SECTION(  );					//��ȫ���ж�
}

//�������ܣ�ֹͣRTU����
void eMBMasterRTUStop( void )
{
	ENTER_CRITICAL_SECTION(  );					//��ȫ���ж�
    vMBMasterPortSerialEnable( FALSE, FALSE );	//�����ж�ʧ�ܣ������ж�ʧ��
    vMBMasterPortTimersDisable(  );				//��ʱ��ʹ����
    EXIT_CRITICAL_SECTION(  );					//��ȫ���ж�			
}

//�������ܣ���ADU����֡�е�PDU����֡��ȡ����
//���� pucRcvAddress�����ͷ��ĵ�ַ
//���� pucFrame�����շ��ͱ������ݻ�����	 
//���� pusLength�����յ����ݳ��ȣ���������ַ��CRCУ���룩
eMBErrorCode eMBMasterRTUReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    ENTER_CRITICAL_SECTION(  );									//��ȫ���ж�
    __assert( usMasterRcvBufferPos < MB_SER_PDU_SIZE_MAX ); //������յ����ݳ���

    //RTU����֡���Ⱥ�CRCУ��
    if( ( usMasterRcvBufferPos >= MB_SER_PDU_SIZE_MIN )
        && ( usMBCRC16( ( UCHAR * ) ucMasterRTURcvBuf, usMasterRcvBufferPos ) == 0 ) )
    {
        //�õ����ͷ��ĵ�ַ
        *pucRcvAddress = ucMasterRTURcvBuf[MB_SER_PDU_ADDR_OFF];
		
		//�õ��������ݵĳ���
        *pusLength = ( USHORT )( usMasterRcvBufferPos - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC );	//PDU����=ADU����-1�ֽ�(��ַ)-2�ֽ�(CRC)

        //ָ��RTU����֡�ײ�
        *pucFrame = ( UCHAR * ) & ucMasterRTURcvBuf[MB_SER_PDU_PDU_OFF];
    }
    else
    {
        eStatus = MB_EIO;
    }

    EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

//��������;��PDU����֡�����ADU����֡
//���� ucSlaveAddress���ӻ���ַ
//���� pucFrame������֡�ײ�
//���� usLength�����ݳ���
eMBErrorCode eMBMasterRTUSend( UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          usCRC16;

    if ( ucSlaveAddress > MB_MASTER_TOTAL_SLAVE_NUM ) return MB_EINVAL;
    ENTER_CRITICAL_SECTION(  );		//��ȫ���ж�

	//���������Ƿ���С�STATE_RX_IDLE״̬��T35��ʱ�ж��б����á������������ڿ���״̬��
    //˵������������ModBus���緢����һ��֡�����Ǳ����жϱ��λ�Ӧ֡�ķ��͡�
    if( eRcvState == STATE_M_RX_IDLE )			//������ջ����ڽ��տ���״̬
    {
        //��ΪPDU֡�ײ��ǹ����룬��ADU֡�ײ��ǵ�ַ�����Դ�ʱ������֡����һλ���������ַ
        pucMasterSndBufferCur = ( UCHAR * ) pucFrame - 1;
        usMasterSndBufferCount = 1;

        //��PDU����֡�Ž�ADU����֡
        pucMasterSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
        usMasterSndBufferCount += usLength;

        //����CRC16У���
        usCRC16 = usMBCRC16( ( UCHAR * ) pucMasterSndBufferCur, usMasterSndBufferCount );
        ucMasterRTUSndBuf[usMasterSndBufferCount++] = ( UCHAR )( usCRC16 & 0xFF );
        ucMasterRTUSndBuf[usMasterSndBufferCount++] = ( UCHAR )( usCRC16 >> 8 );

        //ʹ��������ڴ���״̬
        eSndState = STATE_M_TX_XMIT;
		xMBMasterPortSerialPutByte((CHAR)*pucMasterSndBufferCur);//��������	
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

//���մ�������1.STATE_M_RX_INIT  	��ʼ״̬
//			   2.STATE_M_RX_IDLE	����״̬
//             3.STATE_M_RX_RCV		����״̬
//             4.STATE_M_RX_ERROR	���մ���״̬
//�˺���Ϊ����ָ�룬�ڴ��ڽ����ж��б�����
BOOL xMBMasterRTUReceiveFSM( void )
{
    BOOL            xTaskNeedSwitch = FALSE;
    UCHAR           ucByte;

	//ȷ����ʱ�����ݷ��ͣ���Ϊmodbus����֧�ֽ��պͷ���ͬʱ����
    __assert(( eSndState == STATE_M_TX_IDLE ) || ( eSndState == STATE_M_TX_XFWR ));

    ( void )xMBMasterPortSerialGetByte( ( CHAR * ) & ucByte );		//�Ӵ������ݼĴ�����ȡһ���ֽ�����

    switch ( eRcvState )
    {
		case STATE_M_RX_INIT:						//���ջ����ڳ�ʼ״̬	
			vMBMasterPortTimersT35Enable( );		//���ö�ʱ����ʱģʽΪ��3.5֡�ַ���ʱ
			break;
		case STATE_M_RX_ERROR:						//���ջ���������
			vMBMasterPortTimersT35Enable( );		//���ö�ʱ����ʱģʽΪ��3.5֡�ַ���ʱ
			break;
		case STATE_M_RX_IDLE:						//���ջ����ڿ���״̬��˵����ʱ���Կ�ʼ��������
			vMBMasterPortTimersDisable( );			//�رն�ʱ��
			eSndState = STATE_M_TX_IDLE;			//����״̬������Ϊ����״̬

			usMasterRcvBufferPos = 0;				//���ݳ�������
			ucMasterRTURcvBuf[usMasterRcvBufferPos++] = ucByte;	//������յ������ݣ�ͬʱ���ݳ��ȼ�1
			eRcvState = STATE_M_RX_RCV;				//���ջ�״̬����Ϊ����״̬����Ϊ��ʱ�Ѿ���ʼ��������	

			vMBMasterPortTimersT35Enable( );		//���ö�ʱ����ʱģʽΪ��3.5֡�ַ�
			break;
		case STATE_M_RX_RCV:									//���ջ����ڽ���״̬
			if( usMasterRcvBufferPos < MB_SER_PDU_SIZE_MAX ) 	//������ճ���С���������֡����
			{
				ucMasterRTURcvBuf[usMasterRcvBufferPos++] = ucByte;
			}
			else												//������ճ��ȴ����������֡���ȣ�����ջ�״̬����Ϊ��������
			{
				eRcvState = STATE_M_RX_ERROR;
			}
			vMBMasterPortTimersT35Enable();						//���ö�ʱ����ʱģʽΪ��3.5֡�ַ���ʱ
			break;
    }
    return xTaskNeedSwitch;
}

//���ʹ�������1.STATE_M_TX_IDLE ����״̬����״̬��˵���Ѿ��������һ�����ݣ����Խ����ڽ���ʹ�ܣ�����ʧ��
//			   2.STATE_M_TX_XMIT ����״̬����״̬�±������ڷ���һ֡���ݣ�һ֡���ݷ�����ɹ��󣬽������״̬ʧ�ܴ��ڷ���
//�˺���Ϊ����ָ�룬�ڴ��ڷ����ж��б�����
BOOL xMBMasterRTUTransmitFSM( void )
{
    BOOL            xNeedPoll = FALSE;

    __assert( eRcvState == STATE_M_RX_IDLE );//ȷ����ʱû�н������ݣ���Ϊmodbus����֧�ֽ��պͷ���ͬʱ����

    switch ( eSndState )
    {
		case STATE_M_TX_IDLE:									//���ݷ�����ɣ����ͻ��������״̬
			vMBMasterPortSerialEnable( TRUE, FALSE );			//ʹ�ܴ��ڽ��գ�ʧ�ܴ��ڷ���
			break;
		case STATE_M_TX_XMIT:									//���ͻ����ڷ���״̬
			if( usMasterSndBufferCount != 0 )					//�����Ҫ���͵����ݳ��Ȳ�Ϊ0
			{
				xMBMasterPortSerialPutByte( ( CHAR )*pucMasterSndBufferCur );//����һ�ֽ�����
				pucMasterSndBufferCur++;	 					//׼��������һ���ֽ�����
				usMasterSndBufferCount--;						//�������ݳ��ȼ�1
			}
			else
			{
				//�жϷ��͵�ַ�ǹ㲥��ַ���Ǵӻ���ַ
				xFrameIsBroadcast = ( ucMasterRTUSndBuf[MB_SER_PDU_ADDR_OFF] == MB_ADDRESS_BROADCAST ) ? TRUE : FALSE;
				vMBMasterPortSerialEnable( TRUE, FALSE );		//ʹ�ܴ��ڽ��գ�ʧ�ܴ��ڷ���
				eSndState = STATE_M_TX_XFWR;					//���÷��ͻ�Ϊ���ͽ�������ʱ�ȴ�������Ӧ״̬
				if ( xFrameIsBroadcast == TRUE )
				{
					vMBMasterPortTimersConvertDelayEnable( ); //���ö�ʱ����ʱģʽΪ��ת����ʱ
				}
				else
				{
					vMBMasterPortTimersRespondTimeoutEnable( );	//���ö�ʱ����ʱģʽΪ����Ӧ��ʱ
				}
			}
			break;
    }

    return xNeedPoll;
}

//�˺���Ϊ����ָ�룬�ڶ�ʱ���ж��б�����
BOOL xMBMasterRTUTimerExpired(void)
{
	BOOL xNeedPoll = FALSE;

	switch (eRcvState)
	{
		//�����׶����
		case STATE_M_RX_INIT:	//���ջ����ڳ�ʼ��״̬
			xNeedPoll = xMBMasterPortEventPost(EV_MASTER_READY);
			break;
		//��ʾ�������յ�һ��֡
		case STATE_M_RX_RCV:	//���ջ����ڽ���״̬
			xNeedPoll = xMBMasterPortEventPost(EV_MASTER_FRAME_RECEIVED);
			break;
		case STATE_M_RX_ERROR:	//���շ�������
			vMBMasterSetErrorType(EV_ERROR_RECEIVE_DATA);					//���÷�����������
			xNeedPoll = xMBMasterPortEventPost( EV_MASTER_ERROR_PROCESS );	
			break;
		default:		//�����ڷǷ�״̬�µ���
			__assert(
					( eRcvState == STATE_M_RX_INIT )  || ( eRcvState == STATE_M_RX_RCV ) ||
					( eRcvState == STATE_M_RX_ERROR ) || ( eRcvState == STATE_M_RX_IDLE ));
			break;
	}
	eRcvState = STATE_M_RX_IDLE;  //���ջ����ڿ����ж�

	switch (eSndState)
	{
			/* A frame was send finish and convert delay or respond timeout expired.
			 * If the frame is broadcast,The master will idle,and if the frame is not
			 * broadcast.Notify the listener process error.*/
		case STATE_M_TX_XFWR:
			if ( xFrameIsBroadcast == FALSE ) 		//������ǹ㲥��ַ
			{
				vMBMasterSetErrorType(EV_ERROR_RESPOND_TIMEOUT);	//���ô�������Ϊ������Ӧ��ʱ
				xNeedPoll = xMBMasterPortEventPost(EV_MASTER_ERROR_PROCESS);
			}
			break;
		default:
			__assert(( eSndState == STATE_M_TX_XFWR ) || ( eSndState == STATE_M_TX_IDLE ));
			break;
	}
	eSndState = STATE_M_TX_IDLE;		//���ͻ����ڿ����ж�

	vMBMasterPortTimersDisable( );		//�رն�ʱ��
	if (eMasterCurTimerMode == MB_TMODE_CONVERT_DELAY)		//�����ʱ����ʱģʽΪ��ת����ʱ
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

//��ȡModbus������PDU�Ļ�������ַָ��
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

