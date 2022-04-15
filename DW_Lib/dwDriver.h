#ifndef __DWDRIVER_H
#define __DWDRIVER_H

#include "types_plus.h"

#define DW_COL_RED		0xF800
#define DW_COL_GREEN	0x07E0
#define DW_COL_BLUE 	0x001F
#define DW_COL_WHITE	0xFFFF
#define DW_COL_BLACK	0x0000

#define DW_KEY_PRESSED	0x79   // 触控按下
#define DW_KEY_RELEASE	0x78   // 触控松手
#define DW_CMD_PRESSED	0x73   // 按下
#define DW_CMD_RELEASE	0x72   // 松手
#define DW_CMD_DONE     0xE4   // 校正完成

#define DW_KEY_NONE     0xFF   // 无按键

typedef enum TextSize{
	DW_SIZE_8,
	DW_SIZE_12,
	DW_SIZE_16,
	DW_SIZE_24,
	DW_SIZE_32,
	DW_SIZE_48,
}TextSize;

typedef struct Button{
	u8 picNum;
	u16  xs;
	u16  ys;
	u16  xe;
	u16  ye;
}Button;

typedef struct Image{
	u8 picNum;
	u16  xs;
	u16  ys;
	u16  xe;
	u16  ye;
}Image;

typedef struct{
	void(*pressHandle)(void);
	void(*freeHandle)(void);
	Button* button;
	u16 command;
}DwKeyListen;

extern u16  locaX, locaY;
extern u8 language; 
extern DwKeyListen dwKeyListen[30];

void dwListenCoord(void(*press)(void), void(*free)(void), const Button* btn);
void dwHandlerCoord(void);
void dwListenButton(void(*press)(void), void(*free)(void), u16 Command);
void dwHandlerButton(void);
void AVinput(u8 i, u8 contrast, u8 bright, u8 saturation, u8 hue);

// param lang: LANGUAGE_CHINESE or LANGUAGE_ENGLISH
void dwSetLanguage(u8 lang);

// 设置调色板
void dwSetColor(u16 f,u16 b);


/***取指定位置作为背景色***/
void dwSetBackColor(u16 x,u16 y);

/***取指定位置作为前景色***/
void dwSetForeColor(u16 x,u16 y);

// L = 1区分语言，l= 0 不区分语言
void dwDisButton(const Button* button,u8 l,u16 x,u16 y);

// L = 1区分语言，l= 0 不区分语言
void dwDisImage(const Image* image,u8 l,u16 x,u16 y);


/*****全屏显示指定位置图片不区分语言*****/
//para: Num 为位置索引号
void dwDisPicNoL(u8 picNum);

/*****全屏显示指定位置图片区分语言*****/
//para: Num 为位置索引号
void dwDisPicWithL(u8 picNum);

/**** 屏幕自带蜂鸣器响一声 *******/
void dwSound(u8 duration);  // duration*10ms


/****触屏校准***/
void dwCalibration(void);


/******配置屏幕工作模式******/
void dwSetMode(void);

/****剪切图片显示到指定位置区分语言****/
void dwCutPic(u8 picNum,u16 xs,u16 ys,u16 xe,u16 ye,u16 x,u16 y);

 /****剪切图片显示到指定位置不区分语言****/
void dwCutPicL(u8 picNum,u16 xs,u16 ys,u16 xe,u16 ye,u16 x,u16 y);


 /*****指示大小指定位置显示一个字符*****/
 //prama: size=8 8*8; 16 16*16; 24 24*24; 32 32*32
 void dwDisChar(TextSize size, u16 x, u16 y, u8 c);


 /*****指示大小指定位置显示一个数字*****/
 //prama: size=8 8*8; 16 16*16; 24 24*24; 32 32*32
 void dwDisNum(TextSize size, u16 x, u16 y, u8 c);


 /*****指示大小指定位置显示文字*****/
 //prama: size=8 8*8; 16 16*16; 24 24*24; 32 32*32
 void dwDisString(TextSize size, u16 x, u16 y, u8 string[]);


 /*****等待松手*****/
void dwWaitRelease(void);

 /*****等待按下*****/
void dwWaitPress(void);

 /***查询按键状态****/
 // 0x73  按下
 // 0x72  松手
 // 0xE4  触控校正完成
 u8 dwQueryCmd(void);

void dwCancelKey(void);

//播放音乐
void dwPlayMusic(u8 id, u8 num);

// 停止音乐
void dwStopMusic(void);

//调节音量
void dwPlayVol(u8 i);

//获取发送数据缓存地址, 和数据长度地址
void dwGetSendData(u8 **i, unsigned int **j);

char dwMount(void);

char dwRemove(void);

#endif
