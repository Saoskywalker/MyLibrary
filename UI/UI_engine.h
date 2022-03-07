#ifndef _UI_BASE_H
#define _UI_BASE_H

#include "lcd.h"
#include "types_plus.h"

/************************************
alpha blending算法
src 前景色
dest 背景色
返回处理色
**************************************/
static inline ColorClass alpha_blending(ColorClass *src, ColorClass *dest, u8 alpha)
{
	u8 R3, G3, B3;	 //result

	R3 = ((((*src&0X00FF0000)>>16) - ((*dest&0X00FF0000)>>16)) * alpha >> 8) + ((*dest&0X00FF0000)>>16);
	G3 = ((((*src&0X0000FF00)>>8) - ((*dest&0X0000FF00)>>8)) * alpha >> 8) + ((*dest&0X0000FF00)>>8);
	B3 = (((*src&0X000000FF) - (*dest&0X000000FF)) * alpha >> 8) + (*dest&0X000000FF);

	return 0XFF000000|(ColorClass)R3<<16|(ColorClass)G3<<8|(ColorClass)B3; //ALPHA暂不处理

/*另一个*/
//a1前景,,a2背景   
// u32 A = a1;
// u32 R = (((rgba1 >> 0 &0xffU) * A + (rgba2 >> 0 &0xffU) *(0xff - A)) >> 8)&0xffU;
// u32 G = (((rgba1 >> 8 &0xffU) * A + (rgba2 >> 8 &0xffU) *(0xff - A)) >> 8)&0xffU;
// u32 B = (((rgba1 >> 16&0xffU) * A + (rgba2 >> 16&0xffU) *(0xff - A)) >> 8)&0xffU;
// A = ((a1 * A + a2 *(0xff - A)) >> 8)&0xffU; // 必须对Alpha波段也处理
}

// typedef struct
// {
//     int width;
//     int height;
//     int channels;
//     unsigned char *imageData;
// }ZqImage;

/* 矩形基本信息 默认使用ARGB格式*/
typedef struct 
{
	int width;   /* 宽度: 一行有多少个象素 */
	int height;  /* 高度: 一列有多少个象素 */
	u8 alpha;  /* 整体的透明度 */
	u32 totalPixels; /* 总像素 */ 
	pixel_format_t format; /*图像像素使用的格式*/
	u8 pixelByte; /*单像素字节数*/
	void *pixelDatas;  /* 数据存储的地方 */
	 /*对于Slave图层交于Master时的混合方法
	   0bit:是否用部分显示
	   1bit:是否用透明
	   2bit:是否滤去背景色
	   7bit:移动控件时, 是否恢复背景*/
	u8 crossWay;
}RectInfo;

extern double adaptScaleY, adaptScaleX; //适应缩放系数
extern u8 LCD_disAdaptFlag; //自适应开启标志
extern RectInfo LCD_disAdapt; //适应屏幕用缩放层
extern RectInfo LCD_rectNow, LCD_rectBackup; //lcd rect
extern u8 LCD_renewRequire; //显示更新请求

//用于UI矩形层, 不能作LCD显示操作, 因不带显示方向功能
static inline ColorClass UI_readPoint(RectInfo *Rect, u16 x, u16 y)
{
	return ((ColorClass *)Rect->pixelDatas)[x + y * Rect->width];
}
//用于UI矩形层, 不能作LCD显示操作, 因不带显示方向功能
static inline void UI_drawPoint(RectInfo *Rect, u16 x, u16 y, ColorClass color)
{
	((ColorClass *)Rect->pixelDatas)[x + y * Rect->width] = color;
}

u8 UI_init(void);
u8 UI_disAdapt(int width, int height);
u8 UI_disRegionCrossAdapt(RectInfo *Slave, int x, int y);
u8 UI_disAdaptScale(RectInfo *bmpResult, RectInfo *bmpImg, double dy, double dx);
void UI_unload(void);
void UI_LCDBackupRenew(void);
void UI_LCDBackupDis(void);
// void UI_cacheRenew(void);
void UI_layerCopy(RectInfo *dest, RectInfo *src);
u8 UI_rectRegionCross(RectInfo *Master, RectInfo *Slave, int x, int y);
u8 UI_disRegionCross(RectInfo *Slave, int x, int y);
void UI_FillFrontToDest(RectInfo *dest, u16 x1, u16 y1);
u8 UI_imrotate(RectInfo *bmpResult, RectInfo *bmpImg, int Angle); //picture roll
u8 UI_imscale(RectInfo *bmpResult, RectInfo *bmpImg, double dy, double dx); //picture zoom
void GUI_DrawArc(int x0, int y0, int rx, int ry, int a0, int a1);

/*font show*/
void UI_showStr(RectInfo *pic, u8 *str, u8 size, u8 mode); //font widget
void UI_showStr2(RectInfo *RectF, RectInfo *RectB, u16 x, u16 y, u8 *str, u8 size, u8 mode);
void UI_showStrDefine1(RectInfo *RectF, RectInfo *RectB, 
						u16 x, u16 y, u8 *str, u8 num, u16 xy_size, u8 mode);
void Show_Str2(u16 x, u16 y, u8 *str, u8 size, u8 mode);

/*pic show*/
u8 gif_decode(const u8 *filename, u16 x, u16 y, RectInfo *rect); //解码一个gif文件
unsigned char png_decode(const char *filename, RectInfo *ptr);
unsigned char png_decode_cut(const char *filename, RectInfo *ptr, 
							u16 x1, u16 y1, u16 x2, u16 y2);
unsigned char bmp_decode(const char *filename, RectInfo *ptr);
unsigned char bmp_decode_cut(const char *filename, RectInfo *ptr, 
							u16 x1, u16 y1, u16 x2, u16 y2);
unsigned char UI_pic_cut(const char *filename, RectInfo *ptr,
						 u16 x1, u16 y1, u16 x2, u16 y2);
unsigned char UI_pic(const char *filename, RectInfo *ptr);

/*控件管理器*/
#define CONTROL_NUM 24
typedef struct {
    int x, y; //控件位置
    u8 id; //控件ID号
    u8 state; //控制表元素使用与否
    RectInfo rect;
    u8 layer; //层号, 层之间叠加前后关系
}_controlStruct;
extern _controlStruct controller_table[CONTROL_NUM];

void control_table_init(void);
u8 control_table_check(u8 id);
u8 control_table_malloc(RectInfo *rect, u8 id, int x, int y);
void control_table_free(u8 num);
void control_move(_controlStruct *controller, int x, int y);
void backRectRecovery(int x, int y, u16 width, u16 height);
// void UI_color_replace_set(u8 EN, ColorClass src, ColorClass dest);
// void UI_color_replace(RectInfo *rect);

//touch
extern int UI_pressTouchY, UI_pressTouchX, UI_freeTouchY, UI_freeTouchX;
extern char UI_pressState;
void UI_touchAdapt(int x, int y, u8 state);

//控件结构体

#endif
