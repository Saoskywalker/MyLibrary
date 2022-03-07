/**********************************************************************************************************
** 文件名		:user_mb_app.c
** 作者			:maxlicheng<licheng.chn@outlook.com>
** 作者github	:https://github.com/maxlicheng
** 作者博客		:https://www.maxlicheng.com/	
** 生成日期		:2018-02-18
** 描述			:modbus 线圈及寄存器初始化
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
//**02功能码 
//**02 Read Discrete Inputs (1x)
//**起始地址 10000
//************************************************************************//
USHORT   usSDiscInStart                               = S_DISCRETE_INPUT_START;
#if S_DISCRETE_INPUT_NDISCRETES%8
UCHAR    ucSDiscInBuf[S_DISCRETE_INPUT_NDISCRETES/8+1];
#else
UCHAR    ucSDiscInBuf[S_DISCRETE_INPUT_NDISCRETES/8]  ;
#endif

//Slave mode:Coils variables
//************************************************************************//
//**01 05 15功能码
//**01 Read  Coils（0x）  
//**05 Write Single Coil
//**15 Write Multiple Coils
//**起始地址 00000
//************************************************************************//
USHORT   usSCoilStart                                 = S_COIL_START;
#if S_COIL_NCOILS%8
UCHAR    ucSCoilBuf[S_COIL_NCOILS/8+1]        ;
#else
UCHAR    ucSCoilBuf[S_COIL_NCOILS/8]       ;                
#endif

//Slave mode:InputRegister variables
//************************************************************************//
//**04功能码 (3x)
//**04 Read Input Registers
//**起始地址 30000
//************************************************************************//
USHORT   usSRegInStart                                = S_REG_INPUT_START;
USHORT   usSRegInBuf[S_REG_INPUT_NREGS]                ;

//Slave mode:HoldingRegister variables
//************************************************************************//
//**03 06 16功能码 
//**03 Read  Holding Registers(4x)
//**06 Write Single Registers
//**16 Write Multiple Registers
//**起始地址 40000
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


//函数功能：读或写保持寄存器
//参数 pucRegBuffer: 保持寄存器数据缓存区
//参数 usAddress:    保持寄存器首地址
//参数 usNRegs:	 	 读或写寄存器的个数	
//参数 eMode:		 读写模式
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

    if ((usAddress >= REG_HOLDING_START) && (usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS))	//如果地址符合要求
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


//函数功能：读或写线圈寄存器
//参数 pucRegBuffer: 线圈寄存器数据缓存区
//参数 usAddress:    线圈寄存器首地址
//参数 usNCoils:	 读或写的线圈个数	
//参数 eMode:		 读写模式
eMBErrorCode eMBRegCoilsCB(UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex , iRegBitIndex , iNReg;
    UCHAR *         pucCoilBuf;
    USHORT          COIL_START;
    USHORT          COIL_NCOILS;
    USHORT          usCoilStart;
    iNReg =  usNCoils / 8 + 1;			
    pucCoilBuf = ucSCoilBuf;		//线圈缓冲区域首地址
    COIL_START = S_COIL_START;		//线圈起始地址
    COIL_NCOILS = S_COIL_NCOILS;	//线圈数量
    usCoilStart = usSCoilStart;		//线圈起始地址 

    //它已经在modbus函数方法中加1了
    usAddress--;

    if( ( usAddress >= COIL_START ) &&( usAddress + usNCoils <= COIL_START + COIL_NCOILS ) )	//如果地址符合要求
    {
        iRegIndex = (USHORT) (usAddress - usCoilStart) / 8;			//得到byte的偏移量
        iRegBitIndex = (USHORT) (usAddress - usCoilStart) % 8;		//得到bit的偏移量
        switch ( eMode )
        {
			case MB_REG_READ:
				while (iNReg > 0)
				{
					*pucRegBuffer++ = xMBUtilGetBits(&pucCoilBuf[iRegIndex++], iRegBitIndex, 8);
					iNReg--;
				}
				pucRegBuffer--;
				//最后一组线圈处理
				usNCoils = usNCoils % 8;
				//高位填充为0
				*pucRegBuffer = *pucRegBuffer << (8 - usNCoils);
				*pucRegBuffer = *pucRegBuffer >> (8 - usNCoils);
				break;			
			case MB_REG_WRITE:
				while (iNReg > 1)
				{
					xMBUtilSetBits(&pucCoilBuf[iRegIndex++], iRegBitIndex, 8, *pucRegBuffer++);
					iNReg--;
				}
				//最后一组线圈处理
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

