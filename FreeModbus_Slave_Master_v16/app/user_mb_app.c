/**********************************************************************************************************
** �ļ���		:user_mb_app.c
** ����			:maxlicheng<licheng.chn@outlook.com>
** ����github	:https://github.com/maxlicheng
** ���߲���		:https://www.maxlicheng.com/	
** ��������		:2018-02-18
** ����			:modbus ��Ȧ���Ĵ�����ʼ��
************************************************************************************************************/
/*
 * FreeModbus Libary: user callback functions and buffer define in slave mode
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: user_mb_app.c,v 1.60 2013/11/23 11:49:05 Armink $
 */
#include "user_mb_app.h"
#include "mb.h"

/*------------------------Slave mode use these variables----------------------*/
//Slave mode:DiscreteInputs variables
//************************************************************************//
//**02������ 
//**02 Read Discrete Inputs (1x)
//**��ʼ��ַ 10000
//************************************************************************//
USHORT   usSDiscInStart                               = S_DISCRETE_INPUT_START;
#if S_DISCRETE_INPUT_NDISCRETES%8
UCHAR    ucSDiscInBuf[S_DISCRETE_INPUT_NDISCRETES/8+1];
#else
UCHAR    ucSDiscInBuf[S_DISCRETE_INPUT_NDISCRETES/8]  ;
#endif

//Slave mode:Coils variables
//************************************************************************//
//**01 05 15������
//**01 Read  Coils��0x��  
//**05 Write Single Coil
//**15 Write Multiple Coils
//**��ʼ��ַ 00000
//************************************************************************//
USHORT   usSCoilStart                                 = S_COIL_START;
#if S_COIL_NCOILS%8
UCHAR    ucSCoilBuf[S_COIL_NCOILS/8+1]        ;
#else
UCHAR    ucSCoilBuf[S_COIL_NCOILS/8]       ;                
#endif

//Slave mode:InputRegister variables
//************************************************************************//
//**04������ (3x)
//**04 Read Input Registers
//**��ʼ��ַ 30000
//************************************************************************//
USHORT   usSRegInStart                                = S_REG_INPUT_START;
USHORT   usSRegInBuf[S_REG_INPUT_NREGS]                ;

//Slave mode:HoldingRegister variables
//************************************************************************//
//**03 06 16������ 
//**03 Read  Holding Registers(4x)
//**06 Write Single Registers
//**16 Write Multiple Registers
//**��ʼ��ַ 40000
//************************************************************************//
USHORT   usSRegHoldStart                              = S_REG_HOLDING_START;
USHORT   usSRegHoldBuf[S_REG_HOLDING_NREGS]           ;
 //__attribute__((at(0X40011800+0X0C)))
/**
 * Modbus slave input register callback function.
 *
 * @param pucRegBuffer input register buffer
 * @param usAddress input register address
 * @param usNRegs input register number
 *
 * @return result
 */
eMBErrorCode eMBRegInputCB(UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex;
    USHORT *        pusRegInputBuf;
    USHORT          REG_INPUT_START;
    USHORT          REG_INPUT_NREGS;
    USHORT          usRegInStart;

    pusRegInputBuf = usSRegInBuf;
    REG_INPUT_START = S_REG_INPUT_START;
    REG_INPUT_NREGS = S_REG_INPUT_NREGS;
    usRegInStart = usSRegInStart;

    /* it already plus one in modbus function method. */
    usAddress--;

    if ((usAddress >= REG_INPUT_START) && (usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS))
    {
        iRegIndex = usAddress - usRegInStart;
        while (usNRegs > 0)
        {
            *pucRegBuffer++ = (UCHAR) (pusRegInputBuf[iRegIndex] >> 8);
            *pucRegBuffer++ = (UCHAR) (pusRegInputBuf[iRegIndex] & 0xFF);
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}


//�������ܣ�����д���ּĴ���
//���� pucRegBuffer: ���ּĴ������ݻ�����
//���� usAddress:    ���ּĴ����׵�ַ
//���� usNRegs:	 	 ����д�Ĵ����ĸ���	
//���� eMode:		 ��дģʽ
eMBErrorCode eMBRegHoldingCB(UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex;
    USHORT *        pusRegHoldingBuf;
    USHORT          REG_HOLDING_START;
    USHORT          REG_HOLDING_NREGS;
    USHORT          usRegHoldStart;

    pusRegHoldingBuf = usSRegHoldBuf;
    REG_HOLDING_START = S_REG_HOLDING_START;
    REG_HOLDING_NREGS = S_REG_HOLDING_NREGS;
    usRegHoldStart = usSRegHoldStart;

    usAddress--;

    if ((usAddress >= REG_HOLDING_START) && (usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS))	//�����ַ����Ҫ��
    {
        iRegIndex = usAddress - usRegHoldStart;
        switch (eMode)
        {
			case MB_REG_READ:
				while (usNRegs > 0)
				{
					*pucRegBuffer++ = (UCHAR) (pusRegHoldingBuf[iRegIndex] >> 8);
					*pucRegBuffer++ = (UCHAR) (pusRegHoldingBuf[iRegIndex] & 0xFF);
					iRegIndex++;
					usNRegs--;
				}
				break;
			case MB_REG_WRITE:
				while (usNRegs > 0)
				{
					pusRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
					pusRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
					iRegIndex++;
					usNRegs--;
				}
				break;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}


//�������ܣ�����д��Ȧ�Ĵ���
//���� pucRegBuffer: ��Ȧ�Ĵ������ݻ�����
//���� usAddress:    ��Ȧ�Ĵ����׵�ַ
//���� usNCoils:	 ����д����Ȧ����	
//���� eMode:		 ��дģʽ
eMBErrorCode eMBRegCoilsCB(UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex , iRegBitIndex , iNReg;
    UCHAR *         pucCoilBuf;
    USHORT          COIL_START;
    USHORT          COIL_NCOILS;
    USHORT          usCoilStart;
    iNReg =  usNCoils / 8 + 1;			
    pucCoilBuf = ucSCoilBuf;		//��Ȧ���������׵�ַ
    COIL_START = S_COIL_START;		//��Ȧ��ʼ��ַ
    COIL_NCOILS = S_COIL_NCOILS;	//��Ȧ����
    usCoilStart = usSCoilStart;		//��Ȧ��ʼ��ַ 

    //���Ѿ���modbus���������м�1��
    usAddress--;

    if( ( usAddress >= COIL_START ) &&( usAddress + usNCoils <= COIL_START + COIL_NCOILS ) )	//�����ַ����Ҫ��
    {
        iRegIndex = (USHORT) (usAddress - usCoilStart) / 8;			//�õ�byte��ƫ����
        iRegBitIndex = (USHORT) (usAddress - usCoilStart) % 8;		//�õ�bit��ƫ����
        switch ( eMode )
        {
			case MB_REG_READ:
				while (iNReg > 0)
				{
					*pucRegBuffer++ = xMBUtilGetBits(&pucCoilBuf[iRegIndex++], iRegBitIndex, 8);
					iNReg--;
				}
				pucRegBuffer--;
				//���һ����Ȧ����
				usNCoils = usNCoils % 8;
				//��λ���Ϊ0
				*pucRegBuffer = *pucRegBuffer << (8 - usNCoils);
				*pucRegBuffer = *pucRegBuffer >> (8 - usNCoils);
				break;			
			case MB_REG_WRITE:
				while (iNReg > 1)
				{
					xMBUtilSetBits(&pucCoilBuf[iRegIndex++], iRegBitIndex, 8, *pucRegBuffer++);
					iNReg--;
				}
				//���һ����Ȧ����
				usNCoils = usNCoils % 8;
				if (usNCoils != 0)
				{
					xMBUtilSetBits(&pucCoilBuf[iRegIndex++], iRegBitIndex, usNCoils, *pucRegBuffer++);
				}
				break;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

/**
 * Modbus slave discrete callback function.
 *
 * @param pucRegBuffer discrete buffer
 * @param usAddress discrete address
 * @param usNDiscrete discrete number
 *
 * @return result
 */
eMBErrorCode eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex , iRegBitIndex , iNReg;
    UCHAR *         pucDiscreteInputBuf;
    USHORT          DISCRETE_INPUT_START;
    USHORT          DISCRETE_INPUT_NDISCRETES;
    USHORT          usDiscreteInputStart;
    iNReg =  usNDiscrete / 8 + 1;

    pucDiscreteInputBuf = ucSDiscInBuf;
    DISCRETE_INPUT_START = S_DISCRETE_INPUT_START;
    DISCRETE_INPUT_NDISCRETES = S_DISCRETE_INPUT_NDISCRETES;
    usDiscreteInputStart = usSDiscInStart;

    /* it already plus one in modbus function method. */
    usAddress--;

    if ((usAddress >= DISCRETE_INPUT_START)
            && (usAddress + usNDiscrete    <= DISCRETE_INPUT_START + DISCRETE_INPUT_NDISCRETES))
    {
        iRegIndex = (USHORT) (usAddress - usDiscreteInputStart) / 8;
        iRegBitIndex = (USHORT) (usAddress - usDiscreteInputStart) % 8;

        while (iNReg > 0)
        {
            *pucRegBuffer++ = xMBUtilGetBits(&pucDiscreteInputBuf[iRegIndex++],
                    iRegBitIndex, 8);
            iNReg--;
        }
        pucRegBuffer--;
        /* last discrete */
        usNDiscrete = usNDiscrete % 8;
        /* filling zero to high bit */
        *pucRegBuffer = *pucRegBuffer << (8 - usNDiscrete);
        *pucRegBuffer = *pucRegBuffer >> (8 - usNDiscrete);
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

