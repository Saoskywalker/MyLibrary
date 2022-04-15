/**********************************************
 * name: command application v1.0
 * email:support@mtf123.club
 * Copyright(c) MTF 2016~2030. www.mtf123.club
 * All Rights Reserved
 * ******************************************/

#include "dwDriver.h"

u16  locaX, locaY;
u8 language; 
u8 keyNum = 0;
u16 fColor = DW_COL_WHITE;
u16 bcolor = DW_COL_BLACK;

DwKeyListen dwKeyListen[30];

void(*dwSend)(u8);

union CharInt{
	u8 b[2];
	u16 a;
};

#include <malloc.h>

static unsigned int _data_cnt = 0;
static u8 *_data = NULL;
#define BUF_MAX 256

char dwMount(void)
{
	_data_cnt = 0;
	_data = malloc(BUF_MAX); //缓存最大256字节
	if (_data == NULL)
		return 1;
	else
		return 0;
}

char dwRemove(void)
{
	free(_data);
	return 0;
}

void dwGetSendData(u8 **i, unsigned int **j)
{
	*i = _data;
	*j = &_data_cnt;
}

/*******发送帧头*********/
static __INLINE void dwSendStart(void)
{
	_data_cnt = 0;
}

/*******发送帧尾*********/
static __INLINE void dwSendOver(void)
{
	// MTF_Com_sendTail();
}

/***发送帧用户数据***/
static __INLINE void dwSendBuf(u8 byte)
{
	if (_data_cnt < BUF_MAX)
	{
		_data[_data_cnt] = byte;
		_data_cnt++;
	}
}

/*******************about key********************/
// static u8 keyFlag = 0;
void dwCancelKey(void){
	keyNum = 0;
}

//****************坐标模式*****************
void dwListenCoord(void(*press)(void), void(*free)(void), const Button* btn){
	if(keyNum < 30){
		dwKeyListen[keyNum].pressHandle = press;
		dwKeyListen[keyNum].freeHandle = free;
		dwKeyListen[keyNum].button = (Button*)btn;
		keyNum++;
	}
}

/**将坐标数据转换成int 型**/
/* static void dwConvertLocation(void){
	locaX = 0;
	locaX = frameData[1];
	locaX <<= 8;
	locaX += frameData[2];
	
	locaY = 0;
	locaY = frameData[3];
	locaY <<= 8;
	locaY += frameData[4];
}

static u8 dwGetKey(void){
	u8 i;
	dwConvertLocation();
	for(i=0; i<keyNum; i++){	
		if((locaX >= dwKeyListen[i].button->xs) && (locaX <= dwKeyListen[i].button->xe)
			&& (locaY >= dwKeyListen[i].button->ys) && (locaY <= dwKeyListen[i].button->ye)){
			break;
		}
	}
	return i;
}

void dwHandlerCoord(void){
	static u8 i;

	if (keyFlag)
	{
		// 松手处理
		if (dwQueryCmd() == DW_CMD_RELEASE)
			{
				keyFlag = 0;
				//		if((i < keyNum)&&(i==dwGetKey())){ // 有效按键
				if ((i < keyNum))
				{ // 有效按键
					if (dwKeyListen[i].freeHandle != 0)
					{ // 不为空
						dwKeyListen[i].freeHandle();
					}
				}
			}
	}
	else
	{
		// 按下处理
		if (dwQueryCmd() == DW_CMD_PRESSED)
		{
			keyFlag = 1;
			i = dwGetKey();
			if (i < keyNum)
			{ // 有效按键
				if (dwKeyListen[i].pressHandle != 0)
				{ // 不为空
					dwKeyListen[i].pressHandle();
				}
			}
		}
	}
} */

//***************触控模式***********************
/* void dwListenButton(void(*press)(void), void(*free)(void), u16 Command){
	if(keyNum < 30){
		dwKeyListen[keyNum].pressHandle = press;
		dwKeyListen[keyNum].freeHandle = free;
		dwKeyListen[keyNum].command = Command;
		keyNum++;
	}
} */

/* void dwHandlerButton(void){
	static u8 i;
	u16 cmd = 0;

	if (keyFlag)
	{
		// 松手处理
		if (dwQueryCmd() == DW_KEY_RELEASE)
		{
			keyFlag = 0;
			//		if((i < keyNum)&&(i==dwGetKey())){ // 有效按键
			if ((i < keyNum))
			{ // 有效按键
				if (dwKeyListen[i].freeHandle != 0)
				{ // 不为空
					dwKeyListen[i].freeHandle();
				}
			}
		}
	}
	else
	{
		// 按下处理
		if (dwQueryCmd() == DW_KEY_PRESSED)
		{
			keyFlag = 1;
			cmd = ((u16)frameData[1] << 8) + frameData[2];
			for (i = 0; i < keyNum; i++)
			{
				if (cmd == dwKeyListen[i].command)
					break;
			}
			if (i < keyNum)
			{ // 有效按键
				if (dwKeyListen[i].pressHandle != 0)
				{ // 不为空
					dwKeyListen[i].pressHandle();
				}
			}
		}
	}
} */

// 设置调色板
void dwSetColor(u16 f,u16 b){
	fColor = f;
	bcolor = b;
	dwSendStart();
	dwSendBuf(0x40);
	dwSendBuf((u8)(f>>8));
	dwSendBuf((u8)f);
	dwSendBuf((u8)(b>>8));
	dwSendBuf((u8)b);
	dwSendOver();
}

/***取指定位置作为背景色***/
void dwSetBackColor(u16 x,u16 y){
	dwSendStart();
	dwSendBuf(0x42);
	dwSendBuf((u8)(x>>8));
	dwSendBuf((u8)x);
	dwSendBuf((u8)(y>>8));
	dwSendBuf((u8)y);
	dwSendOver();
}

/***取指定位置作为前景色***/
void dwSetForeColor(u16 x,u16 y){
	dwSendStart();
	dwSendBuf(0x43);
	dwSendBuf((u8)(x>>8));
	dwSendBuf((u8)x);
	dwSendBuf((u8)(y>>8));
	dwSendBuf((u8)y);
	dwSendOver();
}

/*****全屏显示指定位置图片不区分语言*****/
//para: Num 为位置索引号
void dwDisPicNoL(u8 picNum){
	dwSendStart();
	dwSendBuf(0x70); //显示图片命令
	dwSendBuf(picNum);
	dwSendOver();
}

/*****全屏显示指定位置图片区分语言*****/
//para: Num 为位置索引号
void dwDisPicWithL(u8 picNum){
	dwDisPicNoL(picNum + language);
}

// L = 1区分语言，l= 0 不区分语言
void dwDisButton(const Button* button,u8 l,u16 x,u16 y){
	if(l){
		dwCutPicL(button->picNum,button->xs,button->ys,button->xe,button->ye,x,y);
	}else{
		dwCutPic(button->picNum,button->xs,button->ys,button->xe,button->ye,x,y);
	}
}

// L = 1区分语言，l= 0 不区分语言
void dwDisImage(const Image* image,u8 l,u16 x,u16 y){
	if(l){
		dwCutPicL(image->picNum,image->xs,image->ys,image->xe,image->ye,x,y);
	}else{
		dwCutPic(image->picNum,image->xs,image->ys,image->xe,image->ye,x,y);
	}
}

/****剪切图片显示到指定位置不区分语言****/
void dwCutPic(u8 picNum,u16 xs,u16 ys,u16 xe,u16 ye,u16 x,u16 y){
	dwSendStart();

	dwSendBuf(0x71);//剪切图片命令
	dwSendBuf(picNum);
	
	dwSendBuf((u8)(xs>>8));
	dwSendBuf((u8)xs);
	dwSendBuf((u8)(ys>>8));
	dwSendBuf((u8)ys);
	
	dwSendBuf((u8)(xe>>8));
	dwSendBuf((u8)xe);
	dwSendBuf((u8)(ye>>8));
	dwSendBuf((u8)ye);

	dwSendBuf((u8)(x>>8));
	dwSendBuf((u8)x);
	dwSendBuf((u8)(y>>8));
	dwSendBuf((u8)y);

	dwSendOver();
}

/****剪切图片显示到指定位置区分语言****/
void dwCutPicL(u8 picNum,u16 xs,u16 ys,u16 xe,u16 ye,u16 x,u16 y){
	dwCutPic(picNum+language,xs,ys,xe,ye,x,y);
}

/*****指示大小指定位置显示一个字符*****/
//prama: size=8 8*8; 16 16*16; 24 24*24; 32 32*32
void dwDisChar(TextSize size, u16 x, u16 y, u8 c){
	u8 temp[2]= {0,0};
	temp[0] = c;
	dwDisString(size,x,y,temp);
}

/*****指示大小指定位置显示一个数字*****/
//prama: size=8 8*8; 16 16*16; 24 24*24; 32 32*32
void dwDisNum(TextSize size, u16 x, u16 y, u8 c){
	if(c < 10){
		u8 temp[2]= {0,0};
		temp[0] = c + 0x30;
		dwDisString(size,x,y,temp);
	}	
}

/*****指示大小指定位置显示文字*****/
//prama: size=8 8*8; 16 16*16; 24 24*24; 32 32*32
void dwDisString(TextSize size, u16 x, u16 y, u8 string[]){
	u8 i = 0;
	dwSendStart();
	switch(size){
		case DW_SIZE_8:
			dwSendBuf(0x53);
			break;
		case DW_SIZE_12:
			dwSendBuf(0x6E);
			break;
		case DW_SIZE_16:
			dwSendBuf(0x54);
			break;
		case DW_SIZE_24:
			dwSendBuf(0x6F);
			break;
		case DW_SIZE_32:
			dwSendBuf(0x55);
			break;	
		case DW_SIZE_48:
			dwSendBuf(0x98);
			break;
	}
	dwSendBuf((u8)(x>>8));
	dwSendBuf((u8)x);
	dwSendBuf((u8)(y>>8));
	dwSendBuf((u8)y);
	if(size == DW_SIZE_48){
		dwSendBuf(0x24);    // lib_ID
		dwSendBuf(0x81);    // C_Mode
		dwSendBuf(0x05);    // C_Dots
		dwSendBuf((u8)(fColor>>8));
		dwSendBuf((u8)fColor);
		dwSendBuf((u8)(bcolor>>8));
		dwSendBuf((u8)bcolor);
	}
	while(string[i]){
		dwSendBuf(string[i]);
		i++;
	}
	dwSendOver();
}

/****** 配置屏幕工作模式 ******/
void dwSetMode(void){
	dwSendStart();
	dwSendBuf(0xE0);
	dwSendBuf(0x55);
	dwSendBuf(0x5A);
	dwSendBuf(0xA5);
	dwSendBuf(0x00);  // 显示模式
	dwSendBuf(0x07);  // 波特率115200
	dwSendBuf(0x48);  // 触控模式 只上传一次0x73
	dwSendBuf(0xEF);  // 文本显示模式滤除背景
	dwSendOver();
}

/**** 屏幕自带蜂鸣器响一声 *******/
void dwSound(u8 duration){  // duration*10ms
	dwSendStart();
	dwSendBuf(0x79);
	dwSendBuf(duration);
	dwSendOver();
}

 /*****等待松手*****/
/* void dwWaitRelease(void){
	while(frameData[0] != 0x72 && frameData[0] != 0x78);
} */

 /*****等待按下*****/
/* void dwWaitPress(void){
	while(frameData[0] != 0x73 && frameData[0] != 0x79);
} */

/***查询按键状态****/
// DW_CMD_PRESSED= 0x73  按下
// DW_CMD_RELEASE= 0x72  松手
// DW_CMD_DONE   = 0xE4  触控校正完成

/****查询接收的命令***/
// u8 dwQueryCmd(void)
// {
// 	if(frameFlag) //是否有数据帧
// 	{
// 		frameFlag = 0;
// 		return frameData[0];
// 	}
// 	else
// 	{
// 		return 0XFF;
// 	}
// }

/****触屏校准***/
/* void dwCalibration(void){ 
	dwSendStart();
	dwSendBuf(0xE4); 
	dwSendBuf(0x55);
	dwSendBuf(0x5A);
	dwSendBuf(0xA5);
	dwSendOver();
	while(dwQueryCmd() != 0xE4);  // 等待收到校准成功指令
} */

// 最大音量播放音乐
// param id:  0--120;
// param num: 1--120;
// extern u8 muteFlag;
void dwPlayMusic(u8 id, u8 num){
	// if(muteFlag) return; //静音
	dwSendStart();
	dwSendBuf(0x30);
	dwSendBuf(id);
	dwSendBuf(num);
	dwSendBuf(0xA0); 	//音量0x00~0xff
	dwSendOver();
}

// 停止音乐
void dwStopMusic(void){
	dwSendStart();
	dwSendBuf(0x30);
	dwSendBuf(0);
	dwSendBuf(0);
	dwSendBuf(0);   
	dwSendOver();
}

//调节音量
void dwPlayVol(u8 i){
	dwSendStart();
	dwSendBuf(0x32);
	dwSendBuf(i);
	dwSendOver();
}

//AV输入
//i: 0关闭, 1开启
//contrast(0~255): 对比度(默认128), bright(0~255): 亮度(默认32) 
//saturation(0~255): 饱和度(默认128), hue(0~255): 色调(默认0) 
void AVinput(u8 i, u8 contrast, u8 bright, u8 saturation, u8 hue)
{
	dwSendStart();
	dwSendBuf(0x8A);
	dwSendBuf(i);
	dwSendBuf(contrast);
	dwSendBuf(bright);
	dwSendBuf(saturation);
	dwSendBuf(hue);
	dwSendOver();
}
