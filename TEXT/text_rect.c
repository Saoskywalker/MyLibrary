#include "UI_engine.h"
#include "malloc.h"
#include "text.h"
#include "font.h"
#include "string.h"
#include "MTF_io.h"
#include "Sagittarius_global.h"

/***********************************************************************/
//在指定位置显示一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//size:字体大小 12/16/24
//mode:叠加方式(1)还是非叠加方式(0)
static void UI_showChar(RectInfo *Rect, u16 x, u16 y, u8 num, u8 size, u8 mode)
{
	u8 temp, t1, t;
	u16 y0 = y;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); //得到字体一个字符对应点阵集所占的字节数
	num = num - ' ';			//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
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
				UI_drawPoint(Rect, x, y, POINT_COLOR);
			else if (mode == 0)
				UI_drawPoint(Rect, x, y, BACK_COLOR);
			else 
				UI_drawPoint(Rect, x, y, 0X00FFFFFF); //全透明白色
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

//显示一个指定大小的汉字
//x,y :汉字的坐标
//font:汉字GBK码
//size:字体大小
//mode:0,正常显示,1,叠加显示
static void UI_showFont(RectInfo *Rect, u16 x, u16 y, u8 *font, u8 size, u8 mode)
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
				UI_drawPoint(Rect, x, y, POINT_COLOR);
			else if (mode == 0)
				UI_drawPoint(Rect, x, y, BACK_COLOR);
			else 
				UI_drawPoint(Rect, x, y, 0X00FFFFFF); //全透明白色
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

//生成需显示字符串图片
//支持自动换行
//width,height:区域
//str  :字符串
//size :字体大小
//mode:0,非叠加方式;1,叠加方式
void UI_showStr(RectInfo *pic, u8 *str, u8 size, u8 mode)
{
	u16 x = 0, y = 0; 
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
	pic->width = size/2*i;
	pic->height = size*j;
	pic->totalPixels = pic->width*pic->height;
	pic->pixelByte = render_front->bytes_per_pixel;
	pic->pixelDatas = malloc(pic->pixelByte*pic->totalPixels);
    if(pic->pixelDatas==NULL)
	{
        return;
	}
	// for(u16 k = 0; k<pic->totalPixels; k++)
	// 	((ColorClass *)pic->pixelDatas)[k] = 0X00FFFFFF; //全透明白色预填充

	//生成
	while (*str != 0) //数据未结束
	{
		if (!bHz)
		{
			if (*str > 0x80)
				bHz = 1; //中文
			else		 //字符
			{
				if (x > (x0 + pic->width - size / 2)) //换行
				{
					y += size;
					x = x0;
				}
				if (y > (y0 + pic->height - size))
					break;		//越界返回
				if (*str == 13) //换行符号
				{
					y += size;
					x = x0;
					str++;
				}
				else
					UI_showChar(pic, x, y, *str, size, mode); //有效部分写入
				str++;
				x += size / 2; //字符,为全字的一半
			}
		}
		else //中文
		{
			bHz = 0;					 //有汉字库
			if (x > (x0 + pic->width - size)) //换行
			{
				y += size;
				x = x0;
			}
			if (y > (y0 + pic->height - size))
				break;						  //越界返回
			UI_showFont(pic, x, y, str, size, mode); //显示这个汉字,空心显示
			str += 2;
			x += size; //下一个汉字偏移
		}
	}
}
/***********************************************************************/

/***********************************************************************/
//在指定位置显示一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//size:字体大小 12/16/24
//mode:叠加方式(1)还是非叠加方式(0)
static void UI_showChar2(RectInfo *RectF, RectInfo *RectB, u16 x, u16 y, u8 num, u8 size, u8 mode)
{
	u8 temp, t1, t;
	u16 y0 = y;
	u8 dzk[128]; //noet:字节缓存, 用于外部字体
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); //得到字体一个字符对应点阵集所占的字节数
	if (size != 8 && size != 12 && size != 16 && size != 24 && size != 32)
		return;					//不支持的size
	if(size == 8 || size == 32)
		Get_HzMat(&num, dzk, size+100); //得到相应大小的点阵数据, 用于外部字体, ascii字体库号偏移100
	else
		num = num - ' ';			//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
	for (t = 0; t < csize; t++)
	{
		if (size == 8)
			temp = dzk[t]; //调用外部8字体
		else if (size == 12)
			temp = asc2_1206[num][t]; //调用1206字体
		else if (size == 16)
			temp = asc2_1608[num][t]; //调用1608字体
		else if (size == 24)
			temp = asc2_2412[num][t]; //调用2412字体
		else if (size == 32)
			temp = dzk[t]; //调用外部32字体
		else
			return; //没有的字库
		for (t1 = 0; t1 < 8; t1++)
		{
			// if((LCD_disAdaptFlag==0||framebuffer_var_info.scale_flag)&&RectF==&LCD_disAdapt)
			if(RectF==&LCD_disAdapt)
			{
				if (temp & 0x80)
					LCD_Fast_DrawPoint(x, y, POINT_COLOR);
				else if (mode == 0)
					LCD_Fast_DrawPoint(x, y, BACK_COLOR);
				else 
					LCD_Fast_DrawPoint(x, y, LCD_ReadBackPoint(x, y));
			}
			else
			{
				if (temp & 0x80)
					UI_drawPoint(RectF, x, y, POINT_COLOR);
				else if (mode == 0)
					UI_drawPoint(RectF, x, y, BACK_COLOR);
				else
					UI_drawPoint(RectF, x, y, UI_readPoint(RectB, x, y)); //全透明白色
			}
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

//显示一个指定大小的汉字
//x,y :汉字的坐标
//font:汉字GBK码
//size:字体大小
//mode:0,正常显示,1,叠加显示
static void UI_showFont2(RectInfo *RectF, RectInfo *RectB, u16 x, u16 y, u8 *font, u8 size, u8 mode)
{
	u8 temp, t, t1;
	u16 y0 = y;
	u8 dzk[128]; //noet:字节缓存
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size); //得到字体一个字符对应点阵集所占的字节数

	if (size != 8 && size != 12 && size != 16 && size != 24  && size != 32)
		return;					//不支持的size
	Get_HzMat(font, dzk, size); //得到相应大小的点阵数据
	for (t = 0; t < csize; t++)
	{
		temp = dzk[t]; //得到点阵数据
		for (t1 = 0; t1 < 8; t1++)
		{
			// if((LCD_disAdaptFlag==0||framebuffer_var_info.scale_flag)&&RectF==&LCD_disAdapt)
			if(RectF==&LCD_disAdapt)
			{
				if (temp & 0x80)
					LCD_Fast_DrawPoint(x, y, POINT_COLOR);
				else if (mode == 0)
					LCD_Fast_DrawPoint(x, y, BACK_COLOR);
				else 
					LCD_Fast_DrawPoint(x, y, LCD_ReadBackPoint(x, y));
			}
			else
			{
				if (temp & 0x80)
					UI_drawPoint(RectF, x, y, POINT_COLOR);
				else if (mode == 0)
					UI_drawPoint(RectF, x, y, BACK_COLOR);
				else
					UI_drawPoint(RectF, x, y, UI_readPoint(RectB, x, y)); //全透明白色
			}
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

//**************UI推荐使用***************
//在指定位置开始显示一个字符串(自动计算长宽)
//支持自动换行(GBK码)
//RectF, RectB: 前景层(当为NULL时,写至显示层), 背景层
//(x,y):起始坐标
//width,height:区域
//str  :字符串
//size :字库号
//mode:0,非透明;1,透明
void UI_showStr2(RectInfo *RectF, RectInfo *RectB, u16 x, u16 y, u8 *str, u8 size, u8 mode)
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
			else		 //ASCII字符
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
					UI_showChar2(RectF, RectB, x, y, *str, size, mode); //有效部分写入
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
			UI_showFont2(RectF, RectB, x, y, str, size, mode); //显示这个汉字,空心显示
			str += 2;
			x += size; //下一个汉字偏移
		}
	}
	LCD_renewRequire = 1; //显示更新请求
}

/***********************************************************/
static void Get_Lattice(unsigned char *code, unsigned char *mat, u8 num, u8 size)
{
	static mFILE *ffont = NULL;
	u16 i;
	unsigned long foffset;
	u16 csize = 0;
	static char path[64] = {0}; //路径最大64byte
	char path_temp[64];
	char j[8], k[4];
	unsigned char qh, ql;
	
    memset(j, 0, sizeof(j));
	memset(k, 0, sizeof(k));
	memset(path_temp, 0, sizeof(path_temp));
	if (size > 100) //用于ascii字库
	{
		size -= 100; //减去偏移
		csize = ((u16)size / 8 + ((size % 8) ? 1 : 0)) * (size); //得到字体一个字符对应点阵集所占的字节数
		sprintf(j, "%d", num);
		sprintf(k, "%d", size);
		strcat(path_temp, dirPath);
		strcat(path_temp, j);
		strcat(path_temp, "_");
		strcat(path_temp, k);
		strcat(path_temp, ".FON");
		foffset = *code * csize; //得到字库中的字节偏移量
	}
	else
	{
		csize = ((u16)size / 8 + ((size % 8) ? 1 : 0)) * (size); //得到字体一个字符对应点阵集所占的字节数
		sprintf(j, "%d", num);
		sprintf(k, "%d", size);
		strcat(path_temp, dirPath);
		strcat(path_temp, j);
		strcat(path_temp, "_GBK");
		strcat(path_temp, k);
		strcat(path_temp, ".FON");
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
	}
	
	if(strcmp(path, path_temp)!=0) //没初始化
	{
		memset(path, 0, sizeof(path));
		if(path[0]!=0) //之前有, 先关
			MTF_close(ffont);
		ffont = NULL;
		ffont = MTF_open(path_temp, "rb");
		if (ffont != NULL)
			strcat(path, path_temp);
	}

	if (ffont != NULL)
	{
		MTF_seek(ffont, foffset, SEEK_SET);
		MTF_read(mat, 1, csize, ffont);
	}
	else
	{ //无字库, 用填充处理
		for (i = 0; i < csize; i++)
			*mat++ = 0x00; //填充满格
		return;			   //结束访问
	}
}
static void _DefineFont(RectInfo *RectF, RectInfo *RectB, u16 x, u16 y, 
						u8 *font, u8 num, u8 size, u8 mode)
{
	u8 temp;
	u16 t, t1;
	u16 y0 = y;
	u8 dzk[512]; //noet:字节缓存,, 最大64*64
	u16 csize = 0;

	if (size > 100) //用size大小区分汉字和ASCII码, 100以上用于ascii字库, 传入时size+100以区分
	{
		size -= 100;										//减去偏移
		csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size); //得到字体一个字符对应点阵集所占的字节数
		if (size > 64)
			return;								 //不支持的size
		Get_Lattice(font, dzk, num, size + 100); //得到相应大小的点阵数据
	}
	else
	{
		csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size); //得到字体一个字符对应点阵集所占的字节数
		if (size > 64)
			return;						   //不支持的size
		Get_Lattice(font, dzk, num, size); //得到相应大小的点阵数据
	}

	for (t = 0; t < csize; t++)
	{
		temp = dzk[t]; //得到点阵数据
		for (t1 = 0; t1 < 8; t1++)
		{
			// if((LCD_disAdaptFlag==0||framebuffer_var_info.scale_flag)&&RectF==&LCD_disAdapt)
			if(RectF==&LCD_disAdapt)
			{
				if (temp & 0x80) //有点
				{
					if (mode & 0X80) //前色显
						LCD_Fast_DrawPoint(x, y, POINT_COLOR);
					else
						LCD_Fast_DrawPoint(x, y, LCD_ReadBackPoint(x, y));
				}
				else //无点
				{
					if (mode & 0X40) //背色显
						LCD_Fast_DrawPoint(x, y, BACK_COLOR);
					else
						LCD_Fast_DrawPoint(x, y, LCD_ReadBackPoint(x, y));
				}
			}
			else
			{
				if (temp & 0x80) //有点
				{
					if (mode & 0X80) //前色显
						UI_drawPoint(RectF, x, y, POINT_COLOR);
					else
						UI_drawPoint(RectF, x, y, UI_readPoint(RectB, x, y));
				}
				else //无点
				{
					if (mode & 0X40) //背色显
						UI_drawPoint(RectF, x, y, BACK_COLOR);
					else
						UI_drawPoint(RectF, x, y, UI_readPoint(RectB, x, y)); //全透明白色
				}
			}
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

//**************UI推荐使用***************
//在指定位置开始显示一个字符串(使用自定义字库)(自动计算长宽)
//RectF, RectB: 前景层(当为NULL时,写至显示层), 背景层
//(x,y):起始坐标
//width,height:区域
//str  :字符串, len:字符长度
//num: 字库号; xy_size :字库点阵大小 高位y_size, 低位x_size
//mode:0,非透明;1,透明
void UI_showStrDefine1(RectInfo *RectF, RectInfo *RectB, u16 x, u16 y, 
						u8 *str, u8 num, u16 xy_size, u8 mode)
{
	u8 y_size = xy_size>>8; //y方向大小
	u8 x_size = (u8)xy_size; //x方向大小
	u16 x0 = x;
	u8 bHz = 0;		  //字符或者中文

	while (*str != 0) //数据未结束
	{
		if (!bHz)
		{
			if (*str > 0x80)
				bHz = 1; //中文
			else		 //ASCII字符
			{
				if (y + y_size > RectF->height)
					break;					   //越界返回
				if (x + x_size > RectF->width) //换行
				{
					y += y_size;
					x = x0;
					continue;
				}
				if (*str == 13) //换行符号
				{
					y += y_size;
					x = x0;
					str++;
				}
				else
					_DefineFont(RectF, RectB, x, y, str, num, y_size + 100, mode); //显示ASCII
				str++;
				x += x_size; //下一个字偏移
			}
		}
		else //中文
		{
			if (y + y_size > RectF->height)
				break;					   //越界返回
			if (x + y_size > RectF->width) //换行
			{
				y += y_size;
				x = x0;
				continue;
			}
			_DefineFont(RectF, RectB, x, y, str, num, y_size, mode); //显示汉字
			str += 2;
			x += y_size; //下一个汉字偏移
			bHz = 0;
		}
	}
	LCD_renewRequire = 1; //显示更新请求
}
/******************************************************************/
/***********************************************************************/
