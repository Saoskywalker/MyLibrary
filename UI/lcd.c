//////////////////////////////////////////////////////////////////////////////////
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK 战舰STM32F103开发板V3
//2.4寸/2.8寸/3.5寸/4.3寸/7寸 TFT液晶驱动
//支持驱动IC型号包括:ILI9341/ILI9325/RM68042/RM68021/ILI9320/ILI9328/LGDP4531/LGDP4535/
//                  SPFD5408/1505/B505/C505/NT35310/NT35510/SSD1963等
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2010/7/4
//版本：V2.9
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved
//********************************************************************************
//V1.2修改说明
//支持了SPFD5408的驱动,另外把液晶ID直接打印成HEX格式.方便查看LCD驱动IC.
//V1.3
//加入了快速IO的支持
//修改了背光控制的极性（适用于V1.8及以后的开发板版本）
//对于1.8版本之前(不包括1.8)的液晶模块,请修改LCD_Init函数的LCD_LED=1;为LCD_LED=1;
//V1.4
//修改了LCD_ShowChar函数，使用画点功能画字符。
//加入了横竖屏显示的支持
//V1.5 20110730
//1,修改了B505液晶读颜色有误的bug.
//2,修改了快速IO及横竖屏的设置方式.
//V1.6 20111116
//1,加入对LGDP4535液晶的驱动支持
//V1.7 20120713
//1,增加LCD_RD_DATA函数
//2,增加对ILI9341的支持
//3,增加ILI9325的独立驱动代码
//4,增加LCD_Scan_Dir函数(慎重使用)
//6,另外修改了部分原来的函数,以适应9341的操作
//V1.8 20120905
//1,加入LCD重要参数设置结构体lcddev
//2,加入LCD_Display_Dir函数,支持在线横竖屏切换
//V1.9 20120911
//1,新增RM68042驱动（ID:6804），但是6804不支持横屏显示！！原因：改变扫描方式，
//导致6804坐标设置失效，试过很多方法都不行，暂时无解。
//V2.0 20120924
//在不硬件复位的情况下,ILI9341的ID读取会被误读成9300,修改LCD_Init,将无法识别
//的情况（读到ID为9300/非法ID）,强制指定驱动IC为ILI9341，执行9341的初始化。
//V2.1 20120930
//修正ILI9325读颜色的bug。
//V2.2 20121007
//修正LCD_Scan_Dir的bug。
//V2.3 20130120
//新增6804支持横屏显示
//V2.4 20131120
//1,新增NT35310（ID:5310）驱动器的支持
//2,新增LCD_Set_Window函数,用于设置窗口,对快速填充,比较有用,但是该函数在横屏时,不支持6804.
//V2.5 20140211
//1,新增NT35510（ID:5510）驱动器的支持
//V2.6 20140504
//1,新增ASCII 24*24字体的支持(更多字体用户可以自行添加)
//2,修改部分函数参数,以支持MDK -O2优化
//3,针对9341/35310/35510,写时间设置为最快,尽可能的提高速度
//4,去掉了SSD1289的支持,因为1289实在是太慢了,读周期要1us...简直奇葩.不适合F4使用
//5,修正68042及C505等IC的读颜色函数的bug.
//V2.7 20140710
//1,修正LCD_Color_Fill函数的一个bug.
//2,修正LCD_Scan_Dir函数的一个bug.
//V2.8 20140721
//1,解决MDK使用-O2优化时LCD_ReadPoint函数读点失效的问题.
//2,修正LCD_Scan_Dir横屏时设置的扫描方式显示不全的bug.
//V2.9 20141130
//1,新增对SSD1963 LCD的支持.
//2,新增LCD_SSD_BackLightSet函数
//3,取消ILI93XX的Rxx寄存器定义
//////////////////////////////////////////////////////////////////////////////////

#include "UI_engine.h"
#include "font.h"
// #include "usart.h"
// #include "delay.h"

//LCD的画笔颜色和背景色
ColorClass POINT_COLOR = 0x00000000; //画笔颜色
ColorClass BACK_COLOR = 0xFFFFFFFF;  //背景色
//画笔大小
unsigned char PenSize = 1;

//管理LCD重要参数
//默认为竖屏
_lcd_dev lcddev;

/* 	 
//写寄存器函数
//regval:寄存器值
void LCD_WR_REG(u16 regval)
{   
	LCD->LCD_REG=regval;//写入要写的寄存器序号	 
}
//写LCD数据
//data:要写入的值
void LCD_WR_DATA(u16 data)
{	 
	LCD->LCD_RAM=data;		 
}
//读LCD数据
//返回值:读到的值
u16 LCD_RD_DATA(void)
{
	vu16 ram;			//防止被优化
	ram=LCD->LCD_RAM;	
	return ram;	 
}					   
//写寄存器
//LCD_Reg:寄存器地址
//LCD_RegValue:要写入的数据
void LCD_WriteReg(u16 LCD_Reg,u16 LCD_RegValue)
{	
	LCD->LCD_REG = LCD_Reg;		//写入要写的寄存器序号	 
	LCD->LCD_RAM = LCD_RegValue;//写入数据	    		 
}	   
//读寄存器
//LCD_Reg:寄存器地址
//返回值:读到的数据
u16 LCD_ReadReg(u16 LCD_Reg)
{										   
	LCD_WR_REG(LCD_Reg);		//写入要读的寄存器序号
	delay_us(5);		  
	return LCD_RD_DATA();		//返回读到的值
}   
//开始写GRAM
void LCD_WriteRAM_Prepare(void)
{
 	LCD->LCD_REG=lcddev.wramcmd;	  
}	 
//LCD写GRAM
//RGB_Code:颜色值
void LCD_WriteRAM(u16 RGB_Code)
{							    
	LCD->LCD_RAM = RGB_Code;//写十六位GRAM
}
//从ILI93xx读出的数据为GBR格式，而我们写入的时候为RGB格式。
//通过该函数转换
//c:GBR格式的颜色值
//返回值：RGB格式的颜色值
u16 LCD_BGR2RGB(u16 c)
{
	u16  r,g,b,rgb;   
	b=(c>>0)&0x1f;
	g=(c>>5)&0x3f;
	r=(c>>11)&0x1f;	 
	rgb=(b<<11)+(g<<5)+(r<<0);		 
	return(rgb);
} 
 
//设置LCD的自动扫描方向
//注意:其他函数可能会受到此函数设置的影响(尤其是9341/6804这两个奇葩),
//所以,一般设置为L2R_U2D即可,如果设置为其他扫描方式,可能导致显示不正常.
//dir:0~7,代表8个方向(具体定义见lcd.h)
//9320/9325/9328/4531/4535/1505/b505/5408/9341/5310/5510/1963等IC已经实际测试
void LCD_Scan_Dir(u8 dir)
{
	u16 regval = 0;
	u16 dirreg = 0;
	u16 temp;
	if ((lcddev.dir == 1 && lcddev.id != 0X6804 && lcddev.id != 0X1963) || (lcddev.dir == 0 && lcddev.id == 0X1963)) //横屏时，对6804和1963不改变扫描方向！竖屏时1963改变方向
	{
		switch (dir) //方向转换
		{
		case 0:
			dir = 6;
			break;
		case 1:
			dir = 7;
			break;
		case 2:
			dir = 4;
			break;
		case 3:
			dir = 5;
			break;
		case 4:
			dir = 1;
			break;
		case 5:
			dir = 0;
			break;
		case 6:
			dir = 3;
			break;
		case 7:
			dir = 2;
			break;
		}
	}
	if (lcddev.id == 0x9341 || lcddev.id == 0X6804 || lcddev.id == 0X5310 || lcddev.id == 0X5510 || lcddev.id == 0X1963) //9341/6804/5310/5510/1963,特殊处理
	{
		switch (dir)
		{
		case L2R_U2D: //从左到右,从上到下
			regval |= (0 << 7) | (0 << 6) | (0 << 5);
			break;
		case L2R_D2U: //从左到右,从下到上
			regval |= (1 << 7) | (0 << 6) | (0 << 5);
			break;
		case R2L_U2D: //从右到左,从上到下
			regval |= (0 << 7) | (1 << 6) | (0 << 5);
			break;
		case R2L_D2U: //从右到左,从下到上
			regval |= (1 << 7) | (1 << 6) | (0 << 5);
			break;
		case U2D_L2R: //从上到下,从左到右
			regval |= (0 << 7) | (0 << 6) | (1 << 5);
			break;
		case U2D_R2L: //从上到下,从右到左
			regval |= (0 << 7) | (1 << 6) | (1 << 5);
			break;
		case D2U_L2R: //从下到上,从左到右
			regval |= (1 << 7) | (0 << 6) | (1 << 5);
			break;
		case D2U_R2L: //从下到上,从右到左
			regval |= (1 << 7) | (1 << 6) | (1 << 5);
			break;
		}
		if (lcddev.id == 0X5510)
			dirreg = 0X3600;
		else
			dirreg = 0X36;
		if ((lcddev.id != 0X5310) && (lcddev.id != 0X5510) && (lcddev.id != 0X1963))
			regval |= 0X08; //5310/5510/1963不需要BGR
		if (lcddev.id == 0X6804)
			regval |= 0x02; //6804的BIT6和9341的反了
		LCD_WriteReg(dirreg, regval);
		if (lcddev.id != 0X1963) //1963不做坐标处理
		{
			if (regval & 0X20)
			{
				if (lcddev.width < lcddev.height) //交换X,Y
				{
					temp = lcddev.width;
					lcddev.width = lcddev.height;
					lcddev.height = temp;
				}
			}
			else
			{
				if (lcddev.width > lcddev.height) //交换X,Y
				{
					temp = lcddev.width;
					lcddev.width = lcddev.height;
					lcddev.height = temp;
				}
			}
		}
		if (lcddev.id == 0X5510)
		{
			LCD_WR_REG(lcddev.setxcmd);
			LCD_WR_DATA(0);
			LCD_WR_REG(lcddev.setxcmd + 1);
			LCD_WR_DATA(0);
			LCD_WR_REG(lcddev.setxcmd + 2);
			LCD_WR_DATA((lcddev.width - 1) >> 8);
			LCD_WR_REG(lcddev.setxcmd + 3);
			LCD_WR_DATA((lcddev.width - 1) & 0XFF);
			LCD_WR_REG(lcddev.setycmd);
			LCD_WR_DATA(0);
			LCD_WR_REG(lcddev.setycmd + 1);
			LCD_WR_DATA(0);
			LCD_WR_REG(lcddev.setycmd + 2);
			LCD_WR_DATA((lcddev.height - 1) >> 8);
			LCD_WR_REG(lcddev.setycmd + 3);
			LCD_WR_DATA((lcddev.height - 1) & 0XFF);
		}
		else
		{
			LCD_WR_REG(lcddev.setxcmd);
			LCD_WR_DATA(0);
			LCD_WR_DATA(0);
			LCD_WR_DATA((lcddev.width - 1) >> 8);
			LCD_WR_DATA((lcddev.width - 1) & 0XFF);
			LCD_WR_REG(lcddev.setycmd);
			LCD_WR_DATA(0);
			LCD_WR_DATA(0);
			LCD_WR_DATA((lcddev.height - 1) >> 8);
			LCD_WR_DATA((lcddev.height - 1) & 0XFF);
		}
	}
	else
	{
		switch (dir)
		{
		case L2R_U2D: //从左到右,从上到下
			regval |= (1 << 5) | (1 << 4) | (0 << 3);
			break;
		case L2R_D2U: //从左到右,从下到上
			regval |= (0 << 5) | (1 << 4) | (0 << 3);
			break;
		case R2L_U2D: //从右到左,从上到下
			regval |= (1 << 5) | (0 << 4) | (0 << 3);
			break;
		case R2L_D2U: //从右到左,从下到上
			regval |= (0 << 5) | (0 << 4) | (0 << 3);
			break;
		case U2D_L2R: //从上到下,从左到右
			regval |= (1 << 5) | (1 << 4) | (1 << 3);
			break;
		case U2D_R2L: //从上到下,从右到左
			regval |= (1 << 5) | (0 << 4) | (1 << 3);
			break;
		case D2U_L2R: //从下到上,从左到右
			regval |= (0 << 5) | (1 << 4) | (1 << 3);
			break;
		case D2U_R2L: //从下到上,从右到左
			regval |= (0 << 5) | (0 << 4) | (1 << 3);
			break;
		}
		dirreg = 0X03;
		regval |= 1 << 12;
		LCD_WriteReg(dirreg, regval);
	}
} 

//设置窗口,并自动设置画点坐标到窗口左上角(sx,sy).
//sx,sy:窗口起始坐标(左上角)
//width,height:窗口宽度和高度,必须大于0!!
//窗体大小:width*height.
void LCD_Set_Window(u16 sx, u16 sy, u16 width, u16 height)
{
	u8 hsareg, heareg, vsareg, veareg;
	u16 hsaval, heaval, vsaval, veaval;
	u16 twidth, theight;
	twidth = sx + width - 1;
	theight = sy + height - 1;
	if (lcddev.id == 0X9341 || lcddev.id == 0X5310 || lcddev.id == 0X6804 || (lcddev.dir == 1 && lcddev.id == 0X1963))
	{
		LCD_WR_REG(lcddev.setxcmd);
		LCD_WR_DATA(sx >> 8);
		LCD_WR_DATA(sx & 0XFF);
		LCD_WR_DATA(twidth >> 8);
		LCD_WR_DATA(twidth & 0XFF);
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(sy >> 8);
		LCD_WR_DATA(sy & 0XFF);
		LCD_WR_DATA(theight >> 8);
		LCD_WR_DATA(theight & 0XFF);
	}
	else if (lcddev.id == 0X1963) //1963竖屏特殊处理
	{
		sx = lcddev.width - width - sx;
		height = sy + height - 1;
		LCD_WR_REG(lcddev.setxcmd);
		LCD_WR_DATA(sx >> 8);
		LCD_WR_DATA(sx & 0XFF);
		LCD_WR_DATA((sx + width - 1) >> 8);
		LCD_WR_DATA((sx + width - 1) & 0XFF);
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(sy >> 8);
		LCD_WR_DATA(sy & 0XFF);
		LCD_WR_DATA(height >> 8);
		LCD_WR_DATA(height & 0XFF);
	}
	else if (lcddev.id == 0X5510)
	{
		LCD_WR_REG(lcddev.setxcmd);
		LCD_WR_DATA(sx >> 8);
		LCD_WR_REG(lcddev.setxcmd + 1);
		LCD_WR_DATA(sx & 0XFF);
		LCD_WR_REG(lcddev.setxcmd + 2);
		LCD_WR_DATA(twidth >> 8);
		LCD_WR_REG(lcddev.setxcmd + 3);
		LCD_WR_DATA(twidth & 0XFF);
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(sy >> 8);
		LCD_WR_REG(lcddev.setycmd + 1);
		LCD_WR_DATA(sy & 0XFF);
		LCD_WR_REG(lcddev.setycmd + 2);
		LCD_WR_DATA(theight >> 8);
		LCD_WR_REG(lcddev.setycmd + 3);
		LCD_WR_DATA(theight & 0XFF);
	}
	else //其他驱动IC
	{
		if (lcddev.dir == 1) //横屏
		{
			//窗口值
			hsaval = sy;
			heaval = theight;
			vsaval = lcddev.width - twidth - 1;
			veaval = lcddev.width - sx - 1;
		}
		else
		{
			hsaval = sx;
			heaval = twidth;
			vsaval = sy;
			veaval = theight;
		}
		hsareg = 0X50;
		heareg = 0X51; //水平方向窗口寄存器
		vsareg = 0X52;
		veareg = 0X53; //垂直方向窗口寄存器
		//设置寄存器值
		LCD_WriteReg(hsareg, hsaval);
		LCD_WriteReg(heareg, heaval);
		LCD_WriteReg(vsareg, vsaval);
		LCD_WriteReg(veareg, veaval);
		LCD_SetCursor(sx, sy); //设置光标位置
	}
}

//SSD1963 背光设置
//pwm:背光等级,0~100.越大越亮.
void LCD_SSD_BackLightSet(u8 pwm)
{
	LCD_WR_REG(0xBE);		 //配置PWM输出
	LCD_WR_DATA(0x05);		 //1设置PWM频率
	LCD_WR_DATA(pwm * 2.55); //2设置PWM占空比
	LCD_WR_DATA(0x01);		 //3设置C
	LCD_WR_DATA(0xFF);		 //4设置D
	LCD_WR_DATA(0x00);		 //5设置E
	LCD_WR_DATA(0x00);		 //6设置F
}
*/

/*implant yes*/
//当mdk -O1时间优化时需要设置
//延时i
/* 
static void opt_delay(u8 i)
{
	while (i--);
}
 */
//读取个某点的颜色值
//x,y:坐标
//返回值:此点的颜色
ColorClass LCD_ReadPoint(u16 x, u16 y)
{
	// if (x >= lcddev.width || y >= lcddev.height)
	// 	return 0; //超过了范围,直接返回
	if (framebuffer_var_info.scale_flag) //由于f1c100s硬件原因, 硬件缩放颜色格式和使用的不同, 需转换
	{
		ColorClass color;
		if (lcddev.dir == DIS_DIR_90) //竖屏
			color = ((ColorClass *)lcddev.pixelDatas)[lcddev.width * (lcddev.height - 1 - x) + y];
		else if (lcddev.dir == DIS_DIR_180) //横
			color = ((ColorClass *)lcddev.pixelDatas)[lcddev.totalPixels - (x + y * lcddev.width)];
		else if (lcddev.dir == DIS_DIR_270) //竖
			color = ((ColorClass *)lcddev.pixelDatas)[lcddev.width * (x + 1) - 1 - y];
		else //横屏 0度
			color = ((ColorClass *)lcddev.pixelDatas)[x + y * lcddev.width];
		return color = ((color & 0X00FF0000) >> 16) | \
					   (color & 0XFF00FF00) | ((color & 0X000000FF) << 16);
	}
	else
	{
		if (lcddev.dir == DIS_DIR_90) //竖屏
		{
			return ((ColorClass *)lcddev.pixelDatas)[lcddev.width * (lcddev.height - 1 - x) + y];
		}
		else if (lcddev.dir == DIS_DIR_180) //横
		{
			return ((ColorClass *)lcddev.pixelDatas)[lcddev.totalPixels - (x + y * lcddev.width)];
		}
		else if (lcddev.dir == DIS_DIR_270) //竖
		{
			return ((ColorClass *)lcddev.pixelDatas)[lcddev.width * (x + 1) - 1 - y];
		}
		else //横屏 0度
		{
			return ((ColorClass *)lcddev.pixelDatas)[x + y * lcddev.width];
		}
	}
}
//读取背景某点的颜色值(需支持背景)
//x,y:坐标
//返回值:此点的颜色
ColorClass LCD_ReadBackPoint(u16 x, u16 y)
{
	if (framebuffer_var_info.scale_flag) //由于f1c100s硬件原因, 硬件缩放颜色格式和使用的不同, 需转换
	{
		ColorClass color;
		if (lcddev.dir == DIS_DIR_90) //竖屏
			color = ((ColorClass *)LCD_rectBackup.pixelDatas)[LCD_rectBackup.width * (LCD_rectBackup.height - 1 - x) + y];
		else if (lcddev.dir == DIS_DIR_180) //横
			color = ((ColorClass *)LCD_rectBackup.pixelDatas)[LCD_rectBackup.totalPixels - (x + y * LCD_rectBackup.width)];
		else if (lcddev.dir == DIS_DIR_270) //竖
			color = ((ColorClass *)LCD_rectBackup.pixelDatas)[LCD_rectBackup.width * (x + 1) - 1 - y];
		else //横屏 0度
			color = ((ColorClass *)LCD_rectBackup.pixelDatas)[x + y * LCD_rectBackup.width];
		return color = ((color & 0X00FF0000) >> 16) | \
					   (color & 0XFF00FF00) | ((color & 0X000000FF) << 16);
	}
	else
	{
		if (lcddev.dir == DIS_DIR_90) //竖屏
		{
			return ((ColorClass *)LCD_rectBackup.pixelDatas)[LCD_rectBackup.width * (LCD_rectBackup.height - 1 - x) + y];
		}
		else if (lcddev.dir == DIS_DIR_180) //横
		{
			return ((ColorClass *)LCD_rectBackup.pixelDatas)[LCD_rectBackup.totalPixels - (x + y * LCD_rectBackup.width)];
		}
		else if (lcddev.dir == DIS_DIR_270) //竖
		{
			return ((ColorClass *)LCD_rectBackup.pixelDatas)[LCD_rectBackup.width * (x + 1) - 1 - y];
		}
		else //横屏 0度
		{
			return ((ColorClass *)LCD_rectBackup.pixelDatas)[x + y * LCD_rectBackup.width];
		}
	}
}
//读取背景矩形到显示
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)
void LCD_FillBackToFront(u16 sx, u16 sy, u16 ex, u16 ey)
{
	u16 x, y;
	u32 i = 0;
	for (y = sy; y <= ey; y++)
	{
		for (x = sx; x <= ex; x++)
		{
			if (lcddev.dir == DIS_DIR_90) //竖屏
				i = LCD_rectBackup.width * (LCD_rectBackup.height - 1 - x) + y;
			else if (lcddev.dir == DIS_DIR_180) //横
				i = LCD_rectBackup.totalPixels - (x + y * LCD_rectBackup.width);
			else if (lcddev.dir == DIS_DIR_270) //竖
				i = LCD_rectBackup.width * (x + 1) - 1 - y;
			else //横屏 0度
				i = x + y * LCD_rectBackup.width;
			((ColorClass *)lcddev.pixelDatas)[i] = ((ColorClass *)LCD_rectBackup.pixelDatas)[i];
		}
	}
}
/*implant yes*/
//LCD开启显示
void LCD_DisplayOn(void)
{
	MTF_fb_set_backlight(&framebuffer_var_info, 100);
}
//LCD关闭显示
void LCD_DisplayOff(void)
{
	MTF_fb_set_backlight(&framebuffer_var_info, 0);
}
//LCD背光设置 0~100
void LCD_BackLightSet(u8 pwm)
{
	MTF_fb_set_backlight(&framebuffer_var_info, pwm);
}
//设置光标位置
//Xpos:横坐标
//Ypos:纵坐标
void LCD_SetCursor(u16 Xpos, u16 Ypos)
{
	return;
}
//画点
//x,y:坐标
//POINT_COLOR:此点的颜色
void LCD_DrawPoint(u16 x, u16 y)
{
	// if (x >= lcddev.width || y >= lcddev.height)
	// 	return 0; //超过了范围,直接返回
	if (framebuffer_var_info.scale_flag) //由于f1c100s硬件原因, 硬件缩放颜色格式和使用的不同, 需转换
	{
		ColorClass color;
		color = ((POINT_COLOR & 0X00FF0000) >> 16) | (POINT_COLOR & 0XFF00FF00) | ((POINT_COLOR & 0X000000FF) << 16);
		if (lcddev.dir == DIS_DIR_90) //竖屏
		{
			((ColorClass *)lcddev.pixelDatas)[lcddev.width * (lcddev.height - 1 - x) + y] = color;
		}
		else if (lcddev.dir == DIS_DIR_180) //横
		{
			((ColorClass *)lcddev.pixelDatas)[lcddev.totalPixels - 1 - (x + y * lcddev.width)] = color;
		}
		else if (lcddev.dir == DIS_DIR_270) //竖
		{
			((ColorClass *)lcddev.pixelDatas)[lcddev.width * (x + 1) - 1 - y] = color;
		}
		else //横屏 0度
		{
			((ColorClass *)lcddev.pixelDatas)[x + y * lcddev.width] = color;
		}
	}
	else
	{
		if (lcddev.dir == DIS_DIR_90) //竖屏
		{
			((ColorClass *)lcddev.pixelDatas)[lcddev.width * (lcddev.height - 1 - x) + y] = POINT_COLOR;
		}
		else if (lcddev.dir == DIS_DIR_180) //横
		{
			((ColorClass *)lcddev.pixelDatas)[lcddev.totalPixels - 1 - (x + y * lcddev.width)] = POINT_COLOR;
		}
		else if (lcddev.dir == DIS_DIR_270) //竖
		{
			((ColorClass *)lcddev.pixelDatas)[lcddev.width * (x + 1) - 1 - y] = POINT_COLOR;
		}
		else //横屏 0度
		{
			((ColorClass *)lcddev.pixelDatas)[x + y * lcddev.width] = POINT_COLOR;
		}
	}
}
//快速画点
//x,y:坐标
//color:颜色
void LCD_Fast_DrawPoint(u16 x, u16 y, ColorClass color)
{
	// if (x >= lcddev.width || y >= lcddev.height)
	// 	return 0; //超过了范围,直接返回
	if (framebuffer_var_info.scale_flag) //由于f1c100s硬件原因, 硬件缩放颜色格式和使用的不同, 需转换
		color = ((color & 0X00FF0000) >> 16) | (color & 0XFF00FF00) | ((color & 0X000000FF) << 16);

	if (lcddev.dir == DIS_DIR_90) //竖屏
	{
		((ColorClass *)lcddev.pixelDatas)[lcddev.width * (lcddev.height - 1 - x) + y] = color;
	}
	else if (lcddev.dir == DIS_DIR_180) //横
	{
		((ColorClass *)lcddev.pixelDatas)[lcddev.totalPixels - 1 - (x + y * lcddev.width)] = color;
	}
	else if (lcddev.dir == DIS_DIR_270) //竖
	{
		((ColorClass *)lcddev.pixelDatas)[lcddev.width * (x + 1) - 1 - y] = color;
	}
	else //横屏 0度
	{
		((ColorClass *)lcddev.pixelDatas)[x + y * lcddev.width] = color;
	}
}
//设置LCD显示方向
void LCD_Display_Dir(u8 dir)
{
	if (dir == DIS_DIR_90) //竖屏
	{
		lcddev.dir = DIS_DIR_90; //竖屏
		lcddev.width = (u16)framebuffer_var_info.yres_virtual;
		lcddev.height = (u16)framebuffer_var_info.xres_virtual;
	}
	else if (dir == DIS_DIR_180)
	{
		lcddev.dir = DIS_DIR_180; //横屏
		lcddev.width = (u16)framebuffer_var_info.xres_virtual;
		lcddev.height = (u16)framebuffer_var_info.yres_virtual;
	}
	else if (dir == DIS_DIR_270)
	{
		lcddev.dir = DIS_DIR_270; //竖屏
		lcddev.width = (u16)framebuffer_var_info.yres_virtual;
		lcddev.height = (u16)framebuffer_var_info.xres_virtual;
	}
	else //横屏 0度显示
	{
		lcddev.dir = DIS_DIR_0; //横屏
		lcddev.width = (u16)framebuffer_var_info.xres_virtual;
		lcddev.height = (u16)framebuffer_var_info.yres_virtual;
	}
	// LCD_Scan_Dir(DFT_SCAN_DIR);	//默认扫描方向
}
//初始化lcd
//该初始化函数可以初始化各种ILI93XX液晶,但是其他函数是基于ILI9320的!!!
//在其他型号的驱动芯片上没有测试!
render_dev_type *render_front = NULL, *render_back = NULL;
framebuffer_dev framebuffer_var_info; //屏幕信息
void LCD_Init(void)
{
	lcddev.id = 0XFFAA;
	// printf(" LCD ID:%x\r\n",lcddev.id); //打印LCD ID

	MTF_fb_init(&framebuffer_var_info);
	render_front = MTF_fb_render_create(&framebuffer_var_info, framebuffer_var_info.xres, framebuffer_var_info.yres);
	render_back = MTF_fb_render_create(&framebuffer_var_info, framebuffer_var_info.xres, framebuffer_var_info.yres);

	lcddev.pixelDatas = MTF_fb_get_dis_mem(render_front);
	lcddev.totalPixels = render_front->width * render_front->height;
	LCD_Display_Dir(DIS_DIR_0); //横屏
	LCD_Clear(BLACK); //清屏
	LCD_Exec(); //更新显示
	//MTF_fb_set_backlight(&framebuffer_var_info, 100); //点亮背光
}
//清屏函数
//color:要清屏的填充色
void LCD_Clear(ColorClass color)
{
	if (framebuffer_var_info.scale_flag) //由于f1c100s硬件原因, 硬件缩放颜色格式和使用的不同, 需转换
		color = ((color & 0X00FF0000) >> 16) | (color & 0XFF00FF00) | ((color & 0X000000FF) << 16);
	
	for (int i = 0; i < lcddev.totalPixels; i++)
	{
		((ColorClass *)lcddev.pixelDatas)[i] = color;
	}
}
//在指定区域内填充单个颜色
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)
//color:要填充的颜色
void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, ColorClass color)
{
	u16 i, j;
	u16 xlen = 0;
	ColorClass *ptr = 0;

	xlen = ex - sx + 1;

	if (framebuffer_var_info.scale_flag) //由于f1c100s硬件原因, 硬件缩放颜色格式和使用的不同, 需转换
		color = ((color & 0X00FF0000) >> 16) | (color & 0XFF00FF00) | ((color & 0X000000FF) << 16);

	if (lcddev.dir == DIS_DIR_90) //竖屏
	{
		for (i = sy; i <= ey; i++)
		{
			for (j = 0; j < xlen; j++)
			{
				((ColorClass *)lcddev.pixelDatas)[lcddev.width * (lcddev.height - 1 - sx - j) + i] = color;
			}
		}
	}
	else if (lcddev.dir == DIS_DIR_180) //横
	{
		for (i = sy; i <= ey; i++)
		{
			ptr = &((ColorClass *)lcddev.pixelDatas)[(lcddev.height - i) * lcddev.width - sx - 1];
			for (j = 0; j < xlen; j++)
			{
				*ptr = color; //写入数据
				ptr--;
			}
		}
	}
	else if (lcddev.dir == DIS_DIR_270) //竖
	{
		for (i = sy; i <= ey; i++)
		{
			for (j = 0; j < xlen; j++)
			{
				((ColorClass *)lcddev.pixelDatas)[lcddev.width * (sx + j + 1) - 1 - i] = color;
			}
		}
	}
	else //横屏 0度
	{
		for (i = sy; i <= ey; i++)
		{
			ptr = &((ColorClass *)lcddev.pixelDatas)[sx + i * lcddev.width];
			for (j = 0; j < xlen; j++)
			{
				*ptr = color; //写入数据
				ptr++;
			}
		}
	}
}

//在指定区域内填充指定颜色块
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)
//color:要填充的颜色
void LCD_Color_Fill(u16 sx, u16 sy, u16 ex, u16 ey, ColorClass *color)
{
	u16 i, j;
	u16 xlen = 0;
	ColorClass *ptr = 0;
	u16 y;

	xlen = ex - sx + 1;

	if (framebuffer_var_info.scale_flag) //由于f1c100s硬件原因, 硬件缩放颜色格式和使用的不同, 需转换
	{
		ColorClass colorTemp;
		if (lcddev.dir == DIS_DIR_90) //竖屏
		{
			for (i = sy, y = 0; i <= ey; i++)
			{
				for (j = 0; j < xlen; j++)
				{
					colorTemp = color[y * xlen + j];
					colorTemp = ((colorTemp & 0X00FF0000) >> 16) | (colorTemp & 0XFF00FF00) | ((colorTemp & 0X000000FF) << 16);
					((ColorClass *)lcddev.pixelDatas)[lcddev.width * (lcddev.height - 1 - sx - j) + i] = colorTemp;
				}
				y++;
			}
		}
		else if (lcddev.dir == DIS_DIR_180) //横
		{
			for (i = sy, y = 0; i <= ey; i++)
			{
				ptr = &((ColorClass *)lcddev.pixelDatas)[(lcddev.height - i) * lcddev.width - sx - 1];
				for (j = 0; j < xlen; j++)
				{
					colorTemp = color[y * xlen + j];
					colorTemp = ((colorTemp & 0X00FF0000) >> 16) | (colorTemp & 0XFF00FF00) | ((colorTemp & 0X000000FF) << 16);
					*ptr = colorTemp; //写入数据
					ptr--;
				}
				y++;
			}
		}
		else if (lcddev.dir == DIS_DIR_270) //竖
		{
			for (i = sy, y = 0; i <= ey; i++)
			{
				for (j = 0; j < xlen; j++)
				{
					colorTemp = color[y * xlen + j];
					colorTemp = ((colorTemp & 0X00FF0000) >> 16) | (colorTemp & 0XFF00FF00) | ((colorTemp & 0X000000FF) << 16);
					((ColorClass *)lcddev.pixelDatas)[lcddev.width * (sx + j + 1) - 1 - i] = colorTemp;
				}
				y++;
			}
		}
		else //横屏 0度
		{
			for (i = sy, y = 0; i <= ey; i++)
			{
				ptr = &((ColorClass *)lcddev.pixelDatas)[sx + i * lcddev.width];
				for (j = 0; j < xlen; j++)
				{
					colorTemp = color[y * xlen + j];
					colorTemp = ((colorTemp & 0X00FF0000) >> 16) | (colorTemp & 0XFF00FF00) | ((colorTemp & 0X000000FF) << 16);
					*ptr = colorTemp; //写入数据
					ptr++;
				}
				y++;
			}
		}
	}
	else
	{
		if (lcddev.dir == DIS_DIR_90) //竖屏
		{
			for (i = sy, y = 0; i <= ey; i++)
			{
				for (j = 0; j < xlen; j++)
				{
					((ColorClass *)lcddev.pixelDatas)[lcddev.width * (lcddev.height - 1 - sx - j) + i] = color[y * xlen + j];
				}
				y++;
			}
		}
		else if (lcddev.dir == DIS_DIR_180) //横
		{
			for (i = sy, y = 0; i <= ey; i++)
			{
				ptr = &((ColorClass *)lcddev.pixelDatas)[(lcddev.height - i) * lcddev.width - sx - 1];
				for (j = 0; j < xlen; j++)
				{
					*ptr = color[y * xlen + j]; //写入数据
					ptr--;
				}
				y++;
			}
		}
		else if (lcddev.dir == DIS_DIR_270) //竖
		{
			for (i = sy, y = 0; i <= ey; i++)
			{
				for (j = 0; j < xlen; j++)
				{
					((ColorClass *)lcddev.pixelDatas)[lcddev.width * (sx + j + 1) - 1 - i] = color[y * xlen + j];
				}
				y++;
			}
		}
		else //横屏 0度
		{
			for (i = sy, y = 0; i <= ey; i++)
			{
				ptr = &((ColorClass *)lcddev.pixelDatas)[sx + i * lcddev.width];
				for (j = 0; j < xlen; j++)
				{
					*ptr = color[y * xlen + j]; //写入数据
					ptr++;
				}
				y++;
			}
		}
	}
}

/****************************************
Base graph draw and display ASCII char 
 ***************************************/
//画线1
//x1,y1:起点坐标
//x2,y2:终点坐标
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	delta_x = x2 - x1; //计算坐标增量
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;
	if (delta_x > 0)
		incx = 1; //设置单步方向
	else if (delta_x == 0)
		incx = 0; //垂直线
	else
	{
		incx = -1;
		delta_x = -delta_x;
	}
	if (delta_y > 0)
		incy = 1;
	else if (delta_y == 0)
		incy = 0; //水平线
	else
	{
		incy = -1;
		delta_y = -delta_y;
	}
	if (delta_x > delta_y)
		distance = delta_x; //选取基本增量坐标轴
	else
		distance = delta_y;
	for (t = 0; t <= distance + 1; t++) //画线输出
	{
		LCD_DrawPoint(uRow, uCol); //画点
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance)
		{
			xerr -= distance;
			uRow += incx;
		}
		if (yerr > distance)
		{
			yerr -= distance;
			uCol += incy;
		}
	}
}

//画线2
//x0 y0:起点坐标,,,,x1: 终点坐标
void LCD_HLine(u16 x0, u16 y0, u16 x1, ColorClass color)
{
    u8  temp;
    if(x0>x1)               // 对x0、x1大小进行排列，以便画图
    {
        temp = x1;
        x1 = x0;
        x0 = temp;
    }
    do
    {
        LCD_Fast_DrawPoint(x0, y0, color);   // 逐点显示，描出垂直线
        x0++;
    }
    while(x1>=x0);
}

//x0 y0:起点坐标,,,,y1: 终点坐标
void LCD_RLine(u16 x0, u16 y0, u8 y1, ColorClass color)
{
    u8  temp;
    if(y0>y1)       // 对y0、y1大小进行排列，以便画图
    {
        temp = y1;
        y1 = y0;
        y0 = temp;
    }
    do
    {
        LCD_Fast_DrawPoint(x0, y0, color);   // 逐点显示，描出垂直线
        y0++;
    }
    while(y1>=y0);
}

void LCD_DrawLine2(u32 x0, u32 y0, u32 x1, u32 y1, ColorClass color)
{  u32   dx;						// 直线x轴差值变量
   u32   dy;          			// 直线y轴差值变量
   u8    dx_sym;					// x轴增长方向，为-1时减值方向，为1时增值方向
   u8    dy_sym;					// y轴增长方向，为-1时减值方向，为1时增值方向
   u32   dx_x2;					// dx*2值变量，用于加快运算速度
   u32   dy_x2;					// dy*2值变量，用于加快运算速度
   u32   di;						// 决策变量
   
   
   dx = x1-x0;						// 求取两点之间的差值
   dy = y1-y0;
   
   /* 判断增长方向，或是否为水平线、垂直线、点 */
   if(dx>0)							// 判断x轴方向
   {  dx_sym = 1;					// dx>0，设置dx_sym=1
   }
   else
   {  if(dx<0)
      {  dx_sym = -1;				// dx<0，设置dx_sym=-1
      }
      else
      {  // dx==0，画垂直线，或一点
         LCD_RLine(x0, y0, y1, color);
      	 return;
      }
   }
   
   if(dy>0)							// 判断y轴方向
   {  dy_sym = 1;					// dy>0，设置dy_sym=1
   }
   else
   {  if(dy<0)
      {  dy_sym = -1;				// dy<0，设置dy_sym=-1
      }
      else
      {  // dy==0，画水平线，或一点
         LCD_HLine(x0, y0, x1, color);
      	 return;
      }
   }
    
   /* 将dx、dy取绝对值 */
   dx = dx_sym * dx;
   dy = dy_sym * dy;
 
   /* 计算2倍的dx及dy值 */
   dx_x2 = dx*2;
   dy_x2 = dy*2;
   
   /* 使用Bresenham法进行画直线 */
   if(dx>=dy)						// 对于dx>=dy，则使用x轴为基准
   {  di = dy_x2 - dx;
      while(x0!=x1)
      {  LCD_Fast_DrawPoint(x0, y0, color);
         x0 += dx_sym;
         if(di<0)
         {  di += dy_x2;			// 计算出下一步的决策值
         }
         else
         {  di += dy_x2 - dx_x2;
            y0 += dy_sym;
         }
      }
      LCD_Fast_DrawPoint(x0, y0, color);		// 显示最后一点
   }
   else								// 对于dx<dy，则使用y轴为基准
   {  di = dx_x2 - dy;
      while(y0!=y1)
      {  LCD_Fast_DrawPoint(x0, y0, color);
         y0 += dy_sym;
         if(di<0)
         {  di += dx_x2;
         }
         else
         {  di += dx_x2 - dy_x2;
            x0 += dx_sym;
         }
      }
      LCD_Fast_DrawPoint(x0, y0, color);		// 显示最后一点
   } 
  
}

//画矩形
//(x1,y1),(x2,y2):矩形的对角坐标
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1, y1, x2, y1);
	LCD_DrawLine(x1, y1, x1, y2);
	LCD_DrawLine(x1, y2, x2, y2);
	LCD_DrawLine(x2, y1, x2, y2);
}
//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void LCD_Draw_Circle(u16 x0, u16 y0, u8 r)
{
	int a, b;
	int di;
	a = 0;
	b = r;
	di = 3 - (r << 1); //判断下个点位置的标志
	while (a <= b)
	{
		LCD_DrawPoint(x0 + a, y0 - b); //5
		LCD_DrawPoint(x0 + b, y0 - a); //0
		LCD_DrawPoint(x0 + b, y0 + a); //4
		LCD_DrawPoint(x0 + a, y0 + b); //6
		LCD_DrawPoint(x0 - a, y0 + b); //1
		LCD_DrawPoint(x0 - b, y0 + a);
		LCD_DrawPoint(x0 - a, y0 - b); //2
		LCD_DrawPoint(x0 - b, y0 - a); //7
		a++;
		//使用Bresenham算法画圆
		if (di < 0)
			di += 4 * a + 6;
		else
		{
			di += 10 + 4 * (a - b);
			b--;
		}
	}
}
//在指定位置显示一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//size:字体大小 12/16/24
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChar(u16 x, u16 y, u8 num, u8 size, u8 mode)
{
	u8 temp, t1, t;
	u16 y0 = y;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); //得到字体一个字符对应点阵集所占的字节数
	num = num - ' ';										   //得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
	for (t = 0; t < csize; t++)
	{
		if (size == 12)
			temp = asc2_1206[num][t]; //调用1206字体
		else if (size == 16)
			temp = asc2_1608[num][t]; //调用1608字体
		else if (size == 24)
			temp = asc2_2412[num][t]; //调用2412字体
		else
			return; //没有的字库
		for (t1 = 0; t1 < 8; t1++)
		{
			if (temp & 0x80)
				LCD_Fast_DrawPoint(x, y, POINT_COLOR);
			else if (mode == 0)
				LCD_Fast_DrawPoint(x, y, BACK_COLOR);
			else if (mode == 1) //用于透明更新, 需搭配更新背景, 不更新可不用
				LCD_Fast_DrawPoint(x, y, LCD_ReadBackPoint(x, y));
			temp <<= 1;
			y++;
			if (y >= lcddev.height)
				return; //超区域了
			if ((y - y0) == size)
			{
				y = y0;
				x++;
				if (x >= lcddev.width)
					return; //超区域了
				break;
			}
		}
	}
}
//m^n函数
//返回值:m^n次方.
u32 LCD_Pow(u8 m, u8 n)
{
	u32 result = 1;
	while (n--)
		result *= m;
	return result;
}
//显示数字,高位为0,则不显示
//x,y :起点坐标
//len :数字的位数
//size:字体大小
//color:颜色
//num:数值(0~4294967295);
void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size)
{
	u8 t, temp;
	u8 enshow = 0;
	for (t = 0; t < len; t++)
	{
		temp = (num / LCD_Pow(10, len - t - 1)) % 10;
		if (enshow == 0 && t < (len - 1))
		{
			if (temp == 0)
			{
				LCD_ShowChar(x + (size / 2) * t, y, ' ', size, 0);
				continue;
			}
			else
				enshow = 1;
		}
		LCD_ShowChar(x + (size / 2) * t, y, temp + '0', size, 0);
	}
}
//显示数字,高位为0,还是显示
//x,y:起点坐标
//num:数值(0~999999999);
//len:长度(即要显示的位数)
//size:字体大小
//mode:
//[7]:0,不填充;1,填充0.
//[6:1]:保留
//[0]:0,非叠加显示;1,叠加显示.
void LCD_ShowxNum(u16 x, u16 y, u32 num, u8 len, u8 size, u8 mode)
{
	u8 t, temp;
	u8 enshow = 0;
	for (t = 0; t < len; t++)
	{
		temp = (num / LCD_Pow(10, len - t - 1)) % 10;
		if (enshow == 0 && t < (len - 1))
		{
			if (temp == 0)
			{
				if (mode & 0X80)
					LCD_ShowChar(x + (size / 2) * t, y, '0', size, mode & 0X01);
				else
					LCD_ShowChar(x + (size / 2) * t, y, ' ', size, mode & 0X01);
				continue;
			}
			else
				enshow = 1;
		}
		LCD_ShowChar(x + (size / 2) * t, y, temp + '0', size, mode & 0X01);
	}
}
//显示字符串
//x,y:起点坐标
//width,height:区域大小
//size:字体大小
//*p:字符串起始地址
void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, u8 *p)
{
	u8 x0 = x;
	width += x;
	height += y;
	while ((*p <= '~') && (*p >= ' ')) //判断是不是非法字符!
	{
		if (x >= width)
		{
			x = x0;
			y += size;
		}
		if (y >= height)
			break; //退出
		LCD_ShowChar(x, y, *p, size, 0);
		x += size / 2;
		p++;
	}
}

/**********************************************************
 * GUI test
 * ******************************************************/
void GUI_test(void)
{
	LCD_Init();
    LCD_BackLightSet(50);
	LCD_ShowString(100,300,200,16,24,(u8 *)"Lion");	
	LCD_ShowString(100,330,200,16,24,(u8 *)"TEST");	
	LCD_ShowString(100,360,200,16,24,(u8 *)"Aysi");
	LCD_ShowString(100,390,200,16,24,(u8 *)"*^__^*");
    LCD_HLine(0, 0, 50, BLUE);
    LCD_RLine(0, 0, 50, BLUE);
    LCD_HLine(20, 100, 150, GREEN);
    LCD_RLine(30, 80, 120, GREEN);
	LCD_Fill(30, 30, 90, 50, LIGHT_RED);
	LCD_Fill(90, 30, 150, 50, LIGHT_ORANGE);
	LCD_Fill(150, 30, 210, 50, LIGHT_YELLOW);
	LCD_Fill(210, 30, 270, 50, LIGHT_GREEN);
	LCD_Fill(270, 30, 330, 50, SUN_YELLOW);
	LCD_Fill(150, 60, 210, 80, RED);
    LCD_Fill(210, 60, 270, 80, GREEN);
    LCD_Fill(270, 60, 330, 80, BLUE);
	LCD_Exec();
	while(1);
}
