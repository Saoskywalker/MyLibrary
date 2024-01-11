//触摸屏驱动(支持ADS7843/7846/UH7843/7846/XPT2046/TSC2046/OTT2001A/GT9147/FT5206等)代码

#ifndef __TOUCH_H__
#define __TOUCH_H__

#include "types_plus.h"          

#define CT_MAX_TOUCH  5    		//电容屏支持的点数,固定为5点

//触摸屏控制器
typedef struct 
{
	u8 (*init)(void);			//初始化触摸屏控制器
	uint8_t (*scan)(int *x, int *y, uint8_t target_num, uint8_t *result_num); //获取触摸坐标, x, y: 坐标数组; target: 获取坐标数量; result: 获取结果数量 
	void (*adjust)(void);		//触摸屏校准 
	int x[CT_MAX_TOUCH]; 		//当前坐标
	int y[CT_MAX_TOUCH];		//电容屏有最多5组坐标,电阻屏则用x[0],y[0]代表:此次扫描时,触屏的坐标
	u8  sta;					//b7~b5:保留, b4~b0:电容触摸屏按下的点数(0,表示未按下,1表示按下)
/////////////////////触摸屏校准参数(电容屏不需要校准)//////////////////////								
	float xfac;					
	float yfac;
	short xoff;
	short yoff;	   

//b0(用于电阻屏XY反转):0,竖屏(适合左右为X坐标,上下为Y坐标的TP)
//   				  1,横屏(适合左右为Y坐标,上下为X坐标的TP) 
//b1~2: 屏幕方向 新增
//b3~6:保留.
//b7:0,电阻屏
//   1,电容屏 
	u8 touchtype;

//新增
	u16 width;   //LCD 宽度
	u16 height;  //LCD 高度
}_m_tp_dev;

extern _m_tp_dev tp_dev;	 	//触屏控制器在touch.c里面定义

void rtp_test(void); //画图,,,用于测试触屏

/*用于电阻屏*/
void TP_Save_Adjdata(void);						//保存校准参数
u8 TP_Get_Adjdata(void);						//读取校准参数
void TP_Adjust(void);							//触摸屏校准
void TP_Adjust_trigger(void); 				    //校准触发
extern u8 TPAjustFlag;							//校准状态标志
 
#endif

















