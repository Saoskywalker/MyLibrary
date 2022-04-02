// #include "fontupd.h"
// #include "ROM_port.h"
#include "UI_engine.h"
#include "text.h"
#include "MTF_io.h"
#include "file_operate_hal.h" 
#include "Sagittarius_global.h"

static mFILE *ffont8, *ffont12, *ffont16, *ffont24, *ffont32, *ffontASCII8, *ffontASCII32;
static u16 FontStute = 0;

static void FontInit(u8 size)
{
	char path[64]; //目录最长64byte

	memset(path, 0, sizeof(path));
	strcat(path, dirPath);
	if(size>100) //用于ASCII字库
	{
		size -= 100; //还原偏移
		switch (size)
		{
		case 8:
			//第一次检查存在
			strcat(path, "ASCII8.FON");
			ffontASCII8 = MTF_open(path, "rb");
			if(ffontASCII8!=NULL)
			{
				FontStute |= 1<<4; //此字库OK
			}
			break;
		case 32:
			//第一次检查存在
			strcat(path, "ASCII32.FON");
			ffontASCII32 = MTF_open(path, "rb");
			if(ffontASCII32!=NULL)
			{
				FontStute |= 1<<5; //此字库OK
			}
			break;
		default:
			break;
		}
	}
	else
	{
		switch (size)
		{
		case 8:
			//第一次检查存在
			strcat(path, "GBK8.FON");
			ffont8 = MTF_open(path, "rb");
			if(ffont8!=NULL)
			{
				FontStute |= 1<<3; //此字库OK
			}
			break;
		case 12:
			//第一次检查存在
			strcat(path, "GBK12.FON");
			ffont12 = MTF_open(path, "rb");
			if(ffont12!=NULL)
			{
				FontStute |= 1<<0; //此字库OK
			}
			else
			{ //先复制,,后再次检查字库
				// res = mf_copy("./SYSTEM/FONT/GBK12.FON", "./GBK12.FON", 1);
				// if(res==0)
				// 	ffont12 = MTF_open("./GBK12.FON", "rb");
				// if(ffont12!=NULL)
				// 	FontStute |= 1<<0;
			}		
			break;
		case 16:
			//第一次检查存在
			strcat(path, "GBK16.FON");
			ffont16 = MTF_open(path, "rb");
			if(ffont16!=NULL)
			{
				FontStute |= 1<<1; //此字库OK
			}
			else
			{ //先复制,,后再次检查字库
				// res = mf_copy("./SYSTEM/FONT/GBK16.FON", "./GBK16.FON", 1);
				// if(res==0)
				// 	ffont16 = MTF_open("./GBK16.FON", "rb");
				// if(ffont16!=NULL)
				// 	FontStute |= 1<<1;
			}	
			break;
		case 24:
			//第一次检查存在
			strcat(path, "GBK24.FON");
			ffont24 = MTF_open(path, "rb");
			if(ffont24!=NULL)
			{
				FontStute |= 1<<2; //此字库OK
			}
			else
			{ //先复制,,后再次检查字库
				// res = mf_copy("./SYSTEM/FONT/GBK24.FON", "./GBK24.FON", 1);
				// if(res==0)
				// 	ffont24 = MTF_open("./GBK24.FON", "rb");
				// if(ffont24!=NULL)
				// 	FontStute |= 1<<2;
			}	
			break;
		case 32:
			//第一次检查存在
			strcat(path, "GBK32.FON");
			ffont32 = MTF_open(path, "rb");
			if(ffont32!=NULL)
			{
				FontStute |= 1<<6; //此字库OK
			}
			else
			{ //先复制,,后再次检查字库
				// res = mf_copy("./SYSTEM/FONT/GBK32.FON", "./GBK32.FON", 1);
				// if(res==0)
				// 	ffont32 = MTF_open("1:/GBK32.FON", "rb");
				// if(ffont32!=NULL)
				// 	FontStute |= 1<<6;
			}	
			break;
		default:
			break;
		}
	}
}

//code 字符指针开始
//从字库中查找出字模
//code 字符串的开始地址,GBK码
//mat  数据存放地址 (size/8+((size%8)?1:0))*(size) bytes大小
//size:字体大小
void Get_HzMat(unsigned char *code, unsigned char *mat, u8 size)
{
	unsigned char qh, ql;
	unsigned char i;
	unsigned long foffset;
	u8 csize = 0;

	if(size>100) //用于ascii字库
	{
		size -= 100; //减去偏移
		csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size); //得到字体一个字符对应点阵集所占的字节数
		foffset = *code * csize; //得到字库中的字节偏移量
		switch (size)
		{
		case 8:
			if (FontStute & (1 << 4))
			{ //已初始化
				MTF_seek(ffontASCII8, foffset, SEEK_SET);
				MTF_read(mat, 1, csize, ffontASCII8);
			}
			else
			{ //初始化
				FontInit(size+100);
				if (FontStute & (1 << 4))
				{ //初始化失败
					MTF_seek(ffontASCII8, foffset, SEEK_SET);
					MTF_read(mat, 1, csize, ffontASCII8);
				}
				else 
				{ //无字库, 用填充处理
					for (i = 0; i < csize; i++)
						*mat++ = 0x00; //填充满格
					return;			   //结束访问
				}
			}
			break;
		case 32:
			if (FontStute & (1 << 5))
			{ //已初始化
				MTF_seek(ffontASCII32, foffset, SEEK_SET);
				MTF_read(mat, 1, csize, ffontASCII32);
			}
			else
			{ //初始化
				FontInit(size+100);
				if (FontStute & (1 << 5))
				{ //初始化失败
					MTF_seek(ffontASCII32, foffset, SEEK_SET);
					MTF_read(mat, 1, csize, ffontASCII32);
				}
				else 
				{ //无字库, 用填充处理
					for (i = 0; i < csize; i++)
						*mat++ = 0x00; //填充满格
					return;			   //结束访问
				}
			}
			break;
		default:
			for (i = 0; i < csize; i++)
				*mat++ = 0x00; //填充满格
			break;
		}
		return; //结束
	}
	else
	{
		csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size); //得到字体一个字符对应点阵集所占的字节数
	}

	// if (ftinfo.fontok != 0XAA) //检查字库和初始化
	// {
	// 	if (font_init()) 
	// 	{ //初始化失败
	// 		for (i = 0; i < csize; i++)
	// 			*mat++ = 0x00; //填充满格
	// 		return;			   //结束访问
	// 	}
	// }
	
	qh = *code;
	ql = *(++code);
	if (qh < 0x81 || ql < 0x40 || ql == 0xff || qh == 0xff) //非 常用汉字
	{
		for (i = 0; i < csize; i++)
			*mat++ = 0x00; //填充满格
		return;			   //结束访问
	}
	if (ql < 0x7f)
		ql -= 0x40; //注意!
	else
		ql -= 0x41;
	qh -= 0x81;
	foffset = ((unsigned long)190 * qh + ql) * csize; //得到字库中的字节偏移量

	switch (size)
	{
	case 8:
		if (FontStute & (1 << 3))
		{ //已初始化
			MTF_seek(ffont8, foffset, SEEK_SET);
			MTF_read(mat, 1, csize, ffont8);
		}
		else
		{ //初始化
			FontInit(size);
			if (FontStute & (1 << 3))
			{ //初始化失败
				MTF_seek(ffont8, foffset, SEEK_SET);
				MTF_read(mat, 1, csize, ffont8);
			}
			else 
			{ //无字库, 用填充处理
				for (i = 0; i < csize; i++)
					*mat++ = 0x00; //填充满格
				return;			   //结束访问
			}
		}
		break;
	case 12:
		if (FontStute & (1 << 0))
		{ //已初始化
			MTF_seek(ffont12, foffset, SEEK_SET);
			MTF_read(mat, 1, csize, ffont12);
		}
		else
		{ //初始化
			FontInit(size);
			if (FontStute & (1 << 0))
			{ //初始化失败
				MTF_seek(ffont12, foffset, SEEK_SET);
				MTF_read(mat, 1, csize, ffont12);
			}
			else 
			{ //无字库, 用填充处理
				for (i = 0; i < csize; i++)
					*mat++ = 0x00; //填充满格
				return;			   //结束访问
			}
		}
		//注意包含回w25qxx.h
		// MTF_ROM_read(mat, foffset + ftinfo.f12addr, csize);
		break;
	case 16:
		if (FontStute & (1 << 1))
		{ //已初始化
			MTF_seek(ffont16, foffset, SEEK_SET);
			MTF_read(mat, 1, csize, ffont16);
		}
		else
		{ //初始化
			FontInit(size);
			if (FontStute & (1 << 1))
			{ //初始化失败
				MTF_seek(ffont16, foffset, SEEK_SET);
				MTF_read(mat, 1, csize, ffont16);
			}
			else 
			{ //无字库, 用填充处理
				for (i = 0; i < csize; i++)
					*mat++ = 0x00; //填充满格
				return;			   //结束访问
			}
		}
		// MTF_ROM_read(mat, foffset + ftinfo.f16addr, csize);
		break;
	case 24:
		if (FontStute & (1 << 2))
		{ //已初始化
			MTF_seek(ffont24, foffset, SEEK_SET);
			MTF_read(mat, 1, csize, ffont24);
		}
		else
		{ //初始化
			FontInit(size);
			if (FontStute & (1 << 2))
			{ //初始化失败
				MTF_seek(ffont24, foffset, SEEK_SET);
				MTF_read(mat, 1, csize, ffont24);
			}
			else 
			{ //无字库, 用填充处理
				for (i = 0; i < csize; i++)
					*mat++ = 0x00; //填充满格
				return;			   //结束访问
			}
		}
		// MTF_ROM_read(mat, foffset + ftinfo.f24addr, csize);
		break;
	case 32:
		if (FontStute & (1 << 6))
		{ //已初始化
			MTF_seek(ffont32, foffset, SEEK_SET);
			MTF_read(mat, 1, csize, ffont32);
		}
		else
		{ //初始化
			FontInit(size);
			if (FontStute & (1 << 6))
			{ //初始化失败
				MTF_seek(ffont32, foffset, SEEK_SET);
				MTF_read(mat, 1, csize, ffont32);
			}
			else 
			{ //无字库, 用填充处理
				for (i = 0; i < csize; i++)
					*mat++ = 0x00; //填充满格
				return;			   //结束访问
			}
		}
		// MTF_ROM_read(mat, foffset + ftinfo.f32addr, csize);
		break;
	default:
		for (i = 0; i < csize; i++)
			*mat++ = 0x00; //填充满格
		break;
	}
}

//显示一个指定大小的汉字
//x,y :汉字的坐标
//font:汉字GBK码
//size:字体大小
//mode:0,正常显示,1,叠加显示
void Show_Font(u16 x, u16 y, u8 *font, u8 size, u8 mode)
{
	u8 temp, t, t1;
	u16 y0 = y;
	u8 dzk[72]; //noet:字节缓存
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size); //得到字体一个字符对应点阵集所占的字节数

	if (size != 12 && size != 16 && size != 24)
		return;					//不支持的size
	Get_HzMat(font, dzk, size); //得到相应大小的点阵数据
	for (t = 0; t < csize; t++)
	{
		temp = dzk[t]; //得到点阵数据
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
			if ((y - y0) == size)
			{
				y = y0;
				x++;
				break;
			}
		}
	}
}

//在指定位置开始显示一个字符串到屏幕
//支持自动换行
//(x,y):起始坐标
//width,height:区域
//str  :字符串
//size :字体大小
//mode:0,非叠加方式;1,叠加方式
void Show_Str(u16 x, u16 y, u16 width, u16 height, u8 *str, u8 size, u8 mode)
{
	u16 x0 = x;
	u16 y0 = y;
	u8 bHz = 0;		  //字符或者中文

	while (*str != 0) //数据未结束
	{
		if (!bHz)
		{
			if (*str > 0x80)
				bHz = 1; //中文
			else		 //字符
			{
				if (x > (x0 + width - size / 2)) //换行
				{
					y += size;
					x = x0;
				}
				if (y > (y0 + height - size))
					break;		//越界返回
				if (*str == 13) //换行符号
				{
					y += size;
					x = x0;
					str++;
				}
				else
					LCD_ShowChar(x, y, *str, size, mode); //有效部分写入
				str++;
				x += size / 2; //字符,为全字的一半
			}
		}
		else //中文
		{
			bHz = 0;					 //有汉字库
			if (x > (x0 + width - size)) //换行
			{
				y += size;
				x = x0;
			}
			if (y > (y0 + height - size))
				break;						  //越界返回
			Show_Font(x, y, str, size, mode); //显示这个汉字,空心显示
			str += 2;
			x += size; //下一个汉字偏移
		}
	}
}

//在指定位置开始显示一个字符串到屏幕(自动计算长宽)
//支持自动换行
//(x,y):起始坐标
//width,height:区域
//str  :字符串
//size :字体大小
//mode:0,非叠加方式;1,叠加方式
void Show_Str2(u16 x, u16 y, u8 *str, u8 size, u8 mode)
{
	u16 width, height;
	u8 i = 0, j = 1, *cn = str;
	u16 x0 = x;
	u16 y0 = y;
	u8 bHz = 0;		  //字符或者中文

	//统计区域大小
	while (*cn != 0) //数据未结束
	{
		if (*cn == 13) //换行符号
		{
			j++;
			cn++;
			continue;
		}	
		if (*cn > 0x80)
		{
			i += 2;
			cn += 2; //GBK字符
		}
		else
		{
			cn++; //ASCII字符
			i++;
		}
	}
	width = size/2*i;
	height = size*j;

	while (*str != 0) //数据未结束
	{
		if (!bHz)
		{
			if (*str > 0x80)
				bHz = 1; //中文
			else		 //字符
			{
				if (x > (x0 + width - size / 2)) //换行
				{
					y += size;
					x = x0;
				}
				if (y > (y0 + height - size))
					break;		//越界返回
				if (*str == 13) //换行符号
				{
					y += size;
					x = x0;
					str++;
				}
				else
					LCD_ShowChar(x, y, *str, size, mode); //有效部分写入
				str++;
				x += size / 2; //字符,为全字的一半
			}
		}
		else //中文
		{
			bHz = 0;					 //有汉字库
			if (x > (x0 + width - size)) //换行
			{
				y += size;
				x = x0;
			}
			if (y > (y0 + height - size))
				break;						  //越界返回
			Show_Font(x, y, str, size, mode); //显示这个汉字,空心显示
			str += 2;
			x += size; //下一个汉字偏移
		}
	}
}

//在指定宽度的中间显示字符串
//如果字符长度超过了len,则用Show_Str显示
//len:指定要显示的宽度
void Show_Str_Mid(u16 x, u16 y, u8 *str, u8 size, u8 len)
{
	u16 strlenth = 0;

	strlenth = strlen((const char *)str);
	strlenth *= size / 2;
	if (strlenth > len)
	{
		Show_Str(x, y, lcddev.width, lcddev.height, str, size, 1);
	}
	else
	{
		strlenth = (len - strlenth) / 2;
		Show_Str(strlenth + x, y, lcddev.width, lcddev.height, str, size, 1);
	}
}

/***********************************************************
 * text test note: need fatfs
 * *****************************************************/
void text_test(void)
{
	u8 hanzi[] = {0XBA, 0XBA, 0XA4, 0XA2, 0X9A, 0XDD, 0X90, 0XDB, 0XF6, 0XCC, 't', '1', 0};
    u8 hanzi2[] = {0X9A, 0XDD, 0XA4, 0XCB, 0XBF, 0XC9, 0X90, 0XDB, 0XA4, 0XA4, 't', '2', 0};
    POINT_COLOR=RED;       
    Show_Str(30,220,200,24,&hanzi2[0],24,1);
    Show_Str(30,244,200,16,&hanzi[0],16,0);
    Show_Str(30,260,200,12,&hanzi[0],12,0);					    	 
    LCD_Exec();
}
