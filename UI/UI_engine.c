#include "UI_engine.h"
#include "lodepng.h"
#include "MTF_io.h"
#include <malloc.h>
#include "bmp.h"
#include "tjpgd.h"
#include "delay.h"
#include "file_type.h"

RectInfo LCD_rectNow; //直接显存层
RectInfo LCD_rectBackup; //define background, use to backup after run animate
RectInfo LCD_disAdapt; //适应屏幕用缩放层
u8 LCD_renewRequire = 0; //显示更新请求

u8 UI_init(void)
{
	if(render_front==NULL||render_back==NULL)
		return 1; //失败

	LCD_rectNow.width = lcddev.width; 
	LCD_rectNow.height = lcddev.height; 
	LCD_rectNow.alpha = 255;
	LCD_rectNow.totalPixels	= lcddev.width * lcddev.height; 
	LCD_rectNow.format = PIXEL_FORMAT_ARGB32; 
	LCD_rectNow.pixelByte = render_front->bytes_per_pixel;
	LCD_rectNow.pixelDatas = MTF_fb_get_dis_mem(render_front);
	LCD_rectNow.crossWay = 0;
	LCD_rectBackup.width = lcddev.width; 
	LCD_rectBackup.height = lcddev.height; 
	LCD_rectBackup.alpha = 255;
	LCD_rectBackup.totalPixels	= lcddev.width * lcddev.height; 
	LCD_rectBackup.format = PIXEL_FORMAT_ARGB32; 
	LCD_rectBackup.pixelByte = render_front->bytes_per_pixel;
	LCD_rectBackup.crossWay = 0;
	LCD_rectBackup.pixelDatas = MTF_fb_get_dis_mem(render_back);
	if(LCD_rectBackup.pixelDatas==NULL)
		return 1; 
	
	LCD_disAdapt.width = lcddev.width; //自适应功能部分
	LCD_disAdapt.height = lcddev.height; 
	LCD_disAdapt.alpha = 255;
	LCD_disAdapt.totalPixels = lcddev.width * lcddev.height; 
	LCD_disAdapt.format = PIXEL_FORMAT_ARGB32; 
	LCD_disAdapt.pixelByte = render_front->bytes_per_pixel;
	LCD_disAdapt.crossWay = 0;
	LCD_disAdapt.pixelDatas = MTF_fb_get_dis_mem(render_front);

	UI_LCDBackupRenew();
	control_table_init();
	return 0;
}

/********************************************
 * 用于用户程序分辨率和实际屏幕不同时,自适应屏幕, 默认不用
 * ******************************************/
double adaptScaleY = 0, adaptScaleX = 0; //适应缩放系数
u8 LCD_disAdaptFlag = 0; //自适应开启标志
typedef struct __attribute__((packed))
{
	u16 x;
	u16 y;
}UI_coords;
static UI_coords *UI_adaptScaleTable;

//自适应屏幕专用图片缩放处理---注意:这不新建矩阵, 使用前先分配 bmpResult:结果, bmpImg:源, dy, dx 长宽缩放系数
/* static u8 UI_disAdaptScale(RectInfo *bmpResult, RectInfo *bmpImg, double dy, double dx)
{
	int width = 0;
	int height = 0;
	int i, j;

	width = bmpImg->width;
	height = bmpImg->height;
	int pre_i, pre_j; //缩放前对应的像素点坐标

	//坐标变换
	for (i = 0; i < bmpResult->height; i++)
	{
		for (j = 0; j < bmpResult->width; j++)
		{
			pre_i = (int)(i / dx + 0.5);
			pre_j = (int)(j / dy + 0.5);
			if (pre_i >= 0 && pre_i < height && pre_j >= 0 && pre_j < width) //在原图范围内
			{
				LCD_Fast_DrawPoint(j, i, UI_readPoint(bmpImg, pre_j, pre_i));
			}
			else //补充: 在原图范围内, 为避免边界忽略
			{
				if(pre_i >= 0 && pre_j >= 0)
				{
					if(pre_i < height)
						pre_i = height;
					if(pre_j < width)
						pre_j = width;
					LCD_Fast_DrawPoint(j, i, UI_readPoint(bmpImg, pre_j, pre_i));
				}
			}
		}
	}
	return 0; //成功
} */
u8 UI_disAdaptScale(RectInfo *bmpResult, RectInfo *bmpImg, double dy, double dx)
{
	int i, j;
	u32 k = 0;

	//坐标变换 ---查表法
	for (i = 0; i < bmpResult->height; i++)
	{
		for (j = 0; j < bmpResult->width; j++)
		{
			LCD_Fast_DrawPoint(j, i, 
			UI_readPoint(bmpImg, UI_adaptScaleTable[k].x, UI_adaptScaleTable[k].y));
			k++;
		}
	}
	return 0; //成功
}

//开启自适应, WIDTH, HEIGHT: 适应层长宽,, 要先初始化UI_init
u8 UI_disAdapt(int width, int height)
{
	MTF_fb_destroy(render_front); //去掉旧
	MTF_fb_destroy(render_back);

	//重建FB, 以完成缩放(改变实际显示分辨率), 如不重建,则直接使用系统缩放, 则不用重建(不改变实际显示分辨率)
	framebuffer_var_info.xres = width;
	framebuffer_var_info.yres = height;
	MTF_fb_reset(&framebuffer_var_info);

	render_front = MTF_fb_render_create(&framebuffer_var_info, width, height); //再按新参数重建
	render_back = MTF_fb_render_create(&framebuffer_var_info, width, height);
	if(render_front==NULL||render_back==NULL)
		return 1; //失败
		
	LCD_disAdapt.width = width; 
	LCD_disAdapt.height = height; 
	LCD_disAdapt.alpha = 255;
	LCD_disAdapt.totalPixels = width * height; 
	LCD_disAdapt.format = PIXEL_FORMAT_ARGB32; 
	LCD_disAdapt.pixelByte = render_front->bytes_per_pixel;
	LCD_disAdapt.crossWay = 0;
	LCD_disAdapt.pixelDatas = MTF_fb_get_dis_mem(render_front);
	LCD_rectBackup.width = width; 
	LCD_rectBackup.height = height; 
	LCD_rectBackup.alpha = 255;
	LCD_rectBackup.totalPixels	= width * height; 
	LCD_rectBackup.format = PIXEL_FORMAT_ARGB32; 
	LCD_rectBackup.pixelByte = render_front->bytes_per_pixel;
	LCD_rectBackup.crossWay = 0;
	LCD_rectBackup.pixelDatas = MTF_fb_get_dis_mem(render_back);
	if(LCD_disAdapt.pixelDatas==NULL||LCD_rectBackup.pixelDatas==NULL)
		return 1; //失败

	//缩放率计算
	adaptScaleX = (LCD_rectNow.width-0.5)/LCD_disAdapt.width;
	adaptScaleY = (LCD_rectNow.height-0.5)/LCD_disAdapt.height;
/* 
	//缩放表(用于查表法缩放)
	u32 k = 0;
	UI_adaptScaleTable = (UI_coords *)malloc(LCD_rectNow.totalPixels*4);
	if(UI_adaptScaleTable==NULL)
		return 1; //失败
	for (int i = 0; i < LCD_rectNow.height; i++)
	{
		for (int j = 0; j < LCD_rectNow.width; j++)
		{
			UI_adaptScaleTable[k].y = (int)(i / adaptScaleY + 0.5);
			UI_adaptScaleTable[k].x = (int)(j / adaptScaleX + 0.5);
			if (UI_adaptScaleTable[k].y >= 0 && UI_adaptScaleTable[k].y < LCD_disAdapt.height &&
			 UI_adaptScaleTable[k].x >= 0 && UI_adaptScaleTable[k].x < LCD_disAdapt.width) //在原图范围内
			{
			}
			else //补充: 在原图范围内, 为避免边界忽略
			{
				if(UI_adaptScaleTable[k].y >= 0 && UI_adaptScaleTable[k].x >= 0)
				{
					if(UI_adaptScaleTable[k].y < LCD_disAdapt.height)
						UI_adaptScaleTable[k].y = LCD_disAdapt.height;
					if(UI_adaptScaleTable[k].x < LCD_disAdapt.width)
						UI_adaptScaleTable[k].x = LCD_disAdapt.width;
				}
			}
			k++;
		}
	}

	for(u32 p = 0; p<LCD_disAdapt.totalPixels; p++)
		((ColorClass *)LCD_disAdapt.pixelDatas)[p] = 0XFF000000;
	UI_disRegionCrossAdapt(&LCD_disAdapt, 0, 0); //更新显示
	LCD_Exec();
	UI_LCDBackupRenew();
	LCD_disAdaptFlag = 1;
 */	
	/******开启硬件缩放处理************/
	lcddev.pixelDatas = LCD_disAdapt.pixelDatas; //改变显存位置
	lcddev.totalPixels = LCD_disAdapt.totalPixels;
	LCD_Display_Dir(lcddev.dir); //设置读写显存函数
	MTF_fb_scale(&framebuffer_var_info, 1, LCD_disAdapt.pixelDatas);
	LCD_Clear(BLACK); //清屏
	LCD_Exec(); //更新显示
	UI_LCDBackupRenew();
	/*******************************/
	return 0; //成功
} 

//增加缩放层, 之后更新到显示层, 用于块更新, 点或线更新不不缩放
u8 UI_disRegionCrossAdapt(RectInfo *Slave, int x, int y)
{
	u8 res;
	// if (LCD_disAdaptFlag && framebuffer_var_info.scale_flag == 0)
	// { //不用硬件缩放时, 用软件缩放
	// 	res = UI_rectRegionCross(&LCD_disAdapt, Slave, x, y);
	// 	res = UI_disAdaptScale(&LCD_rectNow, &LCD_disAdapt, adaptScaleX, adaptScaleY);
	// }
	// else
	{
		res = UI_disRegionCross(Slave, x, y);
	}
	return res;
}

//触摸适应
int UI_pressTouchY = 0, UI_pressTouchX = 0, UI_freeTouchY = 0, UI_freeTouchX = 0;
char UI_pressState = 0;
void UI_touchAdapt(int x, int y, u8 state)
{
	if(state) //按下
	{
		if(framebuffer_var_info.scale_flag)
		{
			UI_pressTouchX = (int)(x / adaptScaleX + 0.5);
			UI_pressTouchY = (int)(y / adaptScaleY + 0.5);
			if(UI_pressTouchX>LCD_disAdapt.width)
				UI_pressTouchX = LCD_disAdapt.width;
			if(UI_pressTouchY>LCD_disAdapt.height)
				UI_pressTouchY = LCD_disAdapt.height;
		}
		else
		{
			UI_pressTouchX = x;
			UI_pressTouchY = y;
		}
		UI_pressState = 1;
	}
	else //弹起
	{
		if(framebuffer_var_info.scale_flag)
		{
			UI_freeTouchX = (int)(x / adaptScaleX + 0.5);
			UI_freeTouchY = (int)(y / adaptScaleY + 0.5);
			if(UI_freeTouchX>LCD_disAdapt.width)
				UI_freeTouchX = LCD_disAdapt.width;
			if(UI_freeTouchY>LCD_disAdapt.height)
				UI_freeTouchY = LCD_disAdapt.height;
		}
		else
		{
			UI_freeTouchX = x;
			UI_freeTouchY = y;
		}
		UI_pressState = 0;
	}
	
}
/***************************************************/

void UI_unload(void)
{
}

void UI_LCDBackupRenew(void)
{
	memcpy(LCD_rectBackup.pixelDatas, LCD_disAdapt.pixelDatas, 
		LCD_rectBackup.totalPixels*LCD_rectBackup.pixelByte);
}

void UI_LCDBackupDis(void)
{
	memcpy(LCD_disAdapt.pixelDatas, LCD_rectBackup.pixelDatas,
		LCD_rectBackup.totalPixels*LCD_rectBackup.pixelByte);
}

 //由于更新屏幕有时有更新不全, 要执行UI_cacheRenew,临时处理
/* static char temp_flush[1024*256] __attribute__ ((aligned (32)));
void UI_cacheRenew(void)
{
	memcpy(temp_flush, LCD_disAdapt.pixelDatas, 
		1024*256);
} */

void UI_layerCopy(RectInfo *dest, RectInfo *src)
{
	dest->alpha = src->alpha;
	dest->crossWay = src->crossWay;
	dest->format = src->format;
	dest->height = src->height;
	dest->pixelByte = src->pixelByte;
	dest->pixelDatas = src->pixelDatas;
	dest->totalPixels = src->totalPixels;
	dest->width = src->width;
}

/*Math function, can be replaced by math.h*/
static float UI_abs(float x)
{
	if (x < 0)
		x = 0 - x;
	return x;
}

static float UI_sin(float x)
{
	const float B = 1.2732395447;
	const float C = -0.4052847346;
	const float P = 0.2310792853; //0.225;
	float y = B * x + C * x * UI_abs(x);
	y = P * (y * UI_abs(y) - y) + y;
	return y;
}

#define PI 3.1415926

static float UI_cos(float x)
{
	const float Q = 1.5707963268;
	x += Q;
	if (x > PI)
		x -= 2 * PI;
	return (UI_sin(x));
}
/******************************************/

//图片旋转处理, note: 图像长宽必须相等, 否则旋转点不对
//bmpResult:结果, bmpImg:源, angle旋转角度
u8 UI_imrotate(RectInfo *bmpResult, RectInfo *bmpImg, int Angle)
{
	float angle; //要旋转的弧度数
	float sin_value = 0, cos_value = 0;
	int width = 0;
	int height = 0;
	int step = 0;
	int Rot_step = 0;
	int pixelByte = 1;
	u32 i, j, k;
	width = bmpImg->width;
	height = bmpImg->height;
	pixelByte = bmpImg->pixelByte;
	int midX_pre, midY_pre, midX_aft, midY_aft; //旋转前后的中心点的坐标
	midX_pre = (float)width / 2 + 0.5;
	midY_pre = (float)height / 2 + 0.5;
	int pre_i, pre_j, after_i, after_j; //旋转前后对应的像素点坐标
	angle = 1.0 * Angle * PI / 180;
	sin_value = UI_sin((float)angle);
	cos_value = UI_cos((float)angle);
	//初始化旋转后图片的信息
	bmpResult->format = bmpImg->format;
	bmpResult->alpha = bmpImg->alpha;
	bmpResult->crossWay = bmpImg->crossWay;
	bmpResult->pixelByte = pixelByte;
	bmpResult->width = bmpImg->width;
	bmpResult->height = bmpImg->height;
	bmpResult->totalPixels = bmpImg->totalPixels;
	midX_aft = (float)bmpResult->width / 2 + 0.5;
	midY_aft = (float)bmpResult->height / 2 + 0.5;
	step = pixelByte * width;
	Rot_step = pixelByte * bmpResult->width;
	/*NOTE: 注意消去之前分配的内存,,,避免内存池枯竭*/
	bmpResult->pixelDatas = \
	malloc(sizeof(unsigned char) * bmpImg->width * bmpImg->height * pixelByte); //需修改
	if(bmpResult->pixelDatas==NULL)
		return 1; //错误

	if (pixelByte == 1)
	{
		//初始化旋转图像
		for (i = 0; i < bmpResult->totalPixels; i++)
			((unsigned char *)bmpResult->pixelDatas)[i] = 0;

		//坐标变换
		for (i = 0; i < bmpResult->height; i++)
		{
			for (j = 0; j < bmpResult->width; j++)
			{
				after_i = i - midX_aft;
				after_j = j - midY_aft;
				pre_i = (int)(cos_value * after_i - sin_value * after_j) + midX_pre;
				pre_j = (int)(sin_value * after_i + cos_value * after_j) + midY_pre;
				if (pre_i >= 0 && pre_i < height && pre_j >= 0 && pre_j < width) //在原图范围内
					((unsigned char *)bmpResult->pixelDatas)[i * Rot_step + j] = \
					((unsigned char *)bmpImg->pixelDatas)[pre_i * step + pre_j];
			}
		}
	}
	else
	{
		//初始化旋转图像
		for (i = 0; i < bmpResult->totalPixels; i++)
			((ColorClass *)bmpResult->pixelDatas)[i] = 0;

		//坐标变换
		for (i = 0; i < bmpResult->height; i++)
		{
			for (j = 0; j < bmpResult->width; j++)
			{
				after_i = i - midX_aft;
				after_j = j - midY_aft;
				pre_i = (int)(cos_value * after_i - sin_value * after_j) + midX_pre;
				pre_j = (int)(sin_value * after_i + cos_value * after_j) + midY_pre;

				if (pre_i >= 0 && pre_i < height && pre_j >= 0 && pre_j < width) //在原图范围内
				{
					for (k = 0; k < pixelByte; k++)
					{
						((unsigned char *)bmpResult->pixelDatas)[i * Rot_step + j * pixelByte + k] = \
						((unsigned char *)bmpImg->pixelDatas)[pre_i * step + pre_j * pixelByte + k];
					}
				}
			}
		}
	}
	return 0; //成功
}

//图片缩放处理 bmpResult:结果, bmpImg:源, dy, dx 长宽缩放系数
u8 UI_imscale(RectInfo *bmpResult, RectInfo *bmpImg, double dy, double dx)
{
	int width = 0;
	int height = 0;
	int pixelByte = 4;
	int step = 0;
	int Sca_step = 0;
	int i, j, k;
	width = bmpImg->width;
	height = bmpImg->height;
	pixelByte = bmpImg->pixelByte;
	int pre_i, pre_j, after_i, after_j; //缩放前对应的像素点坐标
	//初始化缩放后图片的信息
	bmpResult->pixelByte = pixelByte;
	bmpResult->width = (int)(bmpImg->width * dy + 0.5);
	bmpResult->height = (int)(bmpImg->height * dx + 0.5);
	step = pixelByte * width;
	Sca_step = pixelByte * bmpResult->width;
	/*NOTE: 注意消去之前分配的内存,,,避免内存池枯竭*/
	bmpResult->pixelDatas = \
	malloc(sizeof(unsigned char) * bmpResult->width * bmpResult->height * pixelByte);
	if(bmpResult->pixelDatas==NULL)
		return 1; //错误
		
	if (pixelByte == 1)
	{
		//初始化缩放图像
		for (i = 0; i < bmpResult->height; i++)
		{
			for (j = 0; j < bmpResult->width; j++)
			{
				((unsigned char *)bmpResult->pixelDatas)[(bmpResult->height - 1 - i) * Sca_step + j] = 0;
			}
		}
		//坐标变换
		for (i = 0; i < bmpResult->height; i++)
		{
			for (j = 0; j < bmpResult->width; j++)
			{
				after_i = i;
				after_j = j;
				pre_i = (int)(after_i / dx + 0);
				pre_j = (int)(after_j / dy + 0);
				if (pre_i >= 0 && pre_i < height && pre_j >= 0 && pre_j < width) //在原图范围内
				{
					((unsigned char *)bmpResult->pixelDatas)[i * Sca_step + j] = \
					((unsigned char *)bmpImg->pixelDatas)[pre_i * step + pre_j];
				}
				else //补充: 在原图范围内, 为避免边界忽略
				{
					if(pre_i >= 0 && pre_j >= 0)
					{
						if(pre_i < height)
							pre_i = height;
						if(pre_j < width)
							pre_j = width;
						((unsigned char *)bmpResult->pixelDatas)[i * Sca_step + j] = \
						((unsigned char *)bmpImg->pixelDatas)[pre_i * step + pre_j];
					}
				}
			}
		}
	}
	else
	{
		//初始化缩放图像
		for (i = 0; i < bmpResult->height; i++)
		{
			for (j = 0; j < bmpResult->width; j++)
			{
				for (k = 0; k < pixelByte; k++)
				{
					((unsigned char *)bmpResult->pixelDatas)[(bmpResult->height - 1 - i) * Sca_step + j * pixelByte + k] = 0;
				}
			}
		}
		//坐标变换
		for (i = 0; i < bmpResult->height; i++)
		{
			for (j = 0; j < bmpResult->width; j++)
			{
				after_i = i;
				after_j = j;
				pre_i = (int)(after_i / dx + 0.5);
				pre_j = (int)(after_j / dy + 0.5);
				if (pre_i >= 0 && pre_i < height && pre_j >= 0 && pre_j < width) //在原图范围内
				{
					for (k = 0; k < pixelByte; k++)
					{
						((unsigned char *)bmpResult->pixelDatas)[i * Sca_step + j * pixelByte + k] = \
						((unsigned char *)bmpImg->pixelDatas)[pre_i * step + pre_j * pixelByte + k];
					}
				}
				else //补充: 在原图范围内, 为避免边界忽略
				{
					if(pre_i >= 0 && pre_j >= 0)
					{
						if(pre_i < height)
							pre_i = height;
						if(pre_j < width)
							pre_j = width;
						for (k = 0; k < pixelByte; k++)
						{
							((unsigned char *)bmpResult->pixelDatas)[i * Sca_step + j * pixelByte + k] = \
							((unsigned char *)bmpImg->pixelDatas)[pre_i * step + pre_j * pixelByte + k];
						}
					}
				}
			}
		}
	}
	return 0; //成功
}

//MASTER为背景, slave为前景, 混合设置以slave为准, 混合结果于master, 只用于UI层
u8 UI_rectRegionCross(RectInfo *Master, RectInfo *Slave, int x, int y)
{
	int cutX1 = 0, cutY1 = 0, cutX2 = 0, cutY2 = 0;
	int tempX = 0, tempY = 0, tempDisX = 0, tempDisY = 0;
	int DisX = 0, DisY = 0;
	ColorClass color = 0, color2 = 0, color3 = 0;

	if (Slave->pixelDatas==NULL)
		return 1; //错误, 无数据结束

	if (x > Master->width - 1 || y > Master->height - 1)
		return 0; //正过界结束
	else if (Slave->width - (-x) <= 0 || Slave->height - (-y) <= 0)
		return 0; //负过界结束

	if (x < 0) //slave矩形需剪切范围:cutX1 cutX2
	{
		DisX = 0;
		cutX1 = -x;
		if (Slave->width - cutX1 > Master->width) //x长度大于master
			cutX2 = cutX1 + Master->width - 1;
		else
			cutX2 = Slave->width - 1;
	}
	else
	{
		DisX = x;
		cutX1 = 0;
		if (x + Slave->width - 1 > Master->width - 1) //x长度大于master
			cutX2 = Master->width - 1 - x;
		else
			cutX2 = Slave->width - 1;
	}

	if (y < 0) //slave矩形需剪切范围:cutY1 cutY2
	{
		DisY = 0;
		cutY1 = -y;
		if (Slave->height - cutY1 > Master->height) //y长度大于master
			cutY2 = cutY1 + Master->height - 1;
		else
			cutY2 = Slave->height - 1;
	}
	else
	{
		DisY = y;
		cutY1 = 0;
		if (y + Slave->height - 1 > Master->height - 1) //y长度大于master
			cutY2 = Master->height - 1 - y;
		else
			cutY2 = Slave->height - 1;
	}

	//从slave取数据到显示内存
	if (Slave->crossWay & 0X01 && Slave->crossWay & 0X02)
	{
		for (tempY = cutY1, tempDisY = DisY; tempY <= cutY2; tempY++, tempDisY++)
		{
			for (tempX = cutX1, tempDisX = DisX; tempX <= cutX2; tempX++, tempDisX++)
			{
				color = UI_readPoint(Slave, tempX, tempY);
				if (color & 0XFF000000) //先处理部分显, 再处理透明
				{
					color2 = UI_readPoint(Master, tempDisX, tempDisY);
					color3 = alpha_blending(&color, &color2, Slave->alpha);
					UI_drawPoint(Master, tempDisX, tempDisY, color3);
				}
			}
		}
	}
	else if (Slave->crossWay & 0X01 && (Slave->crossWay & 0X02) == 0)
	{
		for (tempY = cutY1, tempDisY = DisY; tempY <= cutY2; tempY++, tempDisY++)
		{
			for (tempX = cutX1, tempDisX = DisX; tempX <= cutX2; tempX++, tempDisX++)
			{
				color = UI_readPoint(Slave, tempX, tempY);
				if (color & 0XFF000000) //先处理部分显
				{
					UI_drawPoint(Master, tempDisX, tempDisY, color);
				}
			}
		}
	}
	else if ((Slave->crossWay & 0X01) == 0 && Slave->crossWay & 0X02)
	{ //整体透明处理
		for (tempY = cutY1, tempDisY = DisY; tempY <= cutY2; tempY++, tempDisY++)
		{
			for (tempX = cutX1, tempDisX = DisX; tempX <= cutX2; tempX++, tempDisX++)
			{
				color = UI_readPoint(Slave, tempX, tempY);
				color2 = UI_readPoint(Master, tempDisX, tempDisY);
				color3 = alpha_blending(&color, &color2, Slave->alpha);
				UI_drawPoint(Master, tempDisX, tempDisY, color3);
			}
		}
	}
	else if (Slave->crossWay & 0X04)
	{
		for (tempY = cutY1, tempDisY = DisY; tempY <= cutY2; tempY++, tempDisY++)
		{
			for (tempX = cutX1, tempDisX = DisX; tempX <= cutX2; tempX++, tempDisX++)
			{
				color = UI_readPoint(Slave, tempX, tempY);
				if (color != BACK_COLOR) //滤去背景色
					UI_drawPoint(Master, tempDisX, tempDisY, color);
				else
					UI_drawPoint(Master, tempDisX, tempDisY, UI_readPoint(&LCD_rectBackup, tempDisX, tempDisY));
			}
		}
	}
	else
	{
		for (tempY = cutY1, tempDisY = DisY; tempY <= cutY2; tempY++, tempDisY++)
		{ //单纯混合
			for (tempX = cutX1, tempDisX = DisX; tempX <= cutX2; tempX++, tempDisX++)
				UI_drawPoint(Master, tempDisX, tempDisY, UI_readPoint(Slave, tempX, tempY));
		}
	}
	return 0; //成功结束
}

//Master为显示内存, 只用于UI至屏幕显示
u8 UI_disRegionCross(RectInfo *Slave, int x, int y)
{   
	int cutX1 = 0, cutY1 = 0, cutX2 = 0, cutY2 = 0;
	int tempX = 0, tempY = 0, tempDisX = 0, tempDisY = 0;
	int DisX = 0, DisY = 0;
	ColorClass color = 0, color2 = 0, color3 = 0;

	if (Slave->pixelDatas==NULL)
		return 1; //错误, 无数据结束

	if (x > lcddev.width - 1 || y > lcddev.height - 1)
		return 0; //正过界结束
	else if (Slave->width - (-x) <= 0 || Slave->height - (-y) <= 0)
		return 0; //负过界结束

	if (x < 0) //slave矩形需剪切范围:cutX1 cutX2
	{
		DisX = 0;
		cutX1 = -x;
		if (Slave->width - cutX1 > lcddev.width) //x长度大于master
			cutX2 = cutX1 + lcddev.width - 1;
		else
			cutX2 = Slave->width - 1;
	}
	else
	{
		DisX = x;
		cutX1 = 0;
		if (x + Slave->width - 1 > lcddev.width - 1) //x长度大于master
			cutX2 = lcddev.width - 1 - x;
		else
			cutX2 = Slave->width - 1;
	}

	if (y < 0) //slave矩形需剪切范围:cutY1 cutY2
	{
		DisY = 0;
		cutY1 = -y;
		if (Slave->height - cutY1 > lcddev.height) //y长度大于master
			cutY2 = cutY1 + lcddev.height - 1;
		else
			cutY2 = Slave->height - 1;
	}
	else
	{
		DisY = y;
		cutY1 = 0;
		if (y + Slave->height - 1 > lcddev.height - 1) //y长度大于master
			cutY2 = lcddev.height - 1 - y;
		else
			cutY2 = Slave->height - 1;
	}

	//从slave取数据到显示内存
	if (Slave->crossWay & 0X01 && Slave->crossWay & 0X02)
	{
		for (tempY = cutY1, tempDisY = DisY; tempY <= cutY2; tempY++, tempDisY++)
		{
			for (tempX = cutX1, tempDisX = DisX; tempX <= cutX2; tempX++, tempDisX++)
			{
				color = UI_readPoint(Slave, tempX, tempY);
				if (color & 0XFF000000) //先处理部分显, 再处理透明
				{
					color2 = LCD_ReadPoint(tempDisX, tempDisY);
					color3 = alpha_blending(&color, &color2, (u8)(color>>24));//Slave->alpha);
					LCD_Fast_DrawPoint(tempDisX, tempDisY, color3);
				}
			}
		}
	}
	else if (Slave->crossWay & 0X01 && (Slave->crossWay & 0X02) == 0)
	{
		for (tempY = cutY1, tempDisY = DisY; tempY <= cutY2; tempY++, tempDisY++)
		{
			for (tempX = cutX1, tempDisX = DisX; tempX <= cutX2; tempX++, tempDisX++)
			{
				color = UI_readPoint(Slave, tempX, tempY);
				if (color & 0XFF000000) //先处理部分显
				{
					LCD_Fast_DrawPoint(tempDisX, tempDisY, color);
				}
			}
		}
	}
	else if ((Slave->crossWay & 0X01) == 0 && Slave->crossWay & 0X02)
	{
		for (tempY = cutY1, tempDisY = DisY; tempY <= cutY2; tempY++, tempDisY++)
		{
			for (tempX = cutX1, tempDisX = DisX; tempX <= cutX2; tempX++, tempDisX++)
			{	//整体透明处理
				color = UI_readPoint(Slave, tempX, tempY);
				color2 = LCD_ReadPoint(tempDisX, tempDisY);
				color3 = alpha_blending(&color, &color2, Slave->alpha);
				LCD_Fast_DrawPoint(tempDisX, tempDisY, color3);
			}
		}
	}
	else if (Slave->crossWay & 0X04)
	{
		for (tempY = cutY1, tempDisY = DisY; tempY <= cutY2; tempY++, tempDisY++)
		{
			for (tempX = cutX1, tempDisX = DisX; tempX <= cutX2; tempX++, tempDisX++)
			{
				color = UI_readPoint(Slave, tempX, tempY);
				if (color != BACK_COLOR) //滤去背景色
					LCD_Fast_DrawPoint(tempDisX, tempDisY, color);
				else
					LCD_Fast_DrawPoint(tempDisX, tempDisY, LCD_ReadBackPoint(tempDisX, tempDisY));
			}
		}
	}
	else
	{
		for (tempY = cutY1, tempDisY = DisY; tempY <= cutY2; tempY++, tempDisY++)
		{ //单纯混合
			for (tempX = cutX1, tempDisX = DisX; tempX <= cutX2; tempX++, tempDisX++)
				LCD_Fast_DrawPoint(tempDisX, tempDisY, UI_readPoint(Slave, tempX, tempY));
		}
	}
	return 0; //成功结束
}

//读取前景矩形到指定矩形
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)
//dest: 目标矩形
void UI_FillFrontToDest(RectInfo *dest, u16 x1, u16 y1)
{
	u16 x, y, xx, yy;
	u16 sx, sy, ex, ey;
	u32 i = 0;

	dest->pixelDatas = malloc(dest->totalPixels*dest->pixelByte);
	if(dest->pixelDatas==NULL)
		return;
	sx = x1; 
	sy = y1;
	ex = sx+dest->width-1;
	ey = sx+dest->height-1;
	// if (LCD_disAdaptFlag)
	// {
	// 	for (y = sy, yy = 0; y <= ey; y++, yy++)
	// 	{
	// 		for (x = sx, xx = 0; x <= ex; x++, xx++)
	// 		{
	// 			i = x + y * LCD_disAdapt.width;
	// 			((ColorClass *)dest->pixelDatas)[xx+yy*dest->width] = ((ColorClass *)LCD_disAdapt.pixelDatas)[i];
	// 		}
	// 	}
	// }
	// else
	{
		for (y = sy, yy = 0; y <= ey; y++, yy++)
		{
			for (x = sx, xx = 0; x <= ex; x++, xx++)
			{
				if (lcddev.dir == DIS_DIR_90) //竖屏
					i = LCD_disAdapt.width * (LCD_disAdapt.height - 1 - x) + y;
				else if (lcddev.dir == DIS_DIR_180) //横
					i = LCD_disAdapt.totalPixels - (x + y * LCD_disAdapt.width);
				else if (lcddev.dir == DIS_DIR_270) //竖
					i = LCD_disAdapt.width * (x + 1) - 1 - y;
				else //横屏 0度
					i = x + y * LCD_disAdapt.width;
				((ColorClass *)dest->pixelDatas)[xx+yy*dest->width] = ((ColorClass *)LCD_disAdapt.pixelDatas)[i];
			}
		}
	}
}

/*******************************************
 * 图片解码和截取
 * ***************************************/
unsigned char png_decode(const char *filename, RectInfo *ptr)
{
	unsigned char error = 0;
	unsigned char *image;
	unsigned width, height;
	unsigned char *png = 0;
	size_t pngsize = 0;
  	unsigned int pixels = 0;

	error = lodepng_load_file(&png, &pngsize, (const char *)filename);
	if (!error)
	{
		error = lodepng_decode32(&image, &width, &height, png, pngsize);
	}	
	else
	{
		ptr->pixelDatas = NULL; //失败时付NULL, 保证代码健壮性
		return error;
	}
	if (error)
	{
		// printf("error %u: %s\n", error, lodepng_error_text(error));
	}
	else
	{
    // printf("png width: %d, height: %d\r\n", width, height);
    	pixels = width*height;
/* 		for (int i = 0; i < pixels; i++)
		{
			((ColorClass *)image)[i] = ((((ColorClass *)image)[i] & 0X00FF0000) >> 16) + \
									   (((ColorClass *)image)[i] & 0XFF00FF00) + \
									   ((((ColorClass *)image)[i] & 0X000000FF) << 16);
		} */
	}

	free(png);
	ptr->height = height;
	ptr->width = width;
	ptr->totalPixels = pixels;
 	ptr->pixelDatas = image; //note: free it after use it

	return error;
}

unsigned char png_decode_cut(const char *filename, RectInfo *ptr, 
							u16 x1, u16 y1, u16 x2, u16 y2)
{
    RectInfo pic;
    int x, y;

    x = x1;
    y = y1;
    pic.pixelByte = 4;
    pic.crossWay = 0;
    pic.alpha = 255;
    ptr->pixelByte = 4;
    ptr->width = x2-x1+1;
    ptr->height = y2-y1+1;
    ptr->totalPixels = ptr->width*ptr->height;
    ptr->pixelDatas = malloc(ptr->totalPixels*ptr->pixelByte);
    // //printf("x: %d, y: %d, w: %d, h: %d\r\n", x, y, ptr->width, ptr->height);
    if(ptr->pixelDatas==NULL)
    {  
        return 1;
    }
    if(png_decode(filename, &pic)==0) 
    {
        // printf("ptr->r\n");
        UI_rectRegionCross(ptr, &pic, -x, -y); //cut
        free(pic.pixelDatas);
    }
	else
	{
		free(ptr->pixelDatas);
		return 2;
	}
	return 0;
}

unsigned char bmp_decode(const char *filename, RectInfo *ptr)
{
	unsigned char error = 0;
	unsigned char *image;
	unsigned width, height;
  	unsigned int pixels = 0;
	
	error = lodebmp_decode(&image, &width, &height, filename);
	if(error==0)
	{
		pixels = width*height;
		ptr->height = height;
		ptr->width = width;
		ptr->totalPixels = pixels;
		ptr->pixelDatas = image; //note: free it after use it
	}
	return error;
}

unsigned char bmp_decode_cut(const char *filename, RectInfo *ptr, 
							u16 x1, u16 y1, u16 x2, u16 y2)
{
	unsigned char error = 0;
	unsigned char *image;
	
	error = lodebmp_decode_cut(&image, filename, x1, y1, x2, y2);
	if(error==0)
	{
		ptr->height = y2-y1+1;
		ptr->width = x2-x1+1;
		ptr->totalPixels = ptr->height*ptr->width;
		ptr->pixelDatas = image; //note: free it after use it
	}
	return error;
}

unsigned char jpg_decode(const char *filename, RectInfo *ptr)
{
	unsigned char error = 0;
	unsigned char *image;
	unsigned width, height;
  	unsigned int pixels = 0;
	
	error = lodejpg_decode(&image, &width, &height, filename);
	if(error==0)
	{
		pixels = width*height;
		ptr->height = height;
		ptr->width = width;
		ptr->totalPixels = pixels;
		ptr->pixelDatas = image; //note: free it after use it
	}
	return error;
}

unsigned char jpg_decode_cut(const char *filename, RectInfo *ptr, 
							u16 x1, u16 y1, u16 x2, u16 y2)
{
    RectInfo pic;
    int x, y;

    x = x1;
    y = y1;
    pic.pixelByte = 4;
    pic.crossWay = 0;
    pic.alpha = 255;
    ptr->pixelByte = 4;
    ptr->width = x2-x1+1;
    ptr->height = y2-y1+1;
    ptr->totalPixels = ptr->width*ptr->height;
    ptr->pixelDatas = malloc(ptr->totalPixels*ptr->pixelByte);
    // //printf("x: %d, y: %d, w: %d, h: %d\r\n", x, y, ptr->width, ptr->height);
    if(ptr->pixelDatas==NULL)
    {  
        return 1;
    }
    if(jpg_decode(filename, &pic)==0) 
    {
        // printf("ptr->r\n");
        UI_rectRegionCross(ptr, &pic, -x, -y); //cut
        free(pic.pixelDatas);
    }
	else
	{
		free(ptr->pixelDatas);
		return 2;
	}
	return 0;
}

unsigned char UI_pic_cut(const char *filename, RectInfo *ptr,
						 u16 x1, u16 y1, u16 x2, u16 y2)
{
	u8 type = 0, res = 0;

	type = f_typetell((u8 *)filename);
	if(type==T_BMP)
		res = bmp_decode_cut(filename, ptr, x1, y1, x2, y2); //解码bmp
	else if(type==T_PNG)
		res = png_decode_cut(filename, ptr, x1, y1, x2, y2); //解码PNG
	else if(type==T_JPG)
		res = jpg_decode_cut(filename, ptr, x1, y1, x2, y2); //解码jpg
	else
		res = 0X27; //非图片格式!!!
	return res;
}

unsigned char UI_pic(const char *filename, RectInfo *ptr)
{
	u8 type = 0, res = 0;

	type = f_typetell((u8 *)filename);
	if(type==T_BMP)
		res = bmp_decode(filename, ptr); //解码bmp
	else if(type==T_PNG)
		res = png_decode(filename, ptr); //解码PNG
	else if(type==T_JPG)
		res = jpg_decode(filename, ptr); //解码jpg
	else
		res = 0X27; //非图片格式!!!
	return res;
}

/*****************************************
 * 控件管理器
**************************************/
_controlStruct controller_table[CONTROL_NUM];

void control_table_init(void)
{
	for (u8 i = 0; i < CONTROL_NUM; i++)
	{
		controller_table[i].x = 0;
		controller_table[i].y = 0;
		controller_table[i].state = 0; 
		controller_table[i].id = 0; 
		controller_table[i].layer = i;
		controller_table[i].rect.pixelDatas = NULL;
	}
	//0号初始化
	controller_table[0].x = 0;
	controller_table[0].y = 0;
	controller_table[0].state = 1; //0号永远为页面号
	controller_table[0].id = 0;	//0号自动分配
	controller_table[0].rect.alpha = LCD_disAdapt.alpha;
	controller_table[0].rect.crossWay = LCD_disAdapt.crossWay;
	controller_table[0].rect.format = LCD_disAdapt.format;
	controller_table[0].rect.height = LCD_disAdapt.height;
	controller_table[0].rect.pixelByte = LCD_disAdapt.pixelByte;
	controller_table[0].rect.pixelDatas = LCD_disAdapt.pixelDatas;
	controller_table[0].rect.totalPixels = LCD_disAdapt.totalPixels;
	controller_table[0].rect.width = LCD_disAdapt.width;
}

static u8 _layer_num = 0;
u8 control_table_check(u8 id)
{ //注意: 需在控件生成后使用
	u8 i = 0;

	for (i = 0; i < CONTROL_NUM; i++) //已生成
	{
		if (controller_table[i].id == id && controller_table[i].state == 1)
			return i; //返回table号
	}
	return 255; //没生成
}

u8 control_table_malloc(RectInfo *rect, u8 id, int x, int y)
{ //注意: 需在控件生成后使用
	u8 i = 0;

	for (i = 0; i < CONTROL_NUM; i++) //未生成
	{
		if (controller_table[i].state == 0)
		{
			controller_table[i].x = x;
			controller_table[i].y = y;
			controller_table[i].state = 1;
			controller_table[i].id = id;
			_layer_num++;
			controller_table[i].layer = _layer_num;
			controller_table[i].rect.alpha = rect->alpha;
			controller_table[i].rect.crossWay = rect->crossWay;
			controller_table[i].rect.format = rect->format;
			controller_table[i].rect.height = rect->height;
			controller_table[i].rect.pixelByte = rect->pixelByte;
			controller_table[i].rect.pixelDatas = rect->pixelDatas;
			controller_table[i].rect.totalPixels = rect->totalPixels;
			controller_table[i].rect.width = rect->width;
			return i; //返回table号
		}
	}
	free(rect->pixelDatas);
	return 255; //没空闲位
}

void control_table_free(u8 num)
{
	u8 i = 1;

    if(num==255) //release all
    {
        _layer_num = 0;
        for (i = 1; i < CONTROL_NUM; i++) //0页面号元素不清
		{
			controller_table[i].id = 0;
            controller_table[i].state = 0;
			free(controller_table[i].rect.pixelDatas);
			controller_table[i].rect.pixelDatas = NULL;
		}
    }
    else
    {
        if(num==0)
            return; //0页面号元素不清
		for (i = 1; i < CONTROL_NUM; i++)
		{
			if (controller_table[i].id == num)
			{
				controller_table[i].id = 0;
				_layer_num--;
				controller_table[i].state = 0;
				free(controller_table[i].rect.pixelDatas);
				return;
			}
		}
	}
}

//控件移动更新
void backRectRecovery(int x, int y, u16 width, u16 height)
{
	int cutX1 = 0, cutY1 = 0, cutX2 = 0, cutY2 = 0;

	if (x > lcddev.width - 1 || y > lcddev.height - 1)
		return; //正过界结束
	else if (width - (-x) <= 0 || height - (-y) <= 0)
		return; //负过界结束

	if (x < 0) //计算:cutX1 cutX2
		cutX1 = 0;
	else
		cutX1 = x;
	if (cutX1 + width - 1 > lcddev.width - 1) //cutX2超出显示
		cutX2 = lcddev.width - 1;
	else
		cutX2 = cutX1 + width - 1;

	if (y < 0) //计算:cutY1 cutY2
		cutY1 = 0;
	else
		cutY1 = y;
	if (cutY1 + height - 1 > lcddev.height - 1) //cutY2超出显示
		cutY2 = lcddev.height - 1;
	else
		cutY2 = cutY1 + height - 1;

	// printf("x: %d, y: %d, w: %d, h: %d\r\n", x, y, width, height);
	// printf("x1: %d, y1: %d, x2: %d, y2: %d\r\n", cutX1, cutY1, cutX2, cutY2);
	LCD_FillBackToFront(cutX1, cutY1, cutX2, cutY2);
}

void control_move(_controlStruct *controller, int x, int y)
{
	u16 LAx, LAy, LBx, LBy;
	int xA = 0, yA = 0, xB = 0, yB = 0;

	//移动不用恢复背景
	if (controller->rect.crossWay & 0X80)
	{
		controller->x = x; //记录更新
		controller->y = y;
		UI_disRegionCrossAdapt(&controller->rect, x, y);
		return;
	}

	//移动需要恢复背景
	if (controller->rect.crossWay == 3) //图像为透明时,需恢复整个图像背景,不能只恢复移动部分
	{
		backRectRecovery(controller->x, controller->y, (u16)controller->rect.width, \
							(u16)controller->rect.height);
	}
	else //计算移动矩形
	{
		if (controller->x > x)
		{
			xA = x + controller->rect.width;
			LAx = controller->x + controller->rect.width - xA;
		}
		else if (controller->x < x)
		{
			LAx = x - controller->x;
			xA = controller->x;
		}
		else
			LAx = 0;
		if (controller->rect.width < LAx)
		{
			LAx = controller->rect.width;
			xA = controller->x;
		}
		LAy = controller->rect.height;
		yA = controller->y;

		if (controller->y > y)
		{
			yB = y + controller->rect.height;
			LBy = controller->y - y + 1;
		}
		else if (controller->y < y)
		{
			yB = controller->y;
			LBy = y - controller->y;
		}
		else
			LBy = 0;
		if (controller->rect.height < LBy)
		{
			LBy = controller->rect.height;
			yB = controller->y;
		}
		if (controller->x > x)
		{
			xB = controller->x;
			LBx = controller->rect.width - LAx;
		}
		else if (controller->x < x)
		{
			xB = x;
			LBx = controller->rect.width - LAx;
		}
		else
		{
			xB = controller->x;
			LBx = controller->rect.width;
		}

		if (LAx != 0) //此轴没移动就不更
			backRectRecovery(xA, yA, LAx, LAy);
		if (LBy != 0) //此轴没移动就不更
		{
			//完全移动时(除x轴不动情况), 由X轴更了, 不用重复
			if ((controller->x + controller->rect.width - 1 >= x &&
				 controller->x <= x + controller->rect.width - 1) ||
				(controller->y + controller->rect.height - 1 >= y &&
				 controller->x <= x + controller->rect.width - 1))
				backRectRecovery(xB, yB, LBx, LBy);
		}
	}

	controller->x = x; //记录更新
	controller->y = y;

	UI_disRegionCrossAdapt(&controller->rect, x, y);
}

/******没测试,,功能保留********/
/* static ColorClass _UIColorSrc = 0, _UIColorDest = 0;
static u8 _UI_Color_Replace_EN = 0;
void UI_color_replace_set(u8 EN, ColorClass src, ColorClass dest)
{
	_UIColorSrc = src;
	_UIColorDest = dest;
	_UI_Color_Replace_EN = EN;
}

void UI_color_replace(RectInfo *rect)
{
	if (_UI_Color_Replace_EN == 0)
		return;
	for (int i = 0; i < rect->totalPixels; i++)
	{
		if (((ColorClass *)rect->pixelDatas)[i] == _UIColorSrc)
			((ColorClass *)rect->pixelDatas)[i] = _UIColorDest;
	}
} */
