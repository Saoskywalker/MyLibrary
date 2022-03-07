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
#define MB_ADDRESS_BROADCAST    ( 0 )   //Modbus�㲥��ַ
#define MB_ADDRESS_MIN          ( 1 )   //��С�ӻ���ַ
#define MB_ADDRESS_MAX          ( 247 ) //���ӻ���ַ
#define MB_FUNC_NONE                          (  0 )	
#define MB_FUNC_READ_COILS                    (  1 )	//��ȡ��Ȧ״̬��ȡ��һ���߼���Ȧ�ĵ�ǰ״̬��ON/OFF)
//��Ȧ�Ĵ���ʵ�ʾ���PLC������Ĵ�����һϵ�еĴ洢��Ԫ��
//ʵ�ʵ��������Ǽ̵�������Щ�洢��Ԫ��������̵�����
//�͵�Ƭ�����Ƽ̵���һ��ʹ�ã��ٽӽ�һ�����STM32��ODR��IDR�Ĵ�����
#define MB_FUNC_READ_DISCRETE_INPUTS          (  2 )	//��ȡ����״̬��ȡ��һ�鿪������ĵ�ǰ״̬��ON/OFF)
#define MB_FUNC_READ_HOLDING_REGISTER         (  3 )	//��ȡ���ּĴ�������һ���������ּĴ�����ȡ�õ�ǰ�Ķ�����ֵ
#define MB_FUNC_READ_INPUT_REGISTER           (  4 )	//��ȡ����Ĵ�������һ����������Ĵ�����ȡ�õ�ǰ�Ķ�����ֵ
#define MB_FUNC_WRITE_SINGLE_COIL             (  5 )	//ǿ�õ���Ȧ��ǿ��һ���߼���Ȧ��ͨ��״̬
#define MB_FUNC_WRITE_REGISTER                (  6 )	//Ԥ�õ��Ĵ������Ѿ������ֵװ��һ�����ּĴ���
#define MB_FUNC_DIAG_READ_EXCEPTION           (  7 )	//��ȡ�쳣״̬��ȡ��8���ڲ���Ȧ��ͨ��״̬����8����Ȧ�ĵ�ַ�ɿ������������û��߼����Խ���Щ��Ȧ���壬��˵���ӻ�״̬���̱���������Ѹ�ٶ�ȡ״̬
#define MB_FUNC_DIAG_DIAGNOSTIC               (  8 )	//�������У�飬�����У�鱨���ʹӻ����Զ�ͨ�Ŵ����������
#define MB_FUNC_WRITE_MULTIPLE_COILS          ( 15 )	//ǿ�ö���Ȧ��ǿ��һ�������߼���Ȧ��ͨ��
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS      ( 16 )	//Ԥ�ö�Ĵ������Ѿ���Ķ�����ֵװ��һ�������ı��ּĴ���
#define MB_FUNC_READWRITE_MULTIPLE_REGISTERS  ( 23 )	//��������չ���ܱ���


#define MB_FUNC_DIAG_GET_COM_EVENT_CNT        ( 11 )	//��ȡ�¼���������ʹ����������ѯ�ʣ����漴�ж������Ƿ�ɹ��������Ǹ����������Ӧ�����ͨ�Ŵ���ʱ
#define MB_FUNC_DIAG_GET_COM_EVENT_LOG        ( 12 )	//��ȡͨ���¼���¼��������������ÿ̨�ӻ���ModBus������ͨ���¼���¼�����ĳ����������ɣ���¼������йش���
#define MB_FUNC_OTHER_REPORT_SLAVEID          ( 17 )	//����ӻ���ʶ����ʹ�����жϱ�ַ�ӻ������ͼ��ôӻ�����ָʾ�Ƶ�״̬
#define MB_FUNC_ERROR                         ( 128 )	//�����쳣Ӧ��
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
