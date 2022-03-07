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

#ifndef _MB_PROTO_H
#define _MB_PROTO_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif
/* ----------------------- Defines ------------------------------------------*/
#define MB_ADDRESS_BROADCAST    ( 0 )   //Modbus广播地址
#define MB_ADDRESS_MIN          ( 1 )   //最小从机地址
#define MB_ADDRESS_MAX          ( 247 ) //最大从机地址
#define MB_FUNC_NONE                          (  0 )	
#define MB_FUNC_READ_COILS                    (  1 )	//读取线圈状态，取得一组逻辑线圈的当前状态（ON/OFF)
//线圈寄存器实际就是PLC的输出寄存器，一系列的存储单元，
//实际的做法就是继电器，那些存储单元当作是软继电器，
//和单片机控制继电器一样使用，再接近一点就是STM32的ODR和IDR寄存器了
#define MB_FUNC_READ_DISCRETE_INPUTS          (  2 )	//读取输入状态，取得一组开关输入的当前状态（ON/OFF)
#define MB_FUNC_READ_HOLDING_REGISTER         (  3 )	//读取保持寄存器，在一个或多个保持寄存器中取得当前的二进制值
#define MB_FUNC_READ_INPUT_REGISTER           (  4 )	//读取输入寄存器，在一个或多个输入寄存器中取得当前的二进制值
#define MB_FUNC_WRITE_SINGLE_COIL             (  5 )	//强置单线圈，强置一个逻辑线圈的通断状态
#define MB_FUNC_WRITE_REGISTER                (  6 )	//预置单寄存器，把具体二进值装入一个保持寄存器
#define MB_FUNC_DIAG_READ_EXCEPTION           (  7 )	//读取异常状态，取得8个内部线圈的通断状态，这8个线圈的地址由控制器决定，用户逻辑可以将这些线圈定义，以说明从机状态，短报文适宜于迅速读取状态
#define MB_FUNC_DIAG_DIAGNOSTIC               (  8 )	//回送诊断校验，把诊断校验报文送从机，以对通信处理进行评鉴
#define MB_FUNC_WRITE_MULTIPLE_COILS          ( 15 )	//强置多线圈，强置一串连续逻辑线圈的通断
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS      ( 16 )	//预置多寄存器，把具体的二进制值装入一串连续的保持寄存器
#define MB_FUNC_READWRITE_MULTIPLE_REGISTERS  ( 23 )	//保留作扩展功能备用


#define MB_FUNC_DIAG_GET_COM_EVENT_CNT        ( 11 )	//读取事件计数，可使主机发出单询问，并随即判定操作是否成功，尤其是该命令或其他应答产生通信错误时
#define MB_FUNC_DIAG_GET_COM_EVENT_LOG        ( 12 )	//读取通信事件记录，可是主机检索每台从机的ModBus事务处理通信事件记录。如果某项事务处理完成，记录会给出有关错误
#define MB_FUNC_OTHER_REPORT_SLAVEID          ( 17 )	//报告从机标识，可使主机判断编址从机的类型及该从机运行指示灯的状态
#define MB_FUNC_ERROR                         ( 128 )	//用于异常应答
/* ----------------------- Type definitions ---------------------------------*/
    typedef enum
{
    MB_EX_NONE = 0x00,
    MB_EX_ILLEGAL_FUNCTION = 0x01,
    MB_EX_ILLEGAL_DATA_ADDRESS = 0x02,
    MB_EX_ILLEGAL_DATA_VALUE = 0x03,
    MB_EX_SLAVE_DEVICE_FAILURE = 0x04,
    MB_EX_ACKNOWLEDGE = 0x05,
    MB_EX_SLAVE_BUSY = 0x06,
    MB_EX_MEMORY_PARITY_ERROR = 0x08,
    MB_EX_GATEWAY_PATH_FAILED = 0x0A,
    MB_EX_GATEWAY_TGT_FAILED = 0x0B
} eMBException;

typedef         eMBException( *pxMBFunctionHandler ) ( UCHAR * pucFrame, USHORT * pusLength );

typedef struct
{
    UCHAR           ucFunctionCode;
    pxMBFunctionHandler pxHandler;
} xMBFunctionHandler;

#ifdef __cplusplus
PR_END_EXTERN_C
#endif
#endif
