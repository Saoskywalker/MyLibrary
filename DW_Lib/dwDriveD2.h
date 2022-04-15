/**********************************************
 * name: communicate api v1.0
 * History: 2019.2.15: Created
 * Editor: Aysi
 * email:support@mtf123.club
 * Copyright(c) MTF 2016~2030. www.mtf123.club
 * All Rights Reserved
 * ******************************************/

#ifndef __DW_DRIVE_D2_H
#define __DW_DRIVE_D2_H

//if Rec package over time, this data cancel, 
//set 0 will cancel over time function
//Note: overtime is must bigger than Rec one frame time 
#define dwD2RecOvertime 10

//after send frame, need get respond(become slowly), set 0 will not use
#define dwD2RespondOvertime 0

// #define D2_CHECK_CODE_ENABLE //enable crc16 check code

#define dwD2HEAD 0X5AA5 //Head package

#include "types_plus.h"

extern u8 dwD2RecSta, dwD2SendSta, dwD2Lang, dwD2RecDataBusy;

#define dwD2CacheSize 20 //Rec cache

typedef struct
{
	void(*pressHandle)(void);
    u16 button;
}_DwD2KeyListen;

static __INLINE u8 dwD2GetRecSta(void)
{
    return dwD2RecSta;
}

static __INLINE u8 dwD2GetSendSta(void)
{
    return dwD2SendSta;
}

static __INLINE void dwD2SetLanguage(u8 i)
{
    dwD2Lang = i;
}

//Base
void dwD2TimeProcess(void);
void dwD2SendFrame(u8 *i, u8 p);
void dwD2RecFrame(u8 i);

/*display string*/
void dwD2DisString(u16 address, u8 *s);
void dwD2DisNum(u16 address, u8 i);
void dwD2DisTime(u16 address, u16 timing);
void dwD2DisChar(u16 address, u8 i);
void dwD2DisFre(u16 address, u16 i);

//DWD2 Function
u16 dwD2GetPicID(void);
void dwD2SetBL(u8 i, u8 j, u16 k);
void dwD2Rest(void);
u8 dwD2Check(void);
void dwD2DisICO(u16 address, u8 i);
void dwD2CameraOpen(void);
void dwD2Print(void);

/*Page change*/
void dwD2DisPicWithL(u8 i);
void dwD2DisPicNoL(u8 i);

//Key process
#define KEY_ADDRESS 0X1080
void dwD2CancelKey(void);
void dwD2ListenKey(void(*press)(void), const u16 btn);
void dwD2Handler(void);

#endif


