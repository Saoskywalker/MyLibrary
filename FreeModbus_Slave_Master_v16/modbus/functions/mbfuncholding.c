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
#define MB_PDU_FUNC_READ_ADDR_OFF               ( MB_PDU_DATA_OFF + 0)		//读数据帧格式中寄存器地址的偏移量
#define MB_PDU_FUNC_READ_REGCNT_OFF             ( MB_PDU_DATA_OFF + 2 )		//读数据帧格式中读寄存器数量的偏移量
#define MB_PDU_FUNC_READ_SIZE                   ( 4 )						//读数据帧格式的字节长度
#define MB_PDU_FUNC_READ_REGCNT_MAX             ( 0x007D )					//读寄存器的最大数量

#define MB_PDU_FUNC_WRITE_ADDR_OFF              ( MB_PDU_DATA_OFF + 0)		//写数据帧格式中寄存器地址的偏移量
#define MB_PDU_FUNC_WRITE_VALUE_OFF             ( MB_PDU_DATA_OFF + 2 )		//写数据帧格式中数据的偏移量
#define MB_PDU_FUNC_WRITE_SIZE                  ( 4 )						//写数据帧格式的字节长度

#define MB_PDU_FUNC_WRITE_MUL_ADDR_OFF          ( MB_PDU_DATA_OFF + 0 )		//写数据帧格式中寄存器地址的偏移量
#define MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF        ( MB_PDU_DATA_OFF + 2 )		//写数据帧格式中写寄存器数量的偏移量
#define MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF       ( MB_PDU_DATA_OFF + 4 )		//写数据帧格式中字节数的偏移量
#define MB_PDU_FUNC_WRITE_MUL_VALUES_OFF        ( MB_PDU_DATA_OFF + 5 )		//写数据帧格式中数据的偏移量	
#define MB_PDU_FUNC_WRITE_MUL_SIZE_MIN          ( 5 )						//写数据帧的最小长度
#define MB_PDU_FUNC_WRITE_MUL_REGCNT_MAX        ( 0x0078 )					//写寄存器的最大数量

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
//函数功能: 		 写单个保持寄存器
//参数 pucFrame: 接收的PDU数据帧（ 功能码+寄存器起始地址（2个字节）+数据（2个字节）） 
//参数 usLen:	 接收的PDU数据帧的长度

//写单个保持寄存器帧格式:（从机地址+功能码+寄存器地址高位+寄存器地址低位+数据高位+数据低位+CRC高位+CRC低位）
//响应帧格式：如果写入成功，返回发送的指令
//功能码06H写单个保持寄存器

//写保持寄存器的概念和作用说明：
//写保持寄存器实际就是一次读2个byte的数据
eMBException eMBFuncWriteHoldingRegister( UCHAR * pucFrame, USHORT * usLen )
{
    USHORT          usRegAddress;
    eMBException    eStatus = MB_EX_NONE;
    eMBErrorCode    eRegStatus;

    if( *usLen == ( MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN ) )	//如果PDU数据帧等于（功能码（1个字节）+寄存器地址（2个字节）+写保持寄存器数据（2个字节））
    {
        usRegAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8 );
        usRegAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1] );
		usRegAddress++;

        eRegStatus = eMBRegHoldingCB( &pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF], usRegAddress, 1, MB_REG_WRITE );

        if( eRegStatus != MB_ENOERR )				//写数据异常
        {
            eStatus = prveMBError2Exception( eRegStatus );
        }
    }
    else											//接收的数据帧长度异常
    {
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    return eStatus;
}
#endif

#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
//函数功能：		 写多个保持寄存器
//参数 pucFrame: 接收的PDU数据帧（ 功能码+寄存器起始地址（2个字节）+写保持寄存器数量（2个字节）+字节数+数据1+数据2...+数据n） 
//参数 usLen:    接收的PDU数据帧的长度

//写入多个线圈寄存器指令:（从机地址+功能码+寄存器地址高位+寄存器地址低位+数量高位+数量低位+字节数+数据1+数据2...+数据n+CRC高位+CRC低位）
//响应数据帧格式:（从机地址+功能码+寄存器地址高位+寄存器地址低位+数量高位+数量低位+CRC高位+CRC低位）
//功能码10H写多个保持寄存器，其中每个保持寄存器的长度为两个字节
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

//函数功能: 		 读保持寄存器
//参数 pucFrame: 接收的PDU数据帧（ 功能码+寄存器起始地址（2个字节）+数据（2个字节）） 
//参数 usLen:	 接收的PDU数据帧的长度

//读线圈寄存器帧格式：（从机地址+功能码+起始地址高位+起始地址低位+线圈数量高位+线圈数量低位+CRC高位+CRC低位）
//响应帧格式：(从机地址+功能码+返回字节数+高字节数据1+低字节数据1...+高字节数据n+低字节数据n+CRC高位+CRC低位)
//功能码03H读取Modbus从机中保持寄存器的数据，可以是单个寄存器，或者多个连续的寄存器
//每个保持寄存器的长度为2个字节。保持寄存器之间，低地址寄存器先传输，高地址寄存器后传输。单个保持寄存器，高字节数据先传输，低字节数据后传输。
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
