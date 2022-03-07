/**********************************************************************************************************
** �ļ���		:user_mb_app.h
** ����			:maxlicheng<licheng.chn@outlook.com>
** ����github	:https://github.com/maxlicheng
** ���߲���		:https://www.maxlicheng.com/	
** ��������		:2018-02-18
** ����			:modbus��Ȧ���Ĵ�������
************************************************************************************************************/
#ifndef     USER_APP
#define     USER_APP
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbconfig.h"
#include "mbframe.h"
#include "mbutils.h"

/* -----------------------Slave Defines -------------------------------------*/
//************************************************************************//
//**02������ 
//**02 Read Discrete Inputs (1x)
//************************************************************************//
#define S_DISCRETE_INPUT_START        100       //���ؼĴ�����ʼ��ַ
#define S_DISCRETE_INPUT_NDISCRETES   16        //���ؼĴ�������

//************************************************************************//
//**01 05 15������
//**01 Read Coils��0x��  
//**05 Write Single Coil
//**15 Write Multiple Coils
//************************************************************************//
#define S_COIL_START                  0     //��Ȧ��ʼ��ַ 
#define S_COIL_NCOILS                 64    //��Ȧ����

//************************************************************************//
//**04������ (3x)
//**04 Read Input Registers
//************************************************************************//
#define S_REG_INPUT_START             0      //����Ĵ�����ʼ��ַ
#define S_REG_INPUT_NREGS             100       //����Ĵ�������

//************************************************************************//
//**03 06 16������ 
//**03 Read Holding Registers(4x)
//**06 Write Single Registers
//**16 Write Multiple Registers
//************************************************************************//
#define S_REG_HOLDING_START           0       	//���ּĴ�����ʼ��ַ
#define S_REG_HOLDING_NREGS           100       //���ּĴ�������

/* salve mode: holding register's all address */
#define          S_HD_RESERVE                     0
#define          S_HD_CPU_USAGE_MAJOR             1
#define          S_HD_CPU_USAGE_MINOR             2
/* salve mode: input register's all address */
#define          S_IN_RESERVE                     0
/* salve mode: coil's all address */
#define          S_CO_RESERVE                     0
/* salve mode: discrete's all address */
#define          S_DI_RESERVE                     0

/* -----------------------Master Defines -------------------------------------*/
#define M_DISCRETE_INPUT_START        0
#define M_DISCRETE_INPUT_NDISCRETES   16
#define M_COIL_START                  0
#define M_COIL_NCOILS                 64
#define M_REG_INPUT_START             0
#define M_REG_INPUT_NREGS             100
#define M_REG_HOLDING_START           0
#define M_REG_HOLDING_NREGS           100
/* master mode: holding register's all address */
#define          M_HD_RESERVE                     0
/* master mode: input register's all address */
#define          M_IN_RESERVE                     0
/* master mode: coil's all address */
#define          M_CO_RESERVE                     0
/* master mode: discrete's all address */
#define          M_DI_RESERVE                     0

#endif
