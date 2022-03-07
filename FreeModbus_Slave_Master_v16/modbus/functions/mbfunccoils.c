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
#include "mbframe.h"
#include "mbproto.h"
#include "mbconfig.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_PDU_FUNC_READ_ADDR_OFF           ( MB_PDU_DATA_OFF )		//��Ȧ�Ĵ�����ַƫ����
#define MB_PDU_FUNC_READ_COILCNT_OFF        ( MB_PDU_DATA_OFF + 2 )	//��Ȧ����ƫ����
#define MB_PDU_FUNC_READ_SIZE               ( 4 )					//����Ȧ�Ĵ���PDU����֡��С�������������룩
#define MB_PDU_FUNC_READ_COILCNT_MAX        ( 0x07D0 )

#define MB_PDU_FUNC_WRITE_ADDR_OFF          ( MB_PDU_DATA_OFF )		//��Ȧ�Ĵ�����ַƫ����
#define MB_PDU_FUNC_WRITE_VALUE_OFF         ( MB_PDU_DATA_OFF + 2 )	//��Ȧ����ƫ����
#define MB_PDU_FUNC_WRITE_SIZE              ( 4 )					//д��Ȧ�Ĵ���PDU����֡��С�������������룩

#define MB_PDU_FUNC_WRITE_MUL_ADDR_OFF      ( MB_PDU_DATA_OFF )
#define MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF   ( MB_PDU_DATA_OFF + 2 )
#define MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF   ( MB_PDU_DATA_OFF + 4 )
#define MB_PDU_FUNC_WRITE_MUL_VALUES_OFF    ( MB_PDU_DATA_OFF + 5 )
#define MB_PDU_FUNC_WRITE_MUL_SIZE_MIN      ( 5 )
#define MB_PDU_FUNC_WRITE_MUL_COILCNT_MAX   ( 0x07B0 )

/* ----------------------- Static functions ---------------------------------*/
eMBException    prveMBError2Exception( eMBErrorCode eErrorCode );
extern char Start_Flag;
/* ----------------------- Start implementation -----------------------------*/

#if MB_FUNC_READ_COILS_ENABLED > 0

//��������;��һ��������Ȧ�Ĵ���
//���� pucFrame�����յ�PDU����֡�� ������+�Ĵ�����ʼ��ַ��2���ֽڣ�+��ȡ��Ȧ������2���ֽڣ���
//���� usLen�����յ�PDU����֡�ĳ���
//
//����Ȧ�Ĵ���֡��ʽ�����ӻ���ַ+������+��ʼ��ַ��λ+��ʼ��ַ��λ+��Ȧ������λ+��Ȧ������λ+CRC��λ+CRC��λ��
//��Ӧ֡��ʽ��(�ӻ���ַ+������+�����ֽ���+����1+����2+.....����n+CRC��λ+CRC��λ)
//������01H��ȡModbus�ӻ�����Ȧ�Ĵ�����״̬�������ǵ����Ĵ��������߶�������ļĴ�����
//ע������Ȧ��״̬���������ݵ�ÿ��bit��Ӧ��1����ON��0����OFF�������ѯ����Ȧ��������8�ı������������һ���ֽڵĸ�λ��0
//
//����Ȧ�Ĵ����ĸ��������˵����
//������Ҫ���ף���Ȧ�Ĵ���������ָ���ٸ�bitλ����Ҳ��Ϊʲô����Ὣ�Ĵ�����������8��
//�мǣ������һ���Ĵ���ָ����1��bitλ��������һ���ֽڣ�Ҳ����˵��1����Ȧָ1��bitλ
//Ҳ����˵����Ȧʵ����;����д�Ĵ����Ͷ��Ĵ�����
//�ٴӱ������������Ƕ�IO�ڵ�������ݼĴ������������ݼĴ���
eMBException eMBFuncReadCoils( UCHAR * pucFrame, USHORT * usLen )
{
    USHORT          usRegAddress;   //��ȡ����Ȧ�Ĵ�������ʼ��ַ
    USHORT          usCoilCount;	//��ȡ����Ȧ�ĸ���	
    UCHAR           ucNBytes;		
    UCHAR          *pucFrameCur;

    eMBException    eStatus = MB_EX_NONE;  	//�쳣ö�ٱ�������Ϊ�޴���
    eMBErrorCode    eRegStatus;				//����ö�ٱ���

    if( *usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN ) )		//���PDU����֡���ڣ������루1���ֽڣ�+�Ĵ�����ַ��2���ֽڣ�+����Ȧ������2���ֽڣ���
    {
        usRegAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_READ_ADDR_OFF] << 8 );   //�õ���ȡ����Ȧ�Ĵ����ĸ�λ��ʼ��ַ
        usRegAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_READ_ADDR_OFF + 1] );	 //�õ���ȡ����Ȧ�Ĵ����ĵ�λ��ʼ��ַ
        usRegAddress++;

        usCoilCount = ( USHORT )( pucFrame[MB_PDU_FUNC_READ_COILCNT_OFF] << 8 ); //�õ���ȡ����Ȧ�����ĸ�λ
        usCoilCount |= ( USHORT )( pucFrame[MB_PDU_FUNC_READ_COILCNT_OFF + 1] ); //�õ���ȡ����Ȧ�����ĵ�λ
        if( ( usCoilCount >= 1 ) &&( usCoilCount < MB_PDU_FUNC_READ_COILCNT_MAX ) ) //���Ҫ��ȡ�ļĴ�����Ŀ�Ƿ���Ч
        {
            pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];   	//����ǰPDU����ָ��ָ��PDU����֡��ʼ
            *usLen = MB_PDU_FUNC_OFF;  						//��ǰ֡����Ϊ0

            *pucFrameCur++ = MB_FUNC_READ_COILS;			//��һ���ֽڴ湦����
            *usLen += 1;									//֡���ȼ�1

            if( ( usCoilCount & 0x0007 ) != 0 )  			//�жϼĴ�����Ŀ�Ƿ�Ϊ8�ı���
            {
                ucNBytes = ( UCHAR )( usCoilCount / 8 + 1 );
            }
            else
            {
                ucNBytes = ( UCHAR )( usCoilCount / 8 );
            }
            *pucFrameCur++ = ucNBytes;						//�ڶ����ֽڴ淵���ֽ���
            *usLen += 1;									//֡���ȼ�1

			//��ȡ��usRegAddressΪ��ʼ��ַ���Ժ��usCoilCount��bitλ��Ϣ
            eRegStatus = eMBRegCoilsCB( pucFrameCur, usRegAddress, usCoilCount,MB_REG_READ );	

            
            if( eRegStatus != MB_ENOERR )						//�����ȡ��Ϣ��������
            {
                eStatus = prveMBError2Exception( eRegStatus );	//�ж��Ƿ��������ִ���
            }
            else
            {
                *usLen += ucNBytes;;							//֡���ȼ�ucNBytes
            }
        }
        else
        {
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;					//�Ƿ�����ֵ
        }
    }
    else
    {
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;						//�Ƿ�����ֵ
    }
    return eStatus;
}

#if MB_FUNC_WRITE_COIL_ENABLED > 0
//��������;д������Ȧ�Ĵ���
//���� pucFrame: ���յ�PDU����֡�� ������+�Ĵ�����ʼ��ַ��2���ֽڣ�+���ݣ�2���ֽڣ��� 
//���� usLen:    ���յ�PDU����֡�ĳ���

//д������Ȧ�Ĵ���֡��ʽ:���ӻ���ַ+������+�Ĵ�����ַ��λ+�Ĵ�����ַ��λ+���ݸ�λ+���ݵ�λ+CRC��λ+CRC��λ��
//��Ӧ֡��ʽ�����д��ɹ������ط��͵�ָ��
//������05Hд������Ȧ�Ĵ�����FF00H������Ȧ����ON״̬��0000H������Ȧ����OFF״̬��
eMBException eMBFuncWriteCoil( UCHAR * pucFrame, USHORT * usLen )
{
    USHORT          usRegAddress;
    UCHAR           ucBuf[2];

    eMBException    eStatus = MB_EX_NONE;	//�쳣ö�ٱ�������Ϊ�޴���
    eMBErrorCode    eRegStatus;				//����ö�ٱ���

    if( *usLen == ( MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN ) )	//���PDU����֡���ڣ������루1���ֽڣ�+�Ĵ�����ַ��2���ֽڣ�+д��Ȧ���ݣ�2���ֽڣ���
    {
        usRegAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8 );	//�õ�д��Ȧ�Ĵ����ĸ�λ��ʼ��ַ
        usRegAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1] );	//�õ�д��Ȧ�Ĵ����ĵ�λ��ʼ��ַ
        usRegAddress++;

        if( ( pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF + 1] == 0x00 ) &&
            ( ( pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF ) ||
              ( pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0x00 ) ) )		//�ж�PDU����֡�е�3λ�͵�4λΪ0x0000H����0xFF00H
        {
            ucBuf[1] = 0;
            if( pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF )			//�����3λΪ0xFFH������1
            {
                ucBuf[0] = 1;
            }
            else														//�����3λΪ0x00H������0
            {
                ucBuf[0] = 0;
            }
            eRegStatus = eMBRegCoilsCB( &ucBuf[0], usRegAddress, 1, MB_REG_WRITE );
			Start_Flag=ucBuf[0];
            if( eRegStatus != MB_ENOERR )						//���д���ݷ�������
            {
                eStatus = prveMBError2Exception( eRegStatus );	//�ж��Ƿ��������ִ���
            }
        }
        else
        {
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;					//�Ƿ�����ֵ
        }
    }
    else
    {
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;						//�Ƿ�����ֵ
    }
    return eStatus;
}

#endif

#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
//�������ܣ�д�����Ȧ�Ĵ���
//���� pucFrame: ���յ�PDU����֡�� ������+�Ĵ�����ʼ��ַ��2���ֽڣ�+д��Ȧ������2���ֽڣ�+�ֽ���+����1+����2...+����n�� 
//���� usLen:    ���յ�PDU����֡�ĳ���

//д������Ȧ�Ĵ���ָ��:���ӻ���ַ+������+�Ĵ�����ַ��λ+�Ĵ�����ַ��λ+������λ+������λ+�ֽ���+����1+����2...+����n+CRC��λ+CRC��λ��
//��Ӧ����֡��ʽ:���ӻ���ַ+������+�Ĵ�����ַ��λ+�Ĵ�����ַ��λ+������λ+������λ+CRC��λ+CRC��λ��
//������0FHд�����Ȧ�Ĵ����������Ӧ������λΪ1����ʾ��Ȧ״̬ΪON�������Ӧ������λΪ0����ʾ��Ȧ״̬ΪOFF��
//��Ȧ�Ĵ���֮�䣬�͵�ַ�Ĵ����ȴ��䣬�ߵ�ַ�Ĵ������䡣������Ȧ�Ĵ��������ֽ������ȴ��䣬���ֽ����ݺ��䡣
//���д�����Ȧ�Ĵ����ĸ�������8�ı������������һ���ֽڵĸ�λ��0��
eMBException eMBFuncWriteMultipleCoils( UCHAR * pucFrame, USHORT * usLen )
{
    USHORT          usRegAddress;
    USHORT          usCoilCnt;
    UCHAR           ucByteCount;
    UCHAR           ucByteCountVerify;

    eMBException    eStatus = MB_EX_NONE;
    eMBErrorCode    eRegStatus;

    if( *usLen > ( MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN ) )		//���PDU����֡���ڣ������루1���ֽڣ�+�Ĵ�����ַ��2���ֽڣ�+д��Ȧ������2���ֽڣ���
    {
        usRegAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8 );	//�õ�д��Ȧ�Ĵ����ĸ�λ��ʼ��ַ
        usRegAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1] );	//�õ�д��Ȧ�Ĵ����ĵ�λ��ʼ��ַ
        usRegAddress++;

        usCoilCnt = ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF] << 8 );	//�õ�д��Ȧ�����ĸ�λ��ַ
        usCoilCnt |= ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF + 1] );	//�õ�д��Ȧ�����ĵ�λ��ַ

        ucByteCount = pucFrame[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF];		

        
        if( ( usCoilCnt & 0x0007 ) != 0 )							//�жϼĴ�����Ŀ�Ƿ�Ϊ8�ı���
        {
            ucByteCountVerify = ( UCHAR )( usCoilCnt / 8 + 1 );
        }
        else
        {
            ucByteCountVerify = ( UCHAR )( usCoilCnt / 8 );
        }

        if( ( usCoilCnt >= 1 ) && ( usCoilCnt <= MB_PDU_FUNC_WRITE_MUL_COILCNT_MAX ) && ( ucByteCountVerify == ucByteCount ) )
        {
            eRegStatus = eMBRegCoilsCB( &pucFrame[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF], usRegAddress, usCoilCnt, MB_REG_WRITE );
         
            if( eRegStatus != MB_ENOERR )							//���д���ݷ�������
            {
                eStatus = prveMBError2Exception( eRegStatus );		//�ж��Ƿ��������ִ���
            }
            else
            {
                *usLen = MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF;			//������ӦPDU����֡�ĳ���Ϊ5�������루1���ֽڣ�+�Ĵ�����ַ��2���ֽڣ�+������2���ֽڣ���
            }
        }
        else
        {
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;						//�Ƿ�����ֵ
        }
    }
    else
    {
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;							//�Ƿ�����ֵ
    }
    return eStatus;
}

#endif

#endif
