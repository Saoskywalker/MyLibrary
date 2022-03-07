/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006 Christian Walter <wolti@sil.at>
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
 * File: $Id: mbfuncholding.c,v 1.12 2007/02/18 23:48:22 wolti Exp $
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
#define MB_PDU_FUNC_READ_ADDR_OFF               ( MB_PDU_DATA_OFF + 0)		//������֡��ʽ�мĴ�����ַ��ƫ����
#define MB_PDU_FUNC_READ_REGCNT_OFF             ( MB_PDU_DATA_OFF + 2 )		//������֡��ʽ�ж��Ĵ���������ƫ����
#define MB_PDU_FUNC_READ_SIZE                   ( 4 )						//������֡��ʽ���ֽڳ���
#define MB_PDU_FUNC_READ_REGCNT_MAX             ( 0x007D )					//���Ĵ������������

#define MB_PDU_FUNC_WRITE_ADDR_OFF              ( MB_PDU_DATA_OFF + 0)		//д����֡��ʽ�мĴ�����ַ��ƫ����
#define MB_PDU_FUNC_WRITE_VALUE_OFF             ( MB_PDU_DATA_OFF + 2 )		//д����֡��ʽ�����ݵ�ƫ����
#define MB_PDU_FUNC_WRITE_SIZE                  ( 4 )						//д����֡��ʽ���ֽڳ���

#define MB_PDU_FUNC_WRITE_MUL_ADDR_OFF          ( MB_PDU_DATA_OFF + 0 )		//д����֡��ʽ�мĴ�����ַ��ƫ����
#define MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF        ( MB_PDU_DATA_OFF + 2 )		//д����֡��ʽ��д�Ĵ���������ƫ����
#define MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF       ( MB_PDU_DATA_OFF + 4 )		//д����֡��ʽ���ֽ�����ƫ����
#define MB_PDU_FUNC_WRITE_MUL_VALUES_OFF        ( MB_PDU_DATA_OFF + 5 )		//д����֡��ʽ�����ݵ�ƫ����	
#define MB_PDU_FUNC_WRITE_MUL_SIZE_MIN          ( 5 )						//д����֡����С����
#define MB_PDU_FUNC_WRITE_MUL_REGCNT_MAX        ( 0x0078 )					//д�Ĵ������������

#define MB_PDU_FUNC_READWRITE_READ_ADDR_OFF     ( MB_PDU_DATA_OFF + 0 )		
#define MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF   ( MB_PDU_DATA_OFF + 2 )
#define MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF    ( MB_PDU_DATA_OFF + 4 )
#define MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF  ( MB_PDU_DATA_OFF + 6 )
#define MB_PDU_FUNC_READWRITE_BYTECNT_OFF       ( MB_PDU_DATA_OFF + 8 )
#define MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF  ( MB_PDU_DATA_OFF + 9 )
#define MB_PDU_FUNC_READWRITE_SIZE_MIN          ( 9 )

/* ----------------------- Static functions ---------------------------------*/
eMBException    prveMBError2Exception( eMBErrorCode eErrorCode );
extern USHORT   usSRegHoldBuf[100]; 
/* ----------------------- Start implementation -----------------------------*/

#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
//��������: 		 д�������ּĴ���
//���� pucFrame: ���յ�PDU����֡�� ������+�Ĵ�����ʼ��ַ��2���ֽڣ�+���ݣ�2���ֽڣ��� 
//���� usLen:	 ���յ�PDU����֡�ĳ���

//д�������ּĴ���֡��ʽ:���ӻ���ַ+������+�Ĵ�����ַ��λ+�Ĵ�����ַ��λ+���ݸ�λ+���ݵ�λ+CRC��λ+CRC��λ��
//��Ӧ֡��ʽ�����д��ɹ������ط��͵�ָ��
//������06Hд�������ּĴ���

//д���ּĴ����ĸ��������˵����
//д���ּĴ���ʵ�ʾ���һ�ζ�2��byte������
eMBException eMBFuncWriteHoldingRegister( UCHAR * pucFrame, USHORT * usLen )
{
    USHORT          usRegAddress;
    eMBException    eStatus = MB_EX_NONE;
    eMBErrorCode    eRegStatus;

    if( *usLen == ( MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN ) )	//���PDU����֡���ڣ������루1���ֽڣ�+�Ĵ�����ַ��2���ֽڣ�+д���ּĴ������ݣ�2���ֽڣ���
    {
        usRegAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8 );
        usRegAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1] );
		usRegAddress++;

        eRegStatus = eMBRegHoldingCB( &pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF], usRegAddress, 1, MB_REG_WRITE );

        if( eRegStatus != MB_ENOERR )				//д�����쳣
        {
            eStatus = prveMBError2Exception( eRegStatus );
        }
    }
    else											//���յ�����֡�����쳣
    {
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    return eStatus;
}
#endif

#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
//�������ܣ�		 д������ּĴ���
//���� pucFrame: ���յ�PDU����֡�� ������+�Ĵ�����ʼ��ַ��2���ֽڣ�+д���ּĴ���������2���ֽڣ�+�ֽ���+����1+����2...+����n�� 
//���� usLen:    ���յ�PDU����֡�ĳ���

//д������Ȧ�Ĵ���ָ��:���ӻ���ַ+������+�Ĵ�����ַ��λ+�Ĵ�����ַ��λ+������λ+������λ+�ֽ���+����1+����2...+����n+CRC��λ+CRC��λ��
//��Ӧ����֡��ʽ:���ӻ���ַ+������+�Ĵ�����ַ��λ+�Ĵ�����ַ��λ+������λ+������λ+CRC��λ+CRC��λ��
//������10Hд������ּĴ���������ÿ�����ּĴ����ĳ���Ϊ�����ֽ�
eMBException eMBFuncWriteMultipleHoldingRegister( UCHAR * pucFrame, USHORT * usLen )
{
    USHORT          usRegAddress;
    USHORT          usRegCount;
    UCHAR           ucRegByteCount;

    eMBException    eStatus = MB_EX_NONE;
    eMBErrorCode    eRegStatus;

	// char buf[4];
    if( *usLen >= ( MB_PDU_FUNC_WRITE_MUL_SIZE_MIN + MB_PDU_SIZE_MIN ))//&& WDCK_Flag==1 )
    {
        usRegAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8 );
        usRegAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1] );
		usRegAddress++;

        usRegCount = ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF] << 8 );
        usRegCount |= ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF + 1] );

        ucRegByteCount = pucFrame[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF];

        if( ( usRegCount >= 1 ) && ( usRegCount <= MB_PDU_FUNC_WRITE_MUL_REGCNT_MAX ) && ( ucRegByteCount == ( UCHAR ) ( 2 * usRegCount ) ) )
        {
            eRegStatus = eMBRegHoldingCB( &pucFrame[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF], usRegAddress, usRegCount, MB_REG_WRITE );

			// sprintf(buf,"%d",usSRegHoldBuf[0]);
			// EDIT_SetText(Up_Value_Edit,buf);			
			// sprintf(buf,"%d",usSRegHoldBuf[1]);
			// EDIT_SetText(Dn_Value_Edit,buf);				
			// sprintf(buf,"%d",usSRegHoldBuf[2]);
			// EDIT_SetText(Value_Edit,buf);
			
			// up_value=usSRegHoldBuf[0];
			// dn_value=usSRegHoldBuf[1];
			// set_value=usSRegHoldBuf[2];
			// PID_SW_SetPoint(set_value);	
		
            if( eRegStatus != MB_ENOERR )
            {
                eStatus = prveMBError2Exception( eRegStatus );
            }
            else
            {
                *usLen = MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF;
            }
        }
        else
        {
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else
    {
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    return eStatus;
}
#endif

#if MB_FUNC_READ_HOLDING_ENABLED > 0

//��������: 		 �����ּĴ���
//���� pucFrame: ���յ�PDU����֡�� ������+�Ĵ�����ʼ��ַ��2���ֽڣ�+���ݣ�2���ֽڣ��� 
//���� usLen:	 ���յ�PDU����֡�ĳ���

//����Ȧ�Ĵ���֡��ʽ�����ӻ���ַ+������+��ʼ��ַ��λ+��ʼ��ַ��λ+��Ȧ������λ+��Ȧ������λ+CRC��λ+CRC��λ��
//��Ӧ֡��ʽ��(�ӻ���ַ+������+�����ֽ���+���ֽ�����1+���ֽ�����1...+���ֽ�����n+���ֽ�����n+CRC��λ+CRC��λ)
//������03H��ȡModbus�ӻ��б��ּĴ��������ݣ������ǵ����Ĵ��������߶�������ļĴ���
//ÿ�����ּĴ����ĳ���Ϊ2���ֽڡ����ּĴ���֮�䣬�͵�ַ�Ĵ����ȴ��䣬�ߵ�ַ�Ĵ������䡣�������ּĴ��������ֽ������ȴ��䣬���ֽ����ݺ��䡣
eMBException eMBFuncReadHoldingRegister( UCHAR * pucFrame, USHORT * usLen )
{
    USHORT          usRegAddress;
    USHORT          usRegCount;
    UCHAR          *pucFrameCur;

    eMBException    eStatus = MB_EX_NONE;
    eMBErrorCode    eRegStatus;

    if( *usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN ) )
    {
        usRegAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_READ_ADDR_OFF] << 8 );
        usRegAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_READ_ADDR_OFF + 1] );
		usRegAddress++;

        usRegCount = ( USHORT )( pucFrame[MB_PDU_FUNC_READ_REGCNT_OFF] << 8 );
        usRegCount = ( USHORT )( pucFrame[MB_PDU_FUNC_READ_REGCNT_OFF + 1] );

        if( ( usRegCount >= 1 ) && ( usRegCount <= MB_PDU_FUNC_READ_REGCNT_MAX ) )
        {
            pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
            *usLen = MB_PDU_FUNC_OFF;

            *pucFrameCur++ = MB_FUNC_READ_HOLDING_REGISTER;
            *usLen += 1;
			
            *pucFrameCur++ = ( UCHAR ) ( usRegCount * 2 );
            *usLen += 1;

            eRegStatus = eMBRegHoldingCB( pucFrameCur, usRegAddress, usRegCount, MB_REG_READ );
            if( eRegStatus != MB_ENOERR )
            {
                eStatus = prveMBError2Exception( eRegStatus );
            }
            else
            {
                *usLen += usRegCount * 2;
            }
        }
        else
        {
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else
    {
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    return eStatus;
}

#endif

#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0

eMBException eMBFuncReadWriteMultipleHoldingRegister( UCHAR * pucFrame, USHORT * usLen )
{
    USHORT          usRegReadAddress;
    USHORT          usRegReadCount;
    USHORT          usRegWriteAddress;
    USHORT          usRegWriteCount;
    UCHAR           ucRegWriteByteCount;
    UCHAR          *pucFrameCur;

    eMBException    eStatus = MB_EX_NONE;
    eMBErrorCode    eRegStatus;

    if( *usLen >= ( MB_PDU_FUNC_READWRITE_SIZE_MIN + MB_PDU_SIZE_MIN ) )
    {
        usRegReadAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_READWRITE_READ_ADDR_OFF] << 8U );
        usRegReadAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_READWRITE_READ_ADDR_OFF + 1] );
//        usRegReadAddress++;

        usRegReadCount = ( USHORT )( pucFrame[MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF] << 8U );
        usRegReadCount |= ( USHORT )( pucFrame[MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF + 1] );

        usRegWriteAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF] << 8U );
        usRegWriteAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF + 1] );
//        usRegWriteAddress++;

        usRegWriteCount = ( USHORT )( pucFrame[MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF] << 8U );
        usRegWriteCount |= ( USHORT )( pucFrame[MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF + 1] );

        ucRegWriteByteCount = pucFrame[MB_PDU_FUNC_READWRITE_BYTECNT_OFF];

        if( ( usRegReadCount >= 1 ) && ( usRegReadCount <= 0x7D ) &&
            ( usRegWriteCount >= 1 ) && ( usRegWriteCount <= 0x79 ) &&
            ( ( 2 * usRegWriteCount ) == ucRegWriteByteCount ) )
        {
            /* Make callback to update the register values. */
            eRegStatus = eMBRegHoldingCB( &pucFrame[MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF],
                                          usRegWriteAddress, usRegWriteCount, MB_REG_WRITE );

            if( eRegStatus == MB_ENOERR )
            {
                /* Set the current PDU data pointer to the beginning. */
                pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
                *usLen = MB_PDU_FUNC_OFF;

                /* First byte contains the function code. */
                *pucFrameCur++ = MB_FUNC_READWRITE_MULTIPLE_REGISTERS;
                *usLen += 1;

                /* Second byte in the response contain the number of bytes. */
                *pucFrameCur++ = ( UCHAR ) ( usRegReadCount * 2 );
                *usLen += 1;

                /* Make the read callback. */
                eRegStatus =
                    eMBRegHoldingCB( pucFrameCur, usRegReadAddress, usRegReadCount, MB_REG_READ );
                if( eRegStatus == MB_ENOERR )
                {
                    *usLen += 2 * usRegReadCount;
                }
            }
            if( eRegStatus != MB_ENOERR )
            {
                eStatus = prveMBError2Exception( eRegStatus );
            }
        }
        else
        {
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    return eStatus;
}

#endif
