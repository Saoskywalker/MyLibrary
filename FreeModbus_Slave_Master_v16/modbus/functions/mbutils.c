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
#include "mbproto.h"

/* ----------------------- Defines ------------------------------------------*/
#define BITS_UCHAR      8U

/* ----------------------- Start implementation -----------------------------*/
//函数功能：以ucByteBuf为首地址usBitOffset为偏移量开始的地址写入值
//参数 ucByteBuf：数组首地址
//参数 usBitOffset：bit偏移量
//参数 ucNBits：掩码位数
void xMBUtilSetBits( UCHAR * ucByteBuf, USHORT usBitOffset, UCHAR ucNBits, UCHAR ucValue )
{
    USHORT          usWordBuf;
    USHORT          usMask;
    USHORT          usByteOffset;
    USHORT          usNPreBits;
    USHORT          usValue = ucValue;

    __assert( ucNBits <= 8 );
    __assert( ( size_t )BITS_UCHAR == sizeof( UCHAR ) * 8 );

    usByteOffset = ( USHORT )( ( usBitOffset ) / BITS_UCHAR );			//得到byte偏移量
    usNPreBits = ( USHORT )( usBitOffset - usByteOffset * BITS_UCHAR );	//得到相对byte的bit偏移量
	
    usValue <<= usNPreBits;												//左移bit位

    usMask = ( USHORT )( ( 1 << ( USHORT ) ucNBits ) - 1 );				//得到8位的掩码
    usMask <<= usBitOffset - usByteOffset * BITS_UCHAR;					//左移bit位

    usWordBuf = ucByteBuf[usByteOffset];								//读取第一个字节
    usWordBuf |= ucByteBuf[usByteOffset + 1] << BITS_UCHAR;				//读取第二个字节

    usWordBuf = ( USHORT )( ( usWordBuf & ( ~usMask ) ) | usValue );	//写入值

    ucByteBuf[usByteOffset] = ( UCHAR )( usWordBuf & 0xFF );			//写入低8位
    ucByteBuf[usByteOffset + 1] = ( UCHAR )( usWordBuf >> BITS_UCHAR );	//写入高8位
}

//函数功能：得到以ucByteBuf为首地址usBitOffset为偏移量开始的8位bit位信息
//参数 ucByteBuf：数组首地址
//参数 usBitOffset：bit偏移量
//参数 ucNBits：掩码位数
//返回值：得到以ucByteBuf为首地址usBitOffset为偏移量开始的8位bit位信息
UCHAR xMBUtilGetBits( UCHAR * ucByteBuf, USHORT usBitOffset, UCHAR ucNBits )
{
    USHORT          usWordBuf;
    USHORT          usMask;
    USHORT          usByteOffset;
    USHORT          usNPreBits;

    usByteOffset = ( USHORT )( ( usBitOffset ) / BITS_UCHAR );			//得到byte偏移量
    usNPreBits = ( USHORT )( usBitOffset - usByteOffset * BITS_UCHAR );	//得到相对byte的bit偏移量
    usMask = ( USHORT )( ( 1 << ( USHORT ) ucNBits ) - 1 );				//得到8位的掩码

    usWordBuf = ucByteBuf[usByteOffset];								//读取第一个字节
    usWordBuf |= ucByteBuf[usByteOffset + 1] << BITS_UCHAR;				//读取第二个字节
    usWordBuf >>= usNPreBits;											//向右移bit位
    usWordBuf &= usMask;

    return ( UCHAR ) usWordBuf;
}

eMBException prveMBError2Exception( eMBErrorCode eErrorCode )
{
    eMBException    eStatus;

    switch ( eErrorCode )
    {
        case MB_ENOERR:
            eStatus = MB_EX_NONE;
            break;

        case MB_ENOREG:
            eStatus = MB_EX_ILLEGAL_DATA_ADDRESS;
            break;

        case MB_ETIMEDOUT:
            eStatus = MB_EX_SLAVE_BUSY;
            break;

        default:
            eStatus = MB_EX_SLAVE_DEVICE_FAILURE;
            break;
    }

    return eStatus;
}
