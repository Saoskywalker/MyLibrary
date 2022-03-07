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
#define MB_SER_PDU_SIZE_MIN     4       //PDU����֡��С�ֽ�����1�ֽڵ�ַ+1�ֽ�����+2�ֽ�У�飩 
#define MB_SER_PDU_SIZE_MAX     256     //PDU����֡����ֽ��� 
#define MB_SER_PDU_SIZE_CRC     2       //PDU����֡��CRC�ֽ���
#define MB_SER_PDU_ADDR_OFF     0       //PDU����֡�е�ַ���ݵ�ƫ����
#define MB_SER_PDU_PDU_OFF      1       //PDU����֡��ʼƫ����

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_RX_INIT,              //���ջ����ڳ�ʼ״̬
    STATE_RX_IDLE,              //���ջ����ڿ���״̬
    STATE_RX_RCV,               //���ջ����ڽ���״̬
    STATE_RX_ERROR              //�������ݷ�������
} eMBRcvState;

typedef enum
{
    STATE_TX_IDLE,              //��������ڿ���״̬
    STATE_TX_XMIT               //��������ڴ���״̬
} eMBSndState;

/* ----------------------- Static variables ---------------------------------*/
static volatile eMBSndState eSndState;
static volatile eMBRcvState eRcvState;

volatile UCHAR  ucRTUBuf[MB_SER_PDU_SIZE_MAX];

static volatile UCHAR *pucSndBufferCur;		//����ADU����ָ֡��
static volatile USHORT usSndBufferCount;	//����ADCU���ݳ���

static volatile USHORT usRcvBufferPos;		//�������ݳ���

//�������ܣ�RTU����ģʽ��ʼ��
//		   1.���ڳ�ʼ��
//		   2.��ʱ����ʼ��
//���� ucSlaveAddress���ӻ���ַ
//���� ucPort��ʹ�ô���
//���� ulBaudRate�����ڲ�����
//���� eParity��У�鷽ʽ
eMBErrorCode eMBRTUInit( UCHAR ucSlaveAddress, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    ULONG           usTimerT35_50us;

    ( void )ucSlaveAddress;
    ENTER_CRITICAL_SECTION(  );			//��ȫ���ж�

    if( xMBPortSerialInit( ucPort, ulBaudRate, 8, eParity ) != TRUE )	//���ڳ�ʼ��ʧ��
    {
        eStatus = MB_EPORTERR;	//��ֲ�����	
    }
    else
    {
		//���baudrate > 19200����ô����Ӧ��ʹ�ù̶��Ķ�ʱ��ֵt35 = 1750us������t35�������ַ�ʱ���3.5��
        if( ulBaudRate > 19200 )	//���ڲ����ʴ���19200bps/s
        {
            usTimerT35_50us = 35;       //35*50=1750us����ʱ��1750us�ж�һ��
        }
        else
        {
			//��ʱʱ�������
			//���貨���� 9600������1λ���ݵ�ʱ��Ϊ�� 1000000us/9600bit/s=104us
			//���Դ���һ���ֽ������ʱ��Ϊ��    	 104us*10λ  =1040us
			//���� MODBUSȷ��һ������֡��ɵ�ʱ��Ϊ��1040us*3.5=3.64ms ->10ms
            usTimerT35_50us = ( 7UL * 220000UL ) / ( 2UL * ulBaudRate );//���ö�ʱ���ж�ʱ��Ϊ3.5���ַ�ʱ��
        }
        if( xMBPortTimersInit( ( USHORT ) usTimerT35_50us ) != TRUE )	//��ʱ����ʼ��
        {
            eStatus = MB_EPORTERR;		//��ֲ�����	
        }
    }
    EXIT_CRITICAL_SECTION(  );			//��ȫ���ж�

    return eStatus;
}


//�������ܣ�����RTU����
void eMBRTUStart( void )
{
    ENTER_CRITICAL_SECTION(  );				//��ȫ���ж�
	
    //��������շ�����STATE_RX_INIT״̬������������ʱ����
	//�����t3.5��û�н��յ��ַ������ǽ�����ΪSTATE_RX_IDLE��
	//��ȷ�������ӳ�modbusЭ��ջ��������ֱ�������ǿ��еġ�
    eRcvState = STATE_RX_INIT;				//���ý��ջ����ڳ�ʼ״̬
    vMBPortSerialEnable( TRUE, FALSE );		//�����ж�ʹ�ܣ������ж�ʧ��
    vMBPortTimersEnable(  );				//��ʱ��ʹ��
	
    EXIT_CRITICAL_SECTION(  );				//��ȫ���ж�
}

//�������ܣ�ֹͣRTU����
void eMBRTUStop( void )
{
    ENTER_CRITICAL_SECTION(  );				//��ȫ���ж�
    vMBPortSerialEnable( FALSE, FALSE );	//�����ж�ʧ�ܣ������ж�ʧ��
    vMBPortTimersDisable(  );				//��ʱ��ʹ����
    EXIT_CRITICAL_SECTION(  );				//��ȫ���ж�
}

//�������ܣ���ADU����֡�е�PDU����֡��ȡ����
//���� pucRcvAddress�����ͷ��ĵ�ַ
//���� pucFrame�����շ��ͱ������ݻ�����	 
//���� pusLength�����յ����ݳ��ȣ���������ַ��CRCУ���룩
eMBErrorCode eMBRTUReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    BOOL            xFrameReceived = FALSE;
    eMBErrorCode    eStatus = MB_ENOERR;

    ENTER_CRITICAL_SECTION(  );						  //��ȫ���ж�
    __assert( usRcvBufferPos < MB_SER_PDU_SIZE_MAX );   //������յ����ݳ���

	//RTU����֡���Ⱥ�CRCУ��
    if( ( usRcvBufferPos >= MB_SER_PDU_SIZE_MIN ) && ( usMBCRC16( ( UCHAR * ) ucRTUBuf, usRcvBufferPos ) == 0 ) )
    {
		//�õ����ͷ��ĵ�ַ
        *pucRcvAddress = ucRTUBuf[MB_SER_PDU_ADDR_OFF];		

		//�õ��������ݵĳ���
        *pusLength = ( USHORT )( usRcvBufferPos - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC );//PDU����=ADU����-1�ֽ�(��ַ)-2�ֽ�(CRC)

        //ָ��RTU����֡�ײ�
        *pucFrame = ( UCHAR * ) & ucRTUBuf[MB_SER_PDU_PDU_OFF];//PDU����ָ��
        xFrameReceived = TRUE;			
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
eMBErrorCode eMBRTUSend( UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          usCRC16;

    ENTER_CRITICAL_SECTION(  );		//��ȫ���ж�
     
	//���������Ƿ���С�STATE_RX_IDLE״̬��T35��ʱ�ж��б����á������������ڿ���״̬��
    //˵������������ModBus���緢����һ��֡�����Ǳ����жϱ��λ�Ӧ֡�ķ��͡�
    if( eRcvState == STATE_RX_IDLE )		//������ջ����ڽ��տ���״̬
    {
		//��ΪPDU֡�ײ��ǹ����룬��ADU֡�ײ��ǵ�ַ�����Դ�ʱ������֡����һλ���������ַ
        pucSndBufferCur = ( UCHAR * ) pucFrame - 1;		
        usSndBufferCount = 1;

        //��PDU����֡�Ž�ADU����֡
        pucSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;	//����ӻ���ַ
        usSndBufferCount += usLength;

        //����CRC16У���
        usCRC16 = usMBCRC16( ( UCHAR * ) pucSndBufferCur, usSndBufferCount );
        ucRTUBuf[usSndBufferCount++] = ( UCHAR )( usCRC16 & 0xFF );
        ucRTUBuf[usSndBufferCount++] = ( UCHAR )( usCRC16 >> 8 );

        //ʹ��������ڴ���״̬
        eSndState = STATE_TX_XMIT;
			
		xMBPortSerialPutByte((CHAR)*pucSndBufferCur);//��������
		vMBPortSerialEnable( FALSE, TRUE );
		pucSndBufferCur++;
		usSndBufferCount--;	
    }
    else
    {
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION(  );//��ȫ���ж�
    return eStatus;
}

//���մ�������1.STATE_RX_INIT  ��ʼ״̬
//			   2.STATE_RX_IDLE	����״̬
//             3.STATE_RX_RCV	����״̬
//             4.STATE_RX_ERROR	���մ���״̬
//�˺���Ϊ����ָ�룬�ڴ��ڽ����ж��б�����
BOOL xMBRTUReceiveFSM( void )
{
    BOOL            xTaskNeedSwitch = FALSE;
    UCHAR           ucByte;

    __assert( eSndState == STATE_TX_IDLE );				//ȷ����ʱ�����ݷ��ͣ���Ϊmodbus����֧�ֽ��պͷ���ͬʱ����

    ( void )xMBPortSerialGetByte( ( CHAR * ) & ucByte );//�Ӵ������ݼĴ�����ȡһ���ֽ�����

    switch ( eRcvState )
    {		
		case STATE_RX_INIT:				//���ջ����ڳ�ʼ״̬	
			vMBPortTimersEnable(  );	//������ʱ�����ö�ʱ��3.5Tʱ������жϣ���������жϣ�˵���Ѿ��������һ֡����
			break;
		case STATE_RX_ERROR:			//���ջ���������
			vMBPortTimersEnable(  );	//���¿�����ʱ��
			break;
		case STATE_RX_IDLE:				//���ջ����ڿ���״̬��˵����ʱ���Կ�ʼ��������
			usRcvBufferPos = 0;			//���ݳ�������
			ucRTUBuf[usRcvBufferPos++] = ucByte;	//������յ������ݣ�ͬʱ���ݳ��ȼ�1
			eRcvState = STATE_RX_RCV;	//���ջ�״̬����Ϊ����״̬����Ϊ��ʱ�Ѿ���ʼ��������	
			vMBPortTimersEnable(  );	//ÿ�յ�һ���ֽڣ�������ʱ��
			break;
		case STATE_RX_RCV:				//���ջ����ڽ���״̬
			if( usRcvBufferPos < MB_SER_PDU_SIZE_MAX )  //������ճ���С���������֡����
			{
				ucRTUBuf[usRcvBufferPos++] = ucByte;	
			}
			else										//������ճ��ȴ����������֡���ȣ�����ջ�״̬����Ϊ��������		
			{
				eRcvState = STATE_RX_ERROR;
			}
			vMBPortTimersEnable(  );	//ÿ�յ�һ���ֽڣ�������ʱ��
			break;
    }
    return xTaskNeedSwitch;
}

//���ʹ�������1.STATE_TX_IDLE ����״̬����״̬��˵���Ѿ��������һ�����ݣ����Խ����ڽ���ʹ�ܣ�����ʧ��
//			   2.STATE_TX_XMIT ����״̬����״̬�±������ڷ���һ֡���ݣ�һ֡���ݷ�����ɹ��󣬽������״̬ʧ�ܴ��ڷ���
//�˺���Ϊ����ָ�룬�ڴ��ڷ����ж��б�����
BOOL xMBRTUTransmitFSM( void )
{
    BOOL            xNeedPoll = FALSE;

    __assert( eRcvState == STATE_RX_IDLE );		//ȷ����ʱû�н������ݣ���Ϊmodbus����֧�ֽ��պͷ���ͬʱ����

    switch ( eSndState )
    {	
		case STATE_TX_IDLE:							//���ݷ�����ɣ����ͻ��������״̬
			vMBPortSerialEnable( TRUE, FALSE );		//ʹ�ܴ��ڽ��գ�ʧ�ܴ��ڷ���
			break;
		case STATE_TX_XMIT:							//���ͻ����ڷ���״̬
			if( usSndBufferCount != 0 )				//�����Ҫ���͵����ݳ��Ȳ�Ϊ0
			{
				xMBPortSerialPutByte( ( CHAR )*pucSndBufferCur );		//����һ�ֽ�����
				pucSndBufferCur++;  				//׼��������һ���ֽ�����
				usSndBufferCount--;					//�������ݳ��ȼ�1
			}
			else									//һ֡���ݷ������
			{
				xNeedPoll = xMBPortEventPost( EV_FRAME_SENT );
				vMBPortSerialEnable( TRUE, FALSE );	//ʹ�ܴ��ڽ��գ�ʧ�ܴ��ڷ���
				eSndState = STATE_TX_IDLE;			//���ͻ�����Ϊ����״̬��׼����һ�εķ�������
			}
			break;
    }
    return xNeedPoll;
}

//�˺���Ϊ����ָ�룬�ڶ�ʱ���ж��б�����
BOOL xMBRTUTimerT35Expired( void )
{
    BOOL  xNeedPoll = FALSE;
	
    switch ( eRcvState )
    {
		//�����׶����
		//��������ж�ʱ����ʱ���ջ����ڳ�ʼ��״̬
		case STATE_RX_INIT:		//���ջ����ڳ�ʼ��״̬
			xNeedPoll = xMBPortEventPost( EV_READY );
			break;
		//��ʾ�������յ�һ��֡
		//��������ж�ʱ����ʱ���ջ����ڽ���״̬
		case STATE_RX_RCV:		//���ջ����ڽ���״̬
			xNeedPoll = xMBPortEventPost( EV_FRAME_RECEIVED );//����ѯ���������¼�
			break;
		case STATE_RX_ERROR:	//���շ�������
			break;		
		default:		//�����ڷǷ�״̬�µ���
			__assert( ( eRcvState == STATE_RX_INIT ) ||
					( eRcvState == STATE_RX_RCV ) || ( eRcvState == STATE_RX_ERROR ) );
    }

    vMBPortTimersDisable(  );	//�رն�ʱ��
    eRcvState = STATE_RX_IDLE;	//���ջ����ڿ����ж�
    return xNeedPoll;
}

#endif

//һ����ʼ��ModbusЭ��ջ
//	1.��֤�ӻ���ַ�Ƿ����Ҫ��
//	2.ѡ��Modbus����ģʽΪRTU֡
//	3.RTU����ģʽ��ʼ��
//		�ٴ��ڳ�ʼ����������Ϊ9600kps/s��
//		�ڶ�ʱ����ʼ���������ж�ʱ��Ϊ3.5���ַ�ʱ�䣩
//	4.����Э��ʹ��״̬�� eMBState = STATE_DISABLED ��
//��������ModbusЭ��ջ
//	1.����RTU����
//		�����ý��ջ����ڳ�ʼ״̬�� eRcvState = STATE_RX_INIT ��
//		��ʹ�ܴ��ڽ����ж�ʹ�ܣ������ж�ʧ��
//		��ʹ�ܶ�ʱ���жϣ���������
//	2.����Э��ʹ��״̬�� eMBState = STATE_ENABLED ��
//
/* ע;1.�Դ�Э��ջ�Ѿ���ʼ���������ڿ��Կ�ʼ�������� 
 *	  2.�ڴ��ڽ����жϺ����ͷ����жϺ������и��Ե�״̬������
 *	  3.�ڶ�ʱ���ж�����״̬��������
 */

//�����������ݹ���
//	1.��Ϊû�н��յ����ݣ���ʱ����һ�η����ж�
//		��.��ʱ���ջ����ڳ�ʼ״̬�� eRcvState = STATE_RX_INIT ��
//		��.���������¼��� eEvent=EV_READY ��
//		��.�رն�ʱ���� ʧ�ܶ�ʱ���жϣ��������� ��
//		��.���ý��ջ�Ϊ����״̬�� eRcvState = STATE_RX_IDLE ��
//	2.һ��ʱ��󣬴��ڽ��յ�����
//		��.��ʱ���ջ����ڿ���״̬�� eRcvState = STATE_RX_IDLE ��
//		��.��ʱ���Կ�ʼ��������
//		��.���ý��ջ�Ϊ����״̬�� eRcvState = STATE_RX_RCV ��
//		��.����������ʱ����ʹ�ܶ�ʱ�жϣ��������㣩
//		��.�����������ݣ�ÿ����һ�Σ�����һ�ζ�ʱ����ֱ���������ݳ��ȴ�������֡����
//		��.����������ݳ��ȴ�������֡���ȷ�����ʱ���жϣ������ݲ��������򣬷�����ʱ���ж�˵��һ֡���ݽ�����ɣ�����֡��������¼��� eEvent=EV_FRAME_RECEIVED ��

//�ġ��������ݹ���
//	1.�жϽ��ջ��Ƿ��ڽ��տ���״̬
//	2.��PDU����֡�����ADU����֡����ַ+������+�쳣�룩
//	3.�����ͻ�״̬����Ϊ����״̬�� eSndState = STATE_TX_XMIT ��
//	4.ʹ�ܴ��ڷ����жϣ�ʧ�ܽ����ж�
//	5.������ɺ󣬷���֡��������¼��� eEvent=EV_FRAME_SENT ��
//	6.ʹ�ܴ��ڽ��գ�ʧ�ܴ��ڷ���
//	7.���ͻ�����Ϊ����״̬��׼����һ�εķ������ݣ� eSndState = STATE_TX_IDLE �� 
