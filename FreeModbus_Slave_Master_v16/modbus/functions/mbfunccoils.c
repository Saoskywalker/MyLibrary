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
#define MB_PDU_FUNC_READ_ADDR_OFF           ( MB_PDU_DATA_OFF )		//线圈寄存器地址偏移量
#define MB_PDU_FUNC_READ_COILCNT_OFF        ( MB_PDU_DATA_OFF + 2 )	//线圈数量偏移量
#define MB_PDU_FUNC_READ_SIZE               ( 4 )					//读线圈寄存器PDU数据帧大小（不包括功能码）
#define MB_PDU_FUNC_READ_COILCNT_MAX        ( 0x07D0 )

#define MB_PDU_FUNC_WRITE_ADDR_OFF          ( MB_PDU_DATA_OFF )		//线圈寄存器地址偏移量
#define MB_PDU_FUNC_WRITE_VALUE_OFF         ( MB_PDU_DATA_OFF + 2 )	//线圈数量偏移量
#define MB_PDU_FUNC_WRITE_SIZE              ( 4 )					//写线圈寄存器PDU数据帧大小（不包括功能码）

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

//函数功能;读一个或多个线圈寄存器
//参数 pucFrame：接收的PDU数据帧（ 功能码+寄存器起始地址（2个字节）+读取线圈数量（2个字节））
//参数 usLen：接收的PDU数据帧的长度
//
//读线圈寄存器帧格式：（从机地址+功能码+起始地址高位+起始地址低位+线圈数量高位+线圈数量低位+CRC高位+CRC低位）
//响应帧格式：(从机地址+功能码+返回字节数+数据1+数据2+.....数据n+CRC高位+CRC低位)
//功能码01H读取Modbus从机中线圈寄存器的状态，可以是单个寄存器，或者多个连续的寄存器。
//注：各线圈的状态与数据内容的每个bit对应，1代表ON，0代表OFF。如果查询的线圈数量不是8的倍数，则在最后一个字节的高位补0
//
//读线圈寄存器的概念和作用说明：
//首先需要明白，线圈寄存器个数是指多少个bit位，这也是为什么后面会将寄存器个数除以8。
//切记！这里的一个寄存器指的是1个bit位，而不是一个字节，也就是说，1个线圈指1个bit位
//也就是说读线圈实际用途就是写寄存器和读寄存器。
//再从本质来讲，就是读IO口的输出数据寄存器和输入数据寄存器
eMBException eMBFuncReadCoils( UCHAR * pucFrame, USHORT * usLen )
{
    USHORT          usRegAddress;   //读取的线圈寄存器的起始地址
    USHORT          usCoilCount;	//读取的线圈的个数	
    UCHAR           ucNBytes;		
    UCHAR          *pucFrameCur;

    eMBException    eStatus = MB_EX_NONE;  	//异常枚举变量设置为无错误
    eMBErrorCode    eRegStatus;				//错误枚举变量

    if( *usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN ) )		//如果PDU数据帧等于（功能码（1个字节）+寄存器地址（2个字节）+读线圈数量（2个字节））
    {
        usRegAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_READ_ADDR_OFF] << 8 );   //得到读取的线圈寄存器的高位起始地址
        usRegAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_READ_ADDR_OFF + 1] );	 //得到读取的线圈寄存器的低位起始地址
        usRegAddress++;

        usCoilCount = ( USHORT )( pucFrame[MB_PDU_FUNC_READ_COILCNT_OFF] << 8 ); //得到读取的线圈个数的高位
        usCoilCount |= ( USHORT )( pucFrame[MB_PDU_FUNC_READ_COILCNT_OFF + 1] ); //得到读取的线圈个数的低位
        if( ( usCoilCount >= 1 ) &&( usCoilCount < MB_PDU_FUNC_READ_COILCNT_MAX ) ) //检查要读取的寄存器数目是否有效
        {
            pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];   	//将当前PDU数据指针指向PDU数据帧开始
            *usLen = MB_PDU_FUNC_OFF;  						//当前帧长度为0

            *pucFrameCur++ = MB_FUNC_READ_COILS;			//第一个字节存功能码
            *usLen += 1;									//帧长度加1

            if( ( usCoilCount & 0x0007 ) != 0 )  			//判断寄存器数目是否为8的倍数
            {
                ucNBytes = ( UCHAR )( usCoilCount / 8 + 1 );
            }
            else
            {
                ucNBytes = ( UCHAR )( usCoilCount / 8 );
            }
            *pucFrameCur++ = ucNBytes;						//第二个字节存返回字节数
            *usLen += 1;									//帧长度加1

			//读取以usRegAddress为起始地址的以后的usCoilCount个bit位信息
            eRegStatus = eMBRegCoilsCB( pucFrameCur, usRegAddress, usCoilCount,MB_REG_READ );	

            
            if( eRegStatus != MB_ENOERR )						//如果读取信息发生错误
            {
                eStatus = prveMBError2Exception( eRegStatus );	//判断是发生的那种错误
            }
            else
            {
                *usLen += ucNBytes;;							//帧长度加ucNBytes
            }
        }
        else
        {
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;					//非法数据值
        }
    }
    else
    {
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;						//非法数据值
    }
    return eStatus;
}

#if MB_FUNC_WRITE_COIL_ENABLED > 0
//函数功能;写单个线圈寄存器
//参数 pucFrame: 接收的PDU数据帧（ 功能码+寄存器起始地址（2个字节）+数据（2个字节）） 
//参数 usLen:    接收的PDU数据帧的长度

//写单个线圈寄存器帧格式:（从机地址+功能码+寄存器地址高位+寄存器地址低位+数据高位+数据低位+CRC高位+CRC低位）
//响应帧格式：如果写入成功，返回发送的指令
//功能码05H写单个线圈寄存器，FF00H请求线圈处于ON状态，0000H请求线圈处于OFF状态。
eMBException eMBFuncWriteCoil( UCHAR * pucFrame, USHORT * usLen )
{
    USHORT          usRegAddress;
    UCHAR           ucBuf[2];

    eMBException    eStatus = MB_EX_NONE;	//异常枚举变量设置为无错误
    eMBErrorCode    eRegStatus;				//错误枚举变量

    if( *usLen == ( MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN ) )	//如果PDU数据帧等于（功能码（1个字节）+寄存器地址（2个字节）+写线圈数据（2个字节））
    {
        usRegAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8 );	//得到写线圈寄存器的高位起始地址
        usRegAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1] );	//得到写线圈寄存器的低位起始地址
        usRegAddress++;

        if( ( pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF + 1] == 0x00 ) &&
            ( ( pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF ) ||
              ( pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0x00 ) ) )		//判断PDU数据帧中第3位和第4位为0x0000H还是0xFF00H
        {
            ucBuf[1] = 0;
            if( pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF )			//如果第3位为0xFFH，则置1
            {
                ucBuf[0] = 1;
            }
            else														//如果第3位为0x00H，则置0
            {
                ucBuf[0] = 0;
            }
            eRegStatus = eMBRegCoilsCB( &ucBuf[0], usRegAddress, 1, MB_REG_WRITE );
			Start_Flag=ucBuf[0];
            if( eRegStatus != MB_ENOERR )						//如果写数据发生错误
            {
                eStatus = prveMBError2Exception( eRegStatus );	//判断是发生的那种错误
            }
        }
        else
        {
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;					//非法数据值
        }
    }
    else
    {
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;						//非法数据值
    }
    return eStatus;
}

#endif

#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
//函数功能：写多个线圈寄存器
//参数 pucFrame: 接收的PDU数据帧（ 功能码+寄存器起始地址（2个字节）+写线圈数量（2个字节）+字节数+数据1+数据2...+数据n） 
//参数 usLen:    接收的PDU数据帧的长度

//写入多个线圈寄存器指令:（从机地址+功能码+寄存器地址高位+寄存器地址低位+数量高位+数量低位+字节数+数据1+数据2...+数据n+CRC高位+CRC低位）
//响应数据帧格式:（从机地址+功能码+寄存器地址高位+寄存器地址低位+数量高位+数量低位+CRC高位+CRC低位）
//功能码0FH写多个线圈寄存器。如果对应的数据位为1，表示线圈状态为ON；如果对应的数据位为0，表示线圈状态为OFF。
//线圈寄存器之间，低地址寄存器先传输，高地址寄存器后传输。单个线圈寄存器，高字节数据先传输，低字节数据后传输。
//如果写入的线圈寄存器的个数不是8的倍数，则在最后一个字节的高位补0。
eMBException eMBFuncWriteMultipleCoils( UCHAR * pucFrame, USHORT * usLen )
{
    USHORT          usRegAddress;
    USHORT          usCoilCnt;
    UCHAR           ucByteCount;
    UCHAR           ucByteCountVerify;

    eMBException    eStatus = MB_EX_NONE;
    eMBErrorCode    eRegStatus;

    if( *usLen > ( MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN ) )		//如果PDU数据帧大于（功能码（1个字节）+寄存器地址（2个字节）+写线圈数量（2个字节））
    {
        usRegAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8 );	//得到写线圈寄存器的高位起始地址
        usRegAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1] );	//得到写线圈寄存器的低位起始地址
        usRegAddress++;

        usCoilCnt = ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF] << 8 );	//得到写线圈数量的高位地址
        usCoilCnt |= ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF + 1] );	//得到写线圈数量的低位地址

        ucByteCount = pucFrame[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF];		

        
        if( ( usCoilCnt & 0x0007 ) != 0 )							//判断寄存器数目是否为8的倍数
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
         
            if( eRegStatus != MB_ENOERR )							//如果写数据发生错误
            {
                eStatus = prveMBError2Exception( eRegStatus );		//判断是发生的那种错误
            }
            else
            {
                *usLen = MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF;			//设置响应PDU数据帧的长度为5（功能码（1个字节）+寄存器地址（2个字节）+数量（2个字节））
            }
        }
        else
        {
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;						//非法数据值
        }
    }
    else
    {
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;							//非法数据值
    }
    return eStatus;
}

#endif

#endif
