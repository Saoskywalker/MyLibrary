#include "piclib.h"
#include "bmp.h"
#include <string.h>
#include "MTF_io.h"

//////////////////////////////////////////用户配置区///////////////////////////////
#define BMP_USE_MALLOC 1   //定义是否使用malloc,这里我们选择使用malloc
#define BMP_DBUF_SIZE 1024*32 //定义bmp解码数组的大小(最少应为LCD宽度*3)
//////////////////////////////////////////////END/////////////////////////////////

//BMP信息头
typedef struct __attribute__((packed))
{
	u32 biSize;		   //说明BITMAPINFOHEADER结构所需要的字数。
	long biWidth;	  //说明图象的宽度，以象素为单位
	long biHeight;	 //说明图象的高度，以象素为单位
	u16 biPlanes;	  //为目标设备说明位面数，其值将总是被设为1
	u16 biBitCount;	//说明比特数/象素，其值为1、4、8、16、24、或32
	u32 biCompression; //说明图象数据压缩的类型。其值可以是下述值之一：
					   //BI_RGB：没有压缩；
					   //BI_RLE8：每个象素8比特的RLE压缩编码，压缩格式由2字节组成(重复象素计数和颜色索引)；
	//BI_RLE4：每个象素4比特的RLE压缩编码，压缩格式由2字节组成
	//BI_BITFIELDS：每个象素的比特由指定的掩码决定。
	u32 biSizeImage;	  //说明图象的大小，以字节为单位。当用BI_RGB格式时，可设置为0
	long biXPelsPerMeter; //说明水平分辨率，用象素/米表示
	long biYPelsPerMeter; //说明垂直分辨率，用象素/米表示
	u32 biClrUsed;		  //说明位图实际使用的彩色表中的颜色索引数
	u32 biClrImportant;   //说明对图象显示有重要影响的颜色索引的数目，如果是0，表示都重要。
} BITMAP_INFO_HEADER;
//BMP头文件
typedef struct __attribute__((packed))
{
	u16 bfType;		 //文件标志.只对'BM',用来识别BMP位图类型
	u32 bfSize;		 //文件大小,占四个字节
	u16 bfReserved1; //保留
	u16 bfReserved2; //保留
	u32 bfOffBits;   //从文件开始到位图数据(bitmap data)开始之间的的偏移量
} BITMAP_FILE_HEADER;
//彩色表
typedef struct __attribute__((packed))
{
	u8 rgbBlue;		//指定蓝色强度
	u8 rgbGreen;	//指定绿色强度
	u8 rgbRed;		//指定红色强度
	u8 rgbReserved; //保留，设置为0
} RGBQUAD;
//位图信息头
typedef struct __attribute__((packed))
{
	BITMAP_FILE_HEADER bmfHeader;
	BITMAP_INFO_HEADER bmiHeader;
	u32 RGB_MASK[3]; //调色板用于存放RGB掩码.
					 //RGBQUAD bmiColors[256];
} BITMAP_INFO;
typedef RGBQUAD *LPRGBQUAD; //彩色表

//图象数据压缩的类型
#define BI_RGB 0	   //没有压缩.RGB 5,5,5.
#define BI_RLE8 1	  //每个象素8比特的RLE压缩编码，压缩格式由2字节组成(重复象素计数和颜色索引)；
#define BI_RLE4 2	  //每个象素4比特的RLE压缩编码，压缩格式由2字节组成
#define BI_BITFIELDS 3 //每个象素的比特由指定的掩码决定。

//不使用内存分配
#if BMP_USE_MALLOC == 0
u8 bmpreadbuf[BMP_DBUF_SIZE];
#endif

//标准的bmp解码,解码filename这个BMP文件
//速度比较慢.主要
//filename:包含路径的文件名
//返回值:0,成功;
//		 其他,错误码.
u8 stdbmp_decode(const u8 *filename)
{
	mFILE *f_bmp = NULL;
	size_t fbr = 0;
	u16 count;
	u8 rgb, color_byte;
	u16 x, y;
	ColorClass color;
	u16 countpix = 0; //记录像素

	//x,y的实际坐标
	u16 realx = 0;
	u16 realy = 0;
	u8 yok = 1;
	u8 res;

	u8 *databuf = NULL;				  //数据读取存放地址
	size_t readlen = BMP_DBUF_SIZE; //一次从SD卡读取的字节数长度

	u8 *bmpbuf = NULL;			  //数据解码地址
	u8 biCompression = 0; //记录压缩方式

	u16 rowlen;		   //水平方向字节数
	BITMAP_INFO *pbmp; //临时指针

#if BMP_USE_MALLOC == 1					   //使用malloc
	databuf = (u8 *)pic_memalloc(readlen); //开辟readlen字节的内存区域
	if (databuf == NULL)
		return PIC_MEM_ERR;					  //内存申请失败.
#else //不使用malloc
	databuf = bmpreadbuf;
#endif

	f_bmp = MTF_open((const char *)filename, "rb"); //打开文件
	if (f_bmp == NULL)
	{
		pic_memfree(databuf);
		res = 4;
	}
	else
	{
		res = 0;
	}

	if (res == 0)										   //打开成功.
	{
		fbr = MTF_read(databuf, 1, readlen, f_bmp); //读出readlen个字节

		pbmp = (BITMAP_INFO *)databuf;				   //得到BMP的头部信息
		count = pbmp->bmfHeader.bfOffBits;			   //数据偏移,得到数据段的开始地址
		color_byte = pbmp->bmiHeader.biBitCount / 8;   //彩色位 16/24/32
		biCompression = pbmp->bmiHeader.biCompression; //压缩方式
		picinfo.ImgHeight = pbmp->bmiHeader.biHeight;  //得到图片高度
		picinfo.ImgWidth = pbmp->bmiHeader.biWidth;	//得到图片宽度

		ai_draw_init(); //初始化智能画图
		//水平像素必须是4的倍数!!
		if ((picinfo.ImgWidth * color_byte) % 4)
			rowlen = ((picinfo.ImgWidth * color_byte) / 4 + 1) * 4;
		else
			rowlen = picinfo.ImgWidth * color_byte;
		//开始解码BMP
		color = 0; //颜色清空
		x = 0;
		y = picinfo.ImgHeight;
		rgb = 0;
		//对于尺寸小于等于设定尺寸的图片,进行快速解码
		realy = (y * picinfo.Div_Fac) >> 13;
		bmpbuf = databuf;
		while (1)
		{
			//解码, 并填充至显存
			while (count < readlen) //读取一簇1024扇区 (SectorsPerClust 每簇扇区数)
			{
				if (color_byte == 3) //24位颜色图
				{
					switch (rgb)
					{
					case 0:
						color = (ColorClass)bmpbuf[count]; //B
						break;
					case 1:
						color += ((ColorClass)bmpbuf[count] << 8); //G
						break;
					case 2:
						color += ((ColorClass)bmpbuf[count] << 16); //R
						break;
					}
				}
				else if (color_byte == 2) //16位颜色图
				{
					switch (rgb)
					{
					case 0:
						if (biCompression == BI_RGB) //RGB:5,5,5
						{
							color = ((u16)bmpbuf[count] & 0X1F);		 //B
							color += (((u16)bmpbuf[count]) & 0XE0) << 1; //G
						}
						else //RGB:5,6,5
						{
							color = bmpbuf[count]; //G,B
						}
						break;
					case 1:
						if (biCompression == BI_RGB) //RGB:5,5,5
						{
							color += (u16)bmpbuf[count] << 9; //R,G
						}
						else //RGB:5,6,5
						{
							color += (u16)bmpbuf[count] << 8; //R,G
						}
						break;
					}
				}
				else if (color_byte == 4) //32位颜色图
				{
					switch (rgb)
					{
					case 0:
						color = bmpbuf[count] >> 3; //B
						break;
					case 1:
						color += ((u16)bmpbuf[count] << 3) & 0X07E0; //G
						break;
					case 2:
						color += ((u16)bmpbuf[count] << 8) & 0XF800; //R
						break;
					case 3:
						//alphabend=bmpbuf[count];//不读取  ALPHA通道
						break;
					}
				}
				else if (color_byte == 1) //8位色,暂时不支持,需要用到颜色表.
				{
				}
				rgb++;
				count++;
				if (rgb == color_byte) //水平方向读取到1像素数数据后显示
				{
					if (x < picinfo.ImgWidth)
					{
						realx = (x * picinfo.Div_Fac) >> 13;	   //x轴实际值
						if (is_element_ok(realx, realy, 1) && yok) //符合条件
						{
							//显示图片
							pic_phy.draw_point(realx + picinfo.S_XOFF, realy + picinfo.S_YOFF - 1, color); 
						}
					}
					x++; //x轴增加一个像素
					color = 0x00;
					rgb = 0;
				}
				countpix++;				//字节累加
				if (countpix >= rowlen) //水平方向字节值到了.换行
				{
					y--;
					if (y == 0)
					{
						break;
					}
					realy = (y * picinfo.Div_Fac) >> 13; //实际y值改变
					if (is_element_ok(realx, realy, 0))
						yok = 1; //此处不改变picinfo.staticx,y的值
					else
						yok = 0;
					x = 0;
					countpix = 0;
					color = 0x00;
					rgb = 0;
				}
			}
			
			//读出数据
			fbr = MTF_read(databuf, 1, readlen, f_bmp); //读出readlen个字节
			res = 0; //MTF_error(f_bmp);
			if (fbr != readlen)
				readlen = fbr; //最后一批数据

			if (fbr == 0) //读取出错, 或读至文件末尾
			{
				break; //退出循环, 关闭文件
			}

			bmpbuf = databuf;
			count = 0;
		}
		MTF_close(f_bmp); //关闭文件
	}
#if BMP_USE_MALLOC == 1 //使用malloc
	pic_memfree(databuf);
#endif
	return res; //BMP显示结束.
}

//小尺寸的bmp解码,解码filename这个BMP文件
//filename:包含路径的文件名
//x,y,width,height:显示区域大小(在区域正中央显示)
//acolor:附加的alphablend的颜色(这个仅对32位色bmp有效!!!)
//mode:模式(除了bit5,其他的均只对32位色bmp有效!!!)
//     bit[7:6]:0,仅使用图片本身和底色alphablend;
//              1,仅图片和acolor进行alphablend,并且不适用附加的透明度;
//              2,底色,acolor,图片,一起进行alphablend;
//	   bit5:保留
//     bit4~0:0~31,使用附加alphablend的透明程度
//返回值:0,成功;
//    其他,错误码.
#if 0

u8 minibmp_decode(u8 *filename, u16 x, u16 y, u16 width, u16 height, u16 acolor, u8 mode) //尺寸小于240*320的bmp图片解码.
{
	mFILE *f_bmp = NULL;
	size_t fbr = 0;
	u8 color_byte;
	u16 tx, ty, color;
	//tx,ty的实际坐标
	u8 res;
	u16 i, j;
	u8 *databuf = NULL;				 //数据读取存                                                                       放地址
	u16 readlen = BMP_DBUF_SIZE; //一次从SD卡读取的字节数长度,不能小于LCD宽度*3!!!

	u8 *bmpbuf = NULL;			  //数据解码地址
	u8 biCompression = 0; //记录压缩方式

	u16 rowcnt;		//一次读取的行数
	u16 rowlen;		//水平方向字节数
	u16 rowpix = 0; //水平方向像素数
	u8 rowadd;		//每行填充字节数

	u16 tmp_color;

	u8 alphabend = 0xff;	  //代表透明色为0，完全不透明
	u8 alphamode = mode >> 6; //得到模式值,0/1/2
	BITMAP_INFO *pbmp;		  //临时指针
	//得到窗体尺寸
	picinfo.S_Height = height;
	picinfo.S_Width = width;

#if BMP_USE_MALLOC == 1					   //使用malloc
	databuf = (u8 *)pic_memalloc(readlen); //开辟readlen字节的内存区域
	if (databuf == NULL)
		return PIC_MEM_ERR;					  //内存申请失败.
#else
	databuf = bmpreadbuf;
#endif
	f_bmp = MTF_open((const char *)filename, "rb"); //打开文件
	if (f_bmp == NULL)
	{
		pic_memfree(databuf);
		res = 4;
	}
	else
	{
		res = 0;
	}

	if (res == 0)										   //打开成功.
	{
		MTF_read(databuf, 1, sizeof(BITMAP_INFO), f_bmp); //读出BITMAPINFO信息
		pbmp = (BITMAP_INFO *)databuf;					   //得到BMP的头部信息
		color_byte = pbmp->bmiHeader.biBitCount / 8;	   //彩色位 16/24/32
		biCompression = pbmp->bmiHeader.biCompression;	 //压缩方式
		picinfo.ImgHeight = pbmp->bmiHeader.biHeight;	  //得到图片高度
		picinfo.ImgWidth = pbmp->bmiHeader.biWidth;		   //得到图片宽度
		//水平像素必须是4的倍数!!
		if ((picinfo.ImgWidth * color_byte) % 4)
			rowlen = ((picinfo.ImgWidth * color_byte) / 4 + 1) * 4;
		else
			rowlen = picinfo.ImgWidth * color_byte;
		rowadd = rowlen - picinfo.ImgWidth * color_byte; //每行填充字节数
														 //开始解码BMP
		color = 0;										 //颜色清空
		tx = 0;
		ty = picinfo.ImgHeight - 1;
		if (picinfo.ImgWidth <= picinfo.S_Width && picinfo.ImgHeight <= picinfo.S_Height)
		{
			x += (picinfo.S_Width - picinfo.ImgWidth) / 2;   //偏移到正中央
			y += (picinfo.S_Height - picinfo.ImgHeight) / 2; //偏移到正中央
			rowcnt = readlen / rowlen;						 //一次读取的行数
			readlen = rowcnt * rowlen;						 //一次读取的字节数
			rowpix = picinfo.ImgWidth;						 //水平像素数就是宽度
			MTF_seek(f_bmp, pbmp->bmfHeader.bfOffBits, SEEK_SET);		 //偏移到数据起始位置
			while (1)
			{
				fbr = MTF_read(databuf, 1, readlen, f_bmp); //读出readlen个字节
				res = 0;
				bmpbuf = databuf;							 //数据首地址
				if (fbr != readlen)
					rowcnt = fbr / rowlen; //最后剩下的行数
					
				if (color_byte == 3)	   //24位BMP图片
				{
					for (j = 0; j < rowcnt; j++) //每次读到的行数
					{
						for (i = 0; i < rowpix; i++) //写一行像素
						{
							color = (*bmpbuf++) >> 3;				   //B
							color += ((u16)(*bmpbuf++) << 3) & 0X07E0; //G
							color += (((u16)*bmpbuf++) << 8) & 0XF800; //R
							pic_phy.draw_point(x + tx, y + ty, color); //显示图片
							tx++;
						}
						bmpbuf += rowadd; //跳过填充区
						tx = 0;
						ty--;
					}
				}
				else if (color_byte == 2) //16位BMP图片
				{
					for (j = 0; j < rowcnt; j++) //每次读到的行数
					{
						if (biCompression == BI_RGB) //RGB:5,5,5
						{
							for (i = 0; i < rowpix; i++)
							{
								color = ((u16)*bmpbuf & 0X1F);			   //R
								color += (((u16)*bmpbuf++) & 0XE0) << 1;   //G
								color += ((u16)*bmpbuf++) << 9;			   //R,G
								pic_phy.draw_point(x + tx, y + ty, color); //显示图片
								tx++;
							}
						}
						else //RGB 565
						{
							for (i = 0; i < rowpix; i++)
							{
								color = *bmpbuf++;						   //G,B
								color += ((u16)*bmpbuf++) << 8;			   //R,G
								pic_phy.draw_point(x + tx, y + ty, color); //显示图片
								tx++;
							}
						}
						bmpbuf += rowadd; //跳过填充区
						tx = 0;
						ty--;
					}
				}
				else if (color_byte == 4) //32位BMP图片
				{
					for (j = 0; j < rowcnt; j++) //每次读到的行数
					{
						for (i = 0; i < rowpix; i++)
						{
							color = (*bmpbuf++) >> 3;				   //B
							color += ((u16)(*bmpbuf++) << 3) & 0X07E0; //G
							color += (((u16)*bmpbuf++) << 8) & 0XF800; //R
							alphabend = *bmpbuf++;					   //ALPHA通道
							if (alphamode != 1)						   //需要读取底色
							{
								tmp_color = pic_phy.read_point(x + tx, y + ty); //读取颜色
								if (alphamode == 2)								//需要附加的alphablend
								{
									tmp_color = piclib_alpha_blend(tmp_color, acolor, mode & 0X1F); //与指定颜色进行blend
								}
								color = piclib_alpha_blend(tmp_color, color, alphabend / 8); //和底色进行alphablend
							}
							else
								tmp_color = piclib_alpha_blend(acolor, color, alphabend / 8); //与指定颜色进行blend
							pic_phy.draw_point(x + tx, y + ty, color);						  //显示图片
							tx++;															  //x轴增加一个像素
						}
						bmpbuf += rowadd; //跳过填充区
						tx = 0;
						ty--;
					}
				}

				if (fbr != readlen)
					break; //退出循环
			}
		}
		MTF_close(f_bmp); //关闭文件
	}
	else
	{
		res = PIC_SIZE_ERR; //图片尺寸错误
	}
#if BMP_USE_MALLOC == 1		//使用malloc
	pic_memfree(databuf);
#endif
	return res;
}

#endif

//BMP编码函数
//将当前LCD屏幕的指定区域截图,存为16位格式的BMP文件 RGB565格式.
//保存为rgb565则需要掩码,需要利用原来的调色板位置增加掩码.这里我们已经增加了掩码.
//保存为rgb555格式则需要颜色转换,耗时间比较久,所以保存为565是最快速的办法.
//filename:存放路径
//x,y:在屏幕上的起始坐标
//返回值:0,成功;其他,错误码.
u8 bmp_encode(u8 *filename, u16 x, u16 y, u16 width, u16 height)
{
	mFILE *f_bmp = NULL;
	size_t fbr = 0;
	u16 bmpheadsize;  //bmp头大小
	BITMAP_INFO hbmp; //bmp头
	u8 res = 0;
	u16 tx, ty;   //图像尺寸
	u16 *databuf = NULL; //数据缓存区地址
	u16 pixcnt;   //像素计数器
	u16 bi4width; //水平像素字节数
	if (width == 0 || height == 0)
		return PIC_WINDOW_ERR; //区域错误
	if ((x + width - 1) > lcddev.width)
		return PIC_WINDOW_ERR; //区域错误
	if ((y + height - 1) > lcddev.height)
		return PIC_WINDOW_ERR; //区域错误

#if BMP_USE_MALLOC == 1					 //使用malloc
	databuf = (u16 *)pic_memalloc(1024); //开辟至少bi4width大小的字节的内存区域 ,对240宽的屏,480个字节就够了.
	if (databuf == NULL)
		return PIC_MEM_ERR;					  //内存申请失败.
#else
	databuf = (u16 *)bmpreadbuf;
#endif
	bmpheadsize = sizeof(hbmp);																					   //得到bmp文件头的大小
	memset((u8 *)&hbmp, 0, sizeof(hbmp));																		   //置零空申请到的内存.
	hbmp.bmiHeader.biSize = sizeof(BITMAP_INFO_HEADER);															   //信息头大小
	hbmp.bmiHeader.biWidth = width;																				   //bmp的宽度
	hbmp.bmiHeader.biHeight = height;																			   //bmp的高度
	hbmp.bmiHeader.biPlanes = 1;																				   //恒为1
	hbmp.bmiHeader.biBitCount = 16;																				   //bmp为16位色bmp
	hbmp.bmiHeader.biCompression = BI_BITFIELDS;																   //每个象素的比特由指定的掩码决定。
	hbmp.bmiHeader.biSizeImage = hbmp.bmiHeader.biHeight * hbmp.bmiHeader.biWidth * hbmp.bmiHeader.biBitCount / 8; //bmp数据区大小

	hbmp.bmfHeader.bfType = ((u16)'M' << 8) + 'B';					  //BM格式标志
	hbmp.bmfHeader.bfSize = bmpheadsize + hbmp.bmiHeader.biSizeImage; //整个bmp的大小
	hbmp.bmfHeader.bfOffBits = bmpheadsize;							  //到数据区的偏移

	hbmp.RGB_MASK[0] = 0X00F800; //红色掩码
	hbmp.RGB_MASK[1] = 0X0007E0; //绿色掩码
	hbmp.RGB_MASK[2] = 0X00001F; //蓝色掩码

	if ((hbmp.bmiHeader.biWidth * 2) % 4)										//水平像素(字节)不为4的倍数
	{
		bi4width = ((hbmp.bmiHeader.biWidth * 2) / 4 + 1) * 4; //实际要写入的宽度像素,必须为4的倍数.
	}
	else
		bi4width = hbmp.bmiHeader.biWidth * 2; //刚好为4的倍数

	//写入
	f_bmp = MTF_open((const char *)filename, "wb+");
	if (f_bmp == NULL)
	{
		pic_memfree(databuf);
		res = 4;
	}
	else
	{
		res = 0;
	}

	if (res == 0)						   //创建成功
	{
		fbr = MTF_write(&hbmp, 1, bmpheadsize, f_bmp);
		if (fbr == bmpheadsize)
			res = 0;
		else
			res = 1;

		if (res == 0)
		{
			for (ty = y + height - 1; hbmp.bmiHeader.biHeight; ty--)
			{
				pixcnt = 0;
				for (tx = x; pixcnt != (bi4width / 2);)
				{
					if (pixcnt < hbmp.bmiHeader.biWidth)
						databuf[pixcnt] = LCD_ReadPoint(tx, ty); //读取坐标点的值
					else
						databuf[pixcnt] = 0Xffff; //补充白色的像素.
					pixcnt++;
					tx++;
				}
				hbmp.bmiHeader.biHeight--;
				fbr = MTF_write(databuf, 1, bi4width, f_bmp);
				if (fbr == bi4width)
				{
					res = 0;
				}
				else
				{
					res = 1;
					break;
				}
			}
		}
		MTF_close(f_bmp);
	}
#if BMP_USE_MALLOC == 1 //使用malloc
	pic_memfree(databuf);
#endif
	return res;
}

//bmp解码: 支持8,16,24,32位无压缩解码
//out:输出指针, w图宽, h图高
//filename:包含路径的文件名
//返回值:0,成功;
//其他,错误码.
u8 lodebmp_decode(unsigned char **out, unsigned *w,
				  unsigned *h, const char *filename)
{
	mFILE *f_bmp = NULL;
	size_t fbr = 0;
	u16 count = 0;
	u8 rgb, color_byte;
	u16 x, y;
	ColorClass color;
	u16 countpix = 0; //记录像素
	ColorClass *data = NULL;
	u8 res;

	u8 *databuf = NULL;				  //数据读取存放地址
	size_t readlen = BMP_DBUF_SIZE; //一次从SD卡读取的字节数长度

	u8 *bmpbuf = NULL;			  //数据解码地址
	u8 biCompression = 0; //记录压缩方式

	u16 rowlen;		   //水平方向字节数
	BITMAP_INFO *pbmp; //临时指针

	u8 isIncludePalette = 1; //调色板
	RGBQUAD *palettePtr = NULL;
	u32 paletteNums = 0;

	databuf = (u8 *)pic_memalloc(readlen); //开辟readlen字节的内存区域
	if (databuf == NULL)
		return PIC_MEM_ERR;					  //内存申请失败.

	f_bmp = MTF_open((const char *)filename, "rb"); //打开文件
	if (f_bmp == NULL)
	{
		pic_memfree(databuf);
		res = 4;
	}
	else
	{
		res = 0;
	}

	if (res == 0)										   //打开成功.
	{
		fbr = MTF_read(databuf, 1, readlen, f_bmp); //读出readlen个字节

		pbmp = (BITMAP_INFO *)databuf;				   //得到BMP的头部信息
		color_byte = pbmp->bmiHeader.biBitCount / 8;   //彩色位 16/24/32
		biCompression = pbmp->bmiHeader.biCompression; //压缩方式
		*h = pbmp->bmiHeader.biHeight;				   //得到图片高度
		*w = pbmp->bmiHeader.biWidth;				   //得到图片宽度
		/* 验证"BM" 和 正负向扫描(正向扫描不能使用压缩)*/
		if (pbmp->bmfHeader.bfType!=0X4D42||pbmp->bmiHeader.biHeight<0) 
		{
			pic_memfree(databuf);
			MTF_close(f_bmp); //关闭文件
			return 2; //文件格式不兼容
		}
		data = (ColorClass *)pic_memalloc(pbmp->bmiHeader.biHeight * pbmp->bmiHeader.biWidth * 4);
		if (data == NULL)
		{
			pic_memfree(databuf);
			MTF_close(f_bmp); //关闭文件
			return PIC_MEM_ERR;
		}

		/* 是否存在调色板 */
		if (pbmp->bmfHeader.bfOffBits == sizeof(BITMAP_FILE_HEADER) + sizeof(BITMAP_INFO_HEADER))
		{
			isIncludePalette = 0;
		}
		// printf("size: %d, %d, PALETTE: %d\r\n", sizeof(BITMAP_FILE_HEADER),
		// 		sizeof(BITMAP_INFO_HEADER), isIncludePalette);

		/* 为调色板申请内存 */
		if (isIncludePalette == 1)
		{
			paletteNums = pbmp->bmfHeader.bfOffBits - \
						  sizeof(BITMAP_FILE_HEADER) - sizeof(BITMAP_INFO_HEADER);

			palettePtr = (RGBQUAD *)pic_memalloc(paletteNums);
			if (palettePtr == NULL)
			{
				pic_memfree(data);
				pic_memfree(databuf);
				MTF_close(f_bmp); //关闭文件
				return PIC_MEM_ERR;
			}
		}

		/* 读取调色板数据 */
		if (isIncludePalette == 1)
		{
			MTF_seek(f_bmp, sizeof(BITMAP_FILE_HEADER) + sizeof(BITMAP_INFO_HEADER), SEEK_SET);
			fbr = MTF_read(palettePtr, 1, paletteNums, f_bmp);
			if (paletteNums == fbr)
				res = 0;
			else
				res = 7;
			if (res != 0)
			{
				MTF_close(f_bmp);
				pic_memfree(palettePtr);
				pic_memfree(data);
				pic_memfree(databuf);
				return 1; //读取出错
			}
		}

		//水平像素必须是4的倍数!!
		if ((*w * color_byte) % 4)
			rowlen = ((*w * color_byte) / 4 + 1) * 4;
		else
			rowlen = *w * color_byte;
		//开始解码BMP
		color = 0; //颜色清空
		x = 0;
		y = *h - 1;
		rgb = 0;
		bmpbuf = databuf;
		MTF_seek(f_bmp, pbmp->bmfHeader.bfOffBits, SEEK_SET); //数据偏移,数据段的开始
		fbr = MTF_read(databuf, 1, readlen, f_bmp);	 //读出readlen个字节
		if (readlen == fbr)
			res = 0;
		else
			res = 7;
		if (res || fbr == 0)
		{
			MTF_close(f_bmp);
			pic_memfree(data);
			pic_memfree(databuf);
			if (isIncludePalette == 1)
				pic_memfree(palettePtr);
			return 1; //读取出错
		}
		readlen = fbr;
		while (1)
		{
			//解码, 并填充至显存
			while (count < readlen) //读取一簇1024扇区 (SectorsPerClust 每簇扇区数)
			{
				if (color_byte == 3) //24位颜色图
				{
					switch (rgb)
					{
					case 0:
						color = (ColorClass)bmpbuf[count]; //B
						break;
					case 1:
						color += ((ColorClass)bmpbuf[count] << 8); //G
						break;
					case 2:
						color += ((ColorClass)bmpbuf[count] << 16); //R
						color |= 0xFF000000; //ALPHA通道:不透明
						break;
					}
				}
				else if (color_byte == 2) //16位颜色图
				{
					switch (rgb)
					{
					case 0:
						if (biCompression == BI_RGB) //RGB:5,5,5
						{
							color = ((u16)bmpbuf[count] & 0X1F);		 //B
							color += (((u16)bmpbuf[count]) & 0XE0) << 1; //G
						}
						else //RGB:5,6,5
						{
							color = bmpbuf[count]; //G,B
						}
						break;
					case 1:
						if (biCompression == BI_RGB) //RGB:5,5,5
						{
							color += (u16)bmpbuf[count] << 9; //R,G
						}
						else //RGB:5,6,5
						{
							color += (u16)bmpbuf[count] << 8; //R,G
						}
						break;
					}
				}
				else if (color_byte == 4) //32位颜色图
				{
					switch (rgb)
					{
					case 0:
						color = (ColorClass)bmpbuf[count]; //B
						break;
					case 1:
						color += ((ColorClass)bmpbuf[count] << 8); //G
						break;
					case 2:
						color += ((ColorClass)bmpbuf[count] << 16); //R
						break;
					case 3:
						color += ((ColorClass)bmpbuf[count] << 24); //ALPHA通道
						break;
					}
				}
				else if (color_byte == 1) //8位色,需要用到颜色表.
				{
					if (biCompression == BI_RGB)
					{
						color = *(ColorClass *)(palettePtr + bmpbuf[count]);
						color |= 0XFF000000; //可不用,和图片有关
					}
				}
				rgb++;
				count++;
				if (rgb == color_byte) //水平方向读取到1像素数数据后显示
				{
					if (x < *w)
					{
						data[x + y * (*w)] = color; //显示图片
					}
					x++; //x轴增加一个像素
					color = 0;
					rgb = 0;
				}
				countpix++;				//字节累加
				if (countpix >= rowlen) //水平方向字节值到了.换行
				{
					if (y == 0)
					{
						break;
					}
					y--;
					x = 0;
					countpix = 0;
					color = 0;
					rgb = 0;
				}
			}

			//读出数据
			fbr = MTF_read(databuf, 1, readlen, f_bmp); //读出readlen个字节
			res = 0; //MTF_error(f_bmp);
			if (fbr != readlen)
				readlen = fbr; //最后一批数据

			if (fbr == 0) //读取出错, 或读至文件末尾
			{
				break; //退出循环, 关闭文件
			}

			bmpbuf = databuf;
			count = 0;
		}
		MTF_close(f_bmp); //关闭文件
	}
	pic_memfree(databuf);
	if (isIncludePalette == 1)
		pic_memfree(palettePtr);
	*out = (unsigned char *)data;
	return res; //BMP显示结束
}

//bmp解码(截图): 支持8,16,24,32位无压缩解码
//out:输出指针, w图宽, h图高
//filename:包含路径的文件名
//x1,y1,x2,y2:截图对角坐标
//返回值:0,成功;
//其他,错误码.
u8 lodebmp_decode_cut(unsigned char **out, const char *filename, u16 x1, u16 y1, u16 x2, u16 y2)
{
	mFILE *f_bmp = NULL;
	size_t fbr = 0;
	u16 count = 0;
	u8 rgb = 0, color_byte = 0;
	u16 x = 0, y = 0, w = 0, h = 0, yTemp = 0;
	ColorClass color = 0;
	ColorClass *data = NULL;
	u8 res = 0;
	// size_t ppp = 0;
	u8 *iioo = NULL;

	u8 *databuf = NULL;				  //数据读取存放地址
	size_t readlen = BMP_DBUF_SIZE; //一次从SD卡读取的字节数长度

	u8 *bmpbuf = NULL;			  //数据解码地址
	u8 biCompression = 0; //记录压缩方式

	u16 rowlen = 0;		   //水平方向字节数
	BITMAP_INFO *pbmp = NULL; //临时指针

	u8 isIncludePalette = 1; //调色板
	RGBQUAD *palettePtr = NULL;
	u32 paletteNums = 0;

	databuf = (u8 *)pic_memalloc(readlen); //开辟readlen字节的内存区域
	if (databuf == NULL)
		return PIC_MEM_ERR;					  //内存申请失败.

	f_bmp = MTF_open((const char *)filename, "rb"); //打开文件
	if (f_bmp == NULL)
	{
		pic_memfree(databuf);
		res = 4;
	}
	else
	{
		res = 0;
	}

	if (res == 0)										   //打开成功.
	{
		fbr = MTF_read(databuf, 1, readlen, f_bmp); //读出readlen个字节

		pbmp = (BITMAP_INFO *)databuf;				   //得到BMP的头部信息
		color_byte = pbmp->bmiHeader.biBitCount / 8;   //彩色位 16/24/32
		biCompression = pbmp->bmiHeader.biCompression; //压缩方式
		h = y2-y1+1;				  
		w = x2-x1+1;				 
		/* 验证"BM" 和 正负向扫描(正向扫描不能使用压缩)*/
		if (pbmp->bmfHeader.bfType!=0X4D42||pbmp->bmiHeader.biHeight<0) 
		{
			pic_memfree(databuf);
			MTF_close(f_bmp); //关闭文件
			return 2; //文件格式不兼容
		}
		data = (ColorClass *)pic_memalloc(w * h * 4);
		if (data == NULL)
		{
			pic_memfree(databuf);
			MTF_close(f_bmp); //关闭文件
			return PIC_MEM_ERR;
		}

		/* 是否存在调色板 */
		if (pbmp->bmfHeader.bfOffBits == sizeof(BITMAP_FILE_HEADER) + sizeof(BITMAP_INFO_HEADER))
		{
			isIncludePalette = 0;
		}
		// printf("size: %d, %d, PALETTE: %d\r\n", sizeof(BITMAP_FILE_HEADER),
		// 		sizeof(BITMAP_INFO_HEADER), isIncludePalette);

		/* 为调色板申请内存 */
		if (isIncludePalette == 1)
		{
			paletteNums = pbmp->bmfHeader.bfOffBits - \
						  sizeof(BITMAP_FILE_HEADER) - sizeof(BITMAP_INFO_HEADER);

			palettePtr = (RGBQUAD *)pic_memalloc(paletteNums);
			if (palettePtr == NULL)
			{
				pic_memfree(data);
				pic_memfree(databuf);
				MTF_close(f_bmp); //关闭文件
				return PIC_MEM_ERR;
			}
		}

		/* 读取调色板数据 */
		if (isIncludePalette == 1)
		{
			MTF_seek(f_bmp, sizeof(BITMAP_FILE_HEADER) + sizeof(BITMAP_INFO_HEADER), SEEK_SET);
			fbr = MTF_read(palettePtr, 1, paletteNums, f_bmp);
			if (paletteNums == fbr)
				res = 0;
			else
				res = 7;
			if (res != 0)
			{
				MTF_close(f_bmp);
				pic_memfree(palettePtr);
				pic_memfree(data);
				pic_memfree(databuf);
				return 1; //读取出错
			}
		}

		//水平像素必须是4的倍数!!
		if ((pbmp->bmiHeader.biWidth * color_byte) % 4)
			rowlen = ((pbmp->bmiHeader.biWidth * color_byte) / 4 + 1) * 4;
		else
			rowlen = pbmp->bmiHeader.biWidth * color_byte;
		//开始解码BMP
		color = 0; //颜色清空
		x = 0;
		y = h;
		yTemp = y2+1;
		rgb = 0;
		bmpbuf = databuf;
		readlen = 0;
		iioo = (u8 *)pic_memalloc(w*color_byte);
		if(iioo==NULL)
		{
			MTF_close(f_bmp);
			if (isIncludePalette == 1)
				pic_memfree(palettePtr);
			pic_memfree(data);
			pic_memfree(databuf);
			return PIC_MEM_ERR; 
		}
		readlen = w*color_byte;
		bmpbuf = iioo;
		count = readlen;
		while (1)
		{
			//解码, 并填充至显存
			while (count < readlen) 
			{
				if (color_byte == 3) //24位颜色图
				{
					switch (rgb)
					{
					case 0:
						color = (ColorClass)bmpbuf[count]; //B
						break;
					case 1:
						color += ((ColorClass)bmpbuf[count] << 8); //G
						break;
					case 2:
						color += ((ColorClass)bmpbuf[count] << 16); //R
						color |= 0xFF000000; //ALPHA通道:不透明
						break;
					}
/* 					color = 0xFF000000|(ColorClass)bmpbuf[count]|((ColorClass)bmpbuf[count+1] << 8)| \
								((ColorClass)bmpbuf[count+2] << 16); //B
					count += 2;
					rgb += 2; */
				}
				else if (color_byte == 2) //16位颜色图
				{
					switch (rgb)
					{
					case 0:
						if (biCompression == BI_RGB) //RGB:5,5,5
						{
							color = ((u16)bmpbuf[count] & 0X1F);		 //B
							color += (((u16)bmpbuf[count]) & 0XE0) << 1; //G
						}
						else //RGB:5,6,5
						{
							color = bmpbuf[count]; //G,B
						}
						break;
					case 1:
						if (biCompression == BI_RGB) //RGB:5,5,5
						{
							color += (u16)bmpbuf[count] << 9; //R,G
						}
						else //RGB:5,6,5
						{
							color += (u16)bmpbuf[count] << 8; //R,G
						}
						break;
					}
				}
				else if (color_byte == 4) //32位颜色图
				{
					switch (rgb)
					{
					case 0:
						color = (ColorClass)bmpbuf[count]; //B
						break;
					case 1:
						color += ((ColorClass)bmpbuf[count] << 8); //G
						break;
					case 2:
						color += ((ColorClass)bmpbuf[count] << 16); //R
						break;
					case 3:
						color += ((ColorClass)bmpbuf[count] << 24); //ALPHA通道
						break;
					}
				}
				else if (color_byte == 1) //8位色,需要用到颜色表.
				{
					if (biCompression == BI_RGB)
					{
						color = *(ColorClass *)(palettePtr + bmpbuf[count]);
						color |= 0XFF000000; //可不用,和图片有关
					}
				}
				rgb++;
				count++;
				if (rgb == color_byte) //水平方向读取到1像素数数据后显示
				{
					if (x < w)
					{
						data[x + y * w] = color; //显示图片
						// if(yTemp==120)
						// 	printf("%#X, ", color);
					}
					x++; //x轴增加一个像素
					color = 0;
					rgb = 0;
				}
				if (x >= w) //换行
				{
					x = 0;
					color = 0;
					rgb = 0;
					break;
				}
			}

			//读出数据
			if (y == 0)
				break; //结束
			y--;
			yTemp--;
			MTF_seek(f_bmp, (pbmp->bmiHeader.biHeight-1-yTemp)*rowlen+x1* \
						color_byte+pbmp->bmfHeader.bfOffBits, SEEK_SET); //数据偏移
			fbr = MTF_read(iioo, 1, readlen, f_bmp); //读出readlen个字节
			res = 0; //MTF_error(f_bmp);

			if (fbr != readlen || fbr == 0) //读取出错, 或读至文件末尾
				break;						//退出循环, 关闭文件

			count = 0;
		}
		MTF_close(f_bmp); //关闭文件
	}
	pic_memfree(iioo);
	pic_memfree(databuf);
	if (isIncludePalette == 1)
		pic_memfree(palettePtr);
	*out = (unsigned char *)data;
	return res; //BMP显示结束
}
