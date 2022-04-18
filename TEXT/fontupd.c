#include "fontupd.h"
#include "ROM_port.h"
#include "lcd.h"
#include "string.h"
#include <malloc.h>
#include "delay.h"
#include "MTF_io.h"
#include "ff.h"

//用来保存字库基本信息，地址，大小等
_font_info ftinfo;

//字库存放在磁盘中的路径
u8 *const GBK24_PATH = "/SYSTEM/FONT/GBK24.Dzk"; //GBK24的存放位置
u8 *const GBK16_PATH = "/SYSTEM/FONT/GBK16.Dzk"; //GBK16的存放位置
u8 *const GBK12_PATH = "/SYSTEM/FONT/GBK12.Dzk"; //GBK12的存放位置

//显示当前字体更新进度
//x,y:坐标
//size:字体大小
//fsize:整个文件大小
//pos:当前文件指针位置
u32 fupd_prog(u16 x, u16 y, u8 size, u32 fsize, u32 pos)
{
	float prog;
	u8 t = 0XFF;

	prog = (float)pos / fsize;
	prog *= 100;
	if (t != prog)
	{
		LCD_ShowString(x + 3 * size / 2, y, 240, 320, size, (u8 *)"%");
		t = prog;
		if (t > 100)
			t = 100;
		LCD_ShowNum(x, y, t, 3, size); //显示数值
		LCD_Exec();
	}
	return 0;
}

//更新某一个
//x,y:坐标
//size:字体大小
//fxpath:路径
//fx:更新的内容;
//返回值:0,成功;其他,失败.
u8 updata_fontx(u16 x, u16 y, u8 size, u8 *fxpath, u8 fx)
{
	u32 flashaddr = 0;
	mFILE *fftemp = NULL;
	u8 *tempbuf;
	u8 res;
	UINT bread;
	u32 offx = 0;

	tempbuf = malloc(4096); //分配4096个字节空间
	if (tempbuf == NULL)
	{
		res = 1;
	}
	else
	{
		fftemp = MTF_open((const char *)fxpath, "rb");
		if (fftemp == NULL)
			res = 2; //打开文件失败
		else
			res = 0;
	}

	if (res == 0)
	{
		switch (fx)
		{
		case 1:
			ftinfo.f12addr = FONTINFOADDR + sizeof(ftinfo); //ftinfo之后，紧跟GBK12字库
			ftinfo.gbk12size = (u32)MTF_size(fftemp);					//GBK12字库大小
			flashaddr = ftinfo.f12addr;							//GBK12的起始地址
			break;
		case 2:
			ftinfo.f16addr = ftinfo.f12addr + ftinfo.gbk12size; //GBK12之后，紧跟GBK16字库
			ftinfo.gbk16size = (u32)MTF_size(fftemp);					//GBK16字库大小
			flashaddr = ftinfo.f16addr;							//GBK16的起始地址
			break;
		case 3:
			ftinfo.f24addr = ftinfo.f16addr + ftinfo.gbk16size; //GBK16之后，紧跟GBK24字库
			ftinfo.gkb24size = (u32)MTF_size(fftemp);					//GBK24字库大小
			flashaddr = ftinfo.f24addr;							//GBK24的起始地址
			break;
		default:
			break;
		}

		while (1) //死循环执行
		{
			bread = MTF_read(tempbuf, 1, 4096, fftemp); //读取数据
			if (MTF_error(fftemp) != 0)
				break;									   //执行错误
			MTF_ROM_write(tempbuf, offx + flashaddr, 4096); //从0开始写入4096个数据
			offx += bread;
			fupd_prog(x, y, size, (u32)MTF_size(fftemp), offx); //进度显示
			if (bread != 4096)
				break; //读完了.
		}
		MTF_close(fftemp);
	}
	
	free(tempbuf); //释放内存
	return res;
}

//更新字体文件GBK12,GBK16,GBK24一起更新
//x,y:提示信息的显示地址
//size:字体大小
//src:字库来源磁盘."0:",SD卡;"1:",FLASH盘,"2:",U盘.
//提示信息字体大小
//返回值:0,更新成功;
//		 其他,错误代码.
u8 update_font(u16 x, u16 y, u8 size, u8 *src)
{
	u8 *pname;
	u32 *buf;
	u8 res = 0;
	u16 i, j;
	mFILE *fftemp = NULL;
	u8 rval = 0;
	
	res = 0XFF;
	ftinfo.fontok = 0XFF;
	pname = malloc(100);			//申请100字节内存
	buf = malloc(4096);				//申请4K字节内存
	if (buf == NULL || pname == NULL)
	{
		free(pname);
		free(buf);
		return 5; //内存申请失败
	}

	//先查找文件是否正常
	strcpy((char *)pname, (char *)src); //copy src内容到pname
	strcat((char *)pname, (char *)GBK12_PATH);
	fftemp = MTF_open((const char *)pname, "rb");
	if (fftemp == NULL)
		rval |= 1 << 5; //打开文件失败
	else
		MTF_close(fftemp);
	strcpy((char *)pname, (char *)src); //copy src内容到pname
	strcat((char *)pname, (char *)GBK16_PATH);
	fftemp = MTF_open((const char *)pname, "rb");
	if (fftemp == NULL)
		rval |= 1 << 6; //打开文件失败
	else
		MTF_close(fftemp);
	strcpy((char *)pname, (char *)src); //copy src内容到pname
	strcat((char *)pname, (char *)GBK24_PATH);
	fftemp = MTF_open((const char *)pname, "rb");
	if (fftemp == NULL)
		rval |= 1 << 7; //打开文件失败
	else
		MTF_close(fftemp);

	if (rval == 0)			//字库文件都存在.
	{
		//先擦除字库区域,提高写入速度
		// #define FONTSECSIZE	 512
		// LCD_ShowString(x, y, 240, 320, size, (u8 *)"Erasing sectors... "); //提示正在擦除扇区
		// LCD_Exec();
		// for (i = 0; i < FONTSECSIZE; i++)							 
		// {
		// 	fupd_prog(x + 20 * size / 2, y, size, FONTSECSIZE, i);			  //进度显示
		// 	MTF_ROM_read((u8 *)buf, ((FONTINFOADDR / 4096) + i) * 4096, 4096); //读出整个扇区的内容
		// 	for (j = 0; j < 1024; j++)										  //校验数据
		// 	{
		// 		if (buf[j] != 0XFFFFFFFF)
		// 			break; //需要擦除
		// 	}
		// 	if (j != 1024)
		// 		W25QXX_Erase_Sector((FONTINFOADDR / 4096) + i); //需要擦除的扇区
		// }
		// free(buf);

		LCD_ShowString(x, y, 240, 320, size, (u8 *)"Updating GBK12.BIN  ");
		LCD_Exec();
		strcpy((char *)pname, (char *)src); //copy src内容到pname
		strcat((char *)pname, (char *)GBK12_PATH);
		res = updata_fontx(x + 20 * size / 2, y, size, pname, 1); //更新GBK12.Dzk
		if (res)
		{
			free(pname);
			return 2;
		}

		LCD_ShowString(x, y, 240, 320, size, (u8 *)"Updating GBK16.BIN  ");
		LCD_Exec();
		strcpy((char *)pname, (char *)src); //copy src内容到pname
		strcat((char *)pname, (char *)GBK16_PATH);
		res = updata_fontx(x + 20 * size / 2, y, size, pname, 2); //更新GBK16.Dzk
		if (res)
		{
			free(pname);
			return 3;
		}

		// LCD_ShowString(x, y, 240, 320, size, (u8 *)"Updating GBK24.BIN  ");
		// LCD_Exec();
		// strcpy((char *)pname, (char *)src); //copy src内容到pname
		// strcat((char *)pname, (char *)GBK24_PATH);
		// res = updata_fontx(x + 20 * size / 2, y, size, pname, 3); //更新GBK24.Dzk
		// if (res)
		// {
		// 	free(pname);
		// 	return 4;
		// }

		//全部更新好了
		ftinfo.fontok = 0XAA;
		MTF_ROM_write((u8 *)&ftinfo, FONTINFOADDR, sizeof(ftinfo)); //保存字库信息
	}
	free(pname); //释放内存
	free(buf);
	return rval; //无错误.
}

//初始化字体
//返回值:0,字库完好.
//		 其他,字库丢失
u8 font_init(void)
{
	u8 t = 0;

	//W25QXX_Init();
	while (t < 10) //连续读取10次,都是错误,说明确实是有问题,得更新字库了
	{
		t++;
		MTF_ROM_read((u8 *)&ftinfo, FONTINFOADDR, sizeof(ftinfo)); //读出ftinfo结构体数据
		if (ftinfo.fontok == 0XAA)
			break;
		delay_ms(20);
	}
	if (ftinfo.fontok != 0XAA)
		return 1;
	return 0;
}
