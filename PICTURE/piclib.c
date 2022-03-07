//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板V3
//图片解码 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/5/15
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved
//********************************************************************************
//升级说明
//无
//////////////////////////////////////////////////////////////////////////////////

#include "piclib.h"
#include "stdio.h"
#include "lodepng.h"
#include "file_type.h"

_pic_info picinfo;	 	//图片信息
_pic_phy pic_phy;		//图片显示物理接口	

//////////////////////////////////////////////////////////////////////////
//移植改动处
//lcd.h没有提供划横线函数,需要自己实现
//画线函数实现,仅GIF需要
static void piclib_draw_hline(u16 x0,u16 y0,u16 len,ColorClass color)
{
	if((len==0)||(x0>lcddev.width)||(y0>lcddev.height))
		return;
	LCD_Fill(x0,y0,x0+len-1,y0,color);	
}
//填充函数实现,仅GIF需要
static void piclib_fill(u16 sx, u16 sy, u16 ex, u16 ey, ColorClass color)
{
	LCD_Fill(sx, sy, ex, ey, color);
}
//画点函数,仅GIF需要
static void piclib_gif_point(u16 x, u16 y, ColorClass color)
{
	LCD_Fast_DrawPoint(x, y, color);
}

//填充颜色
//x,y:起始坐标
//width，height：宽度和高度。
//*color：颜色数组
//颜色填充函数实现,仅TJPGD快速解码需要
static void piclib_fill_color(u16 x,u16 y,u16 width,u16 height,ColorClass *color)
{  
	u32 total, cnt = 0;
	u16 *data = (u16 *)color;
	ColorClass *data2 = 0;

	data2 = (ColorClass *)pic_memalloc(width*height*sizeof(ColorClass));
	if(data2==0)
		return;
	for(total = width*height; cnt<total; cnt++)
		data2[cnt] = RGB565to888(&data[cnt]);
	LCD_Color_Fill(x,y,x+width-1,y+height-1,data2);	
	pic_memfree(data2);
}

//画图初始化,在画图之前,必须先调用此函数
//指定画点/读点
void piclib_init(void)
{
	pic_phy.read_point=LCD_ReadPoint;  		//读点函数实现,仅BMP需要
	pic_phy.draw_point=LCD_Fast_DrawPoint;	//画点函数实现bmp, TJPGD正常解码需要
	pic_phy.fill=piclib_fill;					//填充函数实现,仅GIF需要
	pic_phy.draw_hline=piclib_draw_hline;  	//画线函数实现,仅GIF需要
	pic_phy.git_draw_point=piclib_gif_point;  	//画点函数,仅GIF需要
	pic_phy.fillcolor=piclib_fill_color;  	//颜色填充函数实现,仅TJPGD快速解码需要 

	picinfo.lcdwidth=lcddev.width;	//得到LCD的宽度像素
	picinfo.lcdheight=lcddev.height;//得到LCD的高度像素

	picinfo.ImgWidth=0;	//初始化宽度为0
	picinfo.ImgHeight=0;//初始化高度为0
	picinfo.Div_Fac=0;	//初始化缩放系数为0
	picinfo.S_Height=0;	//初始化设定的高度为0
	picinfo.S_Width=0;	//初始化设定的宽度为0
	picinfo.S_XOFF=0;	//初始化x轴的偏移量为0
	picinfo.S_YOFF=0;	//初始化y轴的偏移量为0
	picinfo.staticx=0;	//初始化当前显示到的x坐标为0
	picinfo.staticy=0;	//初始化当前显示到的y坐标为0
}
//////////////////////////////////////////////////////////////

//初始化智能画点
//内部调用
void ai_draw_init(void)
{
	float temp,temp1;	   
	temp=(float)picinfo.S_Width/picinfo.ImgWidth;
	temp1=(float)picinfo.S_Height/picinfo.ImgHeight;						 
	if(temp<temp1)temp1=temp;//取较小的那个	 
	if(temp1>1)temp1=1;	  
	//使图片处于所给区域的中间
	picinfo.S_XOFF+=(picinfo.S_Width-temp1*picinfo.ImgWidth)/2;
	picinfo.S_YOFF+=(picinfo.S_Height-temp1*picinfo.ImgHeight)/2;
	temp1*=8192;//扩大8192倍	 
	picinfo.Div_Fac=temp1;
	picinfo.staticx=0xffff;
	picinfo.staticy=0xffff;//放到一个不可能的值上面			 										    
}   

//判断这个像素是否可以显示
//(x,y) :像素原始坐标
//chg   :功能变量. 
//返回值:0,不需要显示.1,需要显示
u8 is_element_ok(u16 x,u16 y,u8 chg)
{				  
	if(x!=picinfo.staticx||y!=picinfo.staticy)
	{
		if(chg==1)
		{
			picinfo.staticx=x;
			picinfo.staticy=y;
		} 
		return 1;
	}else return 0;
}

//智能画图
//FileName:要显示的图片文件  BMP/JPG/JPEG/GIF
//x,y,width,height:坐标及显示区域尺寸
//fast:使能jpeg/jpg小图片(图片尺寸小于等于液晶分辨率)快速解码,0,不使能;1,使能.
//图片在开始和结束的坐标点范围内显示
u8 ai_load_picfile(const u8 *filename,u16 x,u16 y,u16 width,u16 height,u8 fast)
{	
	u8	res = 1;//返回值
	u8 temp;	
	if((x+width)>picinfo.lcdwidth)return PIC_WINDOW_ERR;		//x坐标超范围了.
	if((y+height)>picinfo.lcdheight)return PIC_WINDOW_ERR;		//y坐标超范围了.  
	//得到显示方框大小	  	 
	if(width==0||height==0)return PIC_WINDOW_ERR;	//窗口设定错误
	picinfo.S_Height=height;
	picinfo.S_Width=width;
	//显示区域无效
	if(picinfo.S_Height==0||picinfo.S_Width==0)
	{
		picinfo.S_Height=lcddev.height;
		picinfo.S_Width=lcddev.width;
		return FALSE;   
	}
	if(pic_phy.fillcolor==NULL)fast=0;//颜色填充函数未实现,不能快速显示
	//显示的开始坐标点
	picinfo.S_YOFF=y;
	picinfo.S_XOFF=x;
	//文件名传递		 
	temp = f_typetell((u8*)filename);	//得到文件的类型
	switch(temp)
	{											  
		case T_BMP:
			res=stdbmp_decode(filename); 				//解码bmp	  	  
			break;
		case T_JPG:
			// res=jpg_decode(filename,fast);				//解码JPG/JPEG	
			break; 
		case T_JPEG:
			// res=jpg_decode(filename,fast);				//解码JPG/JPEG	  	  
			break;
		case T_GIF:
			res=gif_decode2(filename,x,y,width,height);	//解码gif  	  
			break;
		case T_PNG:
			res = png_decode_dis(filename);	//解码PNG  
			break;
		default:
	 		res=PIC_FORMAT_ERR;  						//非图片格式!!!  
			break;
	}  											   
	return res;
}

//移植改动处
//动态分配内存
void *pic_memalloc (u32 size)			
{
	return (void*)malloc(size);
}
//释放内存
void pic_memfree (void* mf)		 
{
	free(mf);
}























