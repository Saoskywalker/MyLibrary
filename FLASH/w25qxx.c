#include "w25qxx.h"
#include "string.h"
#include "stdint.h"
#include <types_plus.h>
#include <gpio-f1c100s.h>
#include <stdio.h>

#define w25qxx_debug(...) printf(__VA_ARGS__)
// #define w25qxx_debug(...)

// #include "delay.h"
////////////////////////////////////////////////////////////////
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//W25Q64 代码
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/9
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
////////////////////////////////////////////////////////////////

u16_t W25QXX_TYPE = W25Q128; //默认是W25Q128

//4Kbytes为一个Sector
//16个扇区为1个Block
//W25Q128 容量为16M字节,共有128个Block,4096个Sector

//初始化SPI FLASH的IO口
struct w25q_flash_dev W25QXX_Init(void)
{
	struct w25q_flash_dev dev;

	// gpio_f1c100s_set_dir(&GPIO_PE, 5, GPIO_DIRECTION_OUTPUT);
	spi_flash_init(SPI0);
	// spi_set_rate(SPI0, 0x00001002);
	W25QXX_TYPE = W25QXX_ReadID(); //读取FLASH ID
	dev.type = W25QXX_TYPE;
	dev.sector = 4;
	switch (W25QXX_TYPE)
	{
	case W25Q256:
		dev.size = 32*1024;
		W25QXX_Set4ByteAddr();
		break;

	case W25Q128:
		dev.size = 16*1024;
		break;

	case W25Q64:
		dev.size = 8*1024;
		break;

	case W25Q32:
		dev.size = 4*1024;
		break;

	case W25Q16:
		dev.size = 2*1024;

	case W25Q80:
		dev.size = 1*1024;
		break;

	default:
		dev.size = 0;
		break;
	}
	dev.block = dev.size/dev.sector/16;
	return dev;
}

//读取W25QXX的状态寄存器
//BIT7  6   5   4   3   2   1   0
//SPR   RV  TB BP2 BP1 BP0 WEL BUSY
//SPR:默认0,状态寄存器保护位,配合WP使用
//TB,BP2,BP1,BP0:FLASH区域写保护设置
//WEL:写使能锁定
//BUSY:忙标记位(1,忙;0,空闲)
//默认:0x00
u8_t W25QXX_ReadSR(void)
{
	u8_t i = W25X_ReadStatusReg;
	u8_t byte = 0;

	spi_select(SPI0);					   //使能器件
	//, 发送读取状态寄存器命令, 读取一个字节
	spi_write_then_read(SPI0, (void *)&i, 1, (void *)&byte, 1); 
	spi_deselect(SPI0);						//取消片选
	return byte;
}

//写W25QXX状态寄存器
//只有SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)可以写!!!
void W25QXX_Write_SR(u8_t sr)
{
	u8_t i = W25X_WriteStatusReg;

	spi_select(SPI0);						//使能器件
	spi_transfer(SPI0, (void *)&i, NULL, 1);  //发送写取状态寄存器命令
	spi_transfer(SPI0, (void *)&sr, NULL, 1); //写入一个字节
	spi_deselect(SPI0);						//取消片选
}

//W25QXX写使能
//将WEL置位
void W25QXX_Write_Enable(void)
{
	u8_t i = W25X_WriteEnable;

	spi_select(SPI0);					   //使能器件
	spi_transfer(SPI0, (void *)&i, NULL, 1); //发送写指令
	spi_deselect(SPI0);					   //取消片选
}

//W25QXX写禁止
//将WEL清零
void W25QXX_Write_Disable(void)
{
	u8_t i = W25X_WriteDisable;

	spi_select(SPI0);					   //使能器件
	spi_transfer(SPI0, (void *)&i, NULL, 1); //发送写禁止指令
	spi_deselect(SPI0);					   //取消片选
}

//设置4地址模式 W25Q256使用后128内存需要
void W25QXX_Set4ByteAddr(void)
{
	u8_t i = W25X_Enable4ByteAddr;

	spi_select(SPI0);					   //使能器件
	spi_transfer(SPI0, (void *)&i, NULL, 1); //发送设置4字节指令
	spi_deselect(SPI0);					   //取消片选
}

//设置3地址模式, W25Q256默认模式
void W25QXX_Set3ByteAddr(void)
{
	u8_t i = W25X_Disable4ByteAddr;

	spi_select(SPI0);					   //使能器件
	spi_transfer(SPI0, (void *)&i, NULL, 1); //发送设置3字节指令
	spi_deselect(SPI0);					   //取消片选
}

//读取芯片ID
//返回值如下:
//0XEF13,表示芯片型号为W25Q80
//0XEF14,表示芯片型号为W25Q16
//0XEF15,表示芯片型号为W25Q32
//0XEF16,表示芯片型号为W25Q64
//0XEF17,表示芯片型号为W25Q128
//0XEF18,表示芯片型号为W25Q256
u16_t W25QXX_ReadID(void)
{
	u8_t Temp[] = {0, 0};
	u16_t Temp2 = 0;
	u8_t i[] = {W25X_ManufactDeviceID, 0, 0, 0};

	spi_select(SPI0);
	//读取ID NOTE: F1C100S is big endian
	spi_write_then_read(SPI0, (void *)&i[0], 4, (void *)&Temp[0], 2);	
	Temp2 = (u16_t)Temp[0]<<8|(u16_t)Temp[1];
	spi_deselect(SPI0);
	return Temp2;
}

//获取JEDEC ID
u32_t ReadFlashJedecID(void)
{
	u8_t Temp[] = {0, 0, 0, 0};
	u32_t Temp2 = 0;
	u8_t i[] = {W25X_JedecDeviceID, 0, 0, 0};

	spi_select(SPI0);
	//读取ID NOTE: F1C100S is big endian
	spi_write_then_read(SPI0, (void *)&i[0], 1, (void *)&Temp[0], 4);	
	Temp2 = (u32_t)Temp[0]<<24|(u32_t)Temp[1]<<16|(u32_t)Temp[2]<<8|(u32_t)Temp[3];
	spi_deselect(SPI0);
	return Temp2;
}

//获取user id
u64_t W25QXX_UniqueID(void)
{
	u8_t Temp[] = {0, 0, 0, 0, 0, 0, 0, 0};
	u64_t Temp2 = 0;
	u8_t i[] = {W25X_UniqueID, 0, 0, 0, 0, 0};
	int txlen = 5;
	if(W25QXX_TYPE==W25Q256)
		txlen = 6;

	spi_select(SPI0);
	//读取ID NOTE: F1C100S is big endian
	spi_write_then_read(SPI0, (void *)&i[0], txlen, (void *)&Temp[0], 8);	
	Temp2 = (u64_t)Temp[0]<<56|(u64_t)Temp[1]<<48|(u64_t)Temp[2]<<40|(u64_t)Temp[3]<<32| \
			(u64_t)Temp[4]<<24|(u64_t)Temp[5]<<16|(u64_t)Temp[6]<<8|(u64_t)Temp[7];
	spi_deselect(SPI0);
	return Temp2;
}

//读取SPI FLASH
//在指定地址开始读取指定长度的数据
//pBuffer:数据存储区
//ReadAddr:开始读取的地址
//NumByteToRead:要读取的字节数(最大65535)
void W25QXX_Read(u8_t *pBuffer, u32_t ReadAddr, u16_t NumByteToRead)
{
	uint8_t tx[5];
	int txlen = 0;

	tx[0] = W25X_ReadData;
	if(W25QXX_TYPE==W25Q256)
	{
		txlen = 5;
		tx[1] = (uint8_t)(ReadAddr >> 24);
		tx[2] = (uint8_t)(ReadAddr >> 16);
		tx[3] = (uint8_t)(ReadAddr >> 8);
		tx[4] = (uint8_t)(ReadAddr >> 0);
	}
	else
	{
		txlen = 4;
		tx[1] = (uint8_t)(ReadAddr >> 16);
		tx[2] = (uint8_t)(ReadAddr >> 8);
		tx[3] = (uint8_t)(ReadAddr >> 0);
	}
	spi_select(SPI0);
	spi_write_then_read(SPI0, tx, txlen, (void *)pBuffer, (int)NumByteToRead);
	spi_deselect(SPI0);
}

//SPI在一页(0~65535)内写入少于256个字节的数据
//在指定地址开始写入最大256字节的数据
//pBuffer:数据存储区
//WriteAddr:开始写入的地址
//NumByteToWrite:要写入的字节数(最大256),该数不应该超过该页的剩余字节数!!!
void W25QXX_Write_Page(u8_t *pBuffer, u32_t WriteAddr, u16_t NumByteToWrite)
{
	u8_t tx[5];
	int txlen = 0;

	tx[0] = W25X_PageProgram;
	if(W25QXX_TYPE==W25Q256)
	{
		txlen = 5;
		tx[1] = (uint8_t)(WriteAddr >> 24);
		tx[2] = (uint8_t)(WriteAddr >> 16);
		tx[3] = (uint8_t)(WriteAddr >> 8);
		tx[4] = (uint8_t)(WriteAddr >> 0);
	}
	else
	{
		txlen = 4;
		tx[1] = (uint8_t)(WriteAddr >> 16);
		tx[2] = (uint8_t)(WriteAddr >> 8);
		tx[3] = (uint8_t)(WriteAddr >> 0);
	}
	W25QXX_Write_Enable();						 //SET WEL
	spi_select(SPI0);						 //使能器件
	spi_transfer(SPI0, tx, NULL, txlen); 
	spi_transfer(SPI0, pBuffer, NULL, (int)NumByteToWrite);//循环写数  
	spi_deselect(SPI0);				//取消片选
	W25QXX_Wait_Busy();					//等待写入结束
}

//无检验写SPI FLASH
//必须确保所写的地址范围内的数据全部为0XFF,否则在非0XFF处写入的数据将失败!
//具有自动换页功能
//在指定地址开始写入指定长度的数据,但是要确保地址不越界!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址
//NumByteToWrite:要写入的字节数(最大65535)
//CHECK OK
void W25QXX_Write_NoCheck(u8_t *pBuffer, u32_t WriteAddr, u16_t NumByteToWrite)
{
	u16_t pageremain;
	pageremain = 256 - WriteAddr % 256; //单页剩余的字节数
	if (NumByteToWrite <= pageremain)
		pageremain = NumByteToWrite; //不大于256个字节
	while (1)
	{
		W25QXX_Write_Page(pBuffer, WriteAddr, pageremain);
		if (NumByteToWrite == pageremain)
		{
			break; //写入结束了
		}
		else	   //NumByteToWrite>pageremain
		{
			pBuffer += pageremain;
			WriteAddr += pageremain;

			NumByteToWrite -= pageremain; //减去已经写入了的字节数
			if (NumByteToWrite > 256)
				pageremain = 256; //一次可以写入256个字节
			else
				pageremain = NumByteToWrite; //不够256个字节了
		}
	};
}

//写SPI FLASH
//在指定地址开始写入指定长度的数据
//该函数带擦除操作!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址
//NumByteToWrite:要写入的字节数(最大65535)
u8_t W25QXX_BUFFER[4096];
void W25QXX_Write(u8_t *pBuffer, u32_t WriteAddr, u16_t NumByteToWrite)
{
	u32_t secpos;
	u16_t secoff;
	u16_t secremain;
	u16_t i;
	u8_t *W25QXX_BUF;

	W25QXX_BUF = W25QXX_BUFFER;
	secpos = WriteAddr / 4096; //扇区地址
	secoff = WriteAddr % 4096; //在扇区内的偏移
	secremain = 4096 - secoff; //扇区剩余空间大小
	//w25qxx_debug("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);//测试用
	if (NumByteToWrite <= secremain)
		secremain = NumByteToWrite; //不大于4096个字节
	while (1)
	{
		W25QXX_Read(W25QXX_BUF, secpos * 4096, 4096); //读出整个扇区的内容
		for (i = 0; i < secremain; i++)				  //校验数据
		{
			if (W25QXX_BUF[secoff + i] != 0XFF)
				break; //需要擦除
		}
		if (i < secremain) //需要擦除
		{
			W25QXX_Erase_Sector(secpos);	//擦除这个扇区
			for (i = 0; i < secremain; i++) //复制
			{
				W25QXX_BUF[i + secoff] = pBuffer[i];
			}
			W25QXX_Write_NoCheck(W25QXX_BUF, secpos * 4096, 4096); //写入整个扇区
		}
		else
		{	//写已经擦除了的,直接写入扇区剩余区间.
			W25QXX_Write_NoCheck(pBuffer, WriteAddr, secremain); 
		}
		if (NumByteToWrite == secremain)
		{
			break; //写入结束了
		}
		else	   //写入未结束
		{
			secpos++;   //扇区地址增1
			secoff = 0; //偏移位置为0

			pBuffer += secremain;		 //指针偏移
			WriteAddr += secremain;		 //写地址偏移
			NumByteToWrite -= secremain; //字节数递减
			if (NumByteToWrite > 4096)
				secremain = 4096; //下一个扇区还是写不完
			else
				secremain = NumByteToWrite; //下一个扇区可以写完了
		}
	}
}

//擦除整个芯片
//等待时间超长...
void W25QXX_Erase_Chip(void)
{
	u8_t tx = W25X_ChipErase;

	W25QXX_Write_Enable(); //SET WEL
	W25QXX_Wait_Busy();
	spi_select(SPI0);			//使能器件
	spi_transfer(SPI0, (void *)&tx, NULL, 1); //发送片擦除命令
	spi_deselect(SPI0);						//取消片选
	W25QXX_Wait_Busy();					//等待芯片擦除结束
}

//擦除一个扇区
//Dst_Addr:扇区地址 根据实际容量设置
//擦除一个山区的最少时间:150ms
void W25QXX_Erase_Sector(u32_t Dst_Addr)
{
	u8_t tx[5];
	int txlen = 0;

	// w25qxx_debug("fe:%x\r\n",Dst_Addr); //监视falsh擦除情况,测试用
	Dst_Addr *= 4096;
	tx[0] = W25X_SectorErase;
	if(W25QXX_TYPE==W25Q256)
	{
		txlen = 5;
		tx[1] = (uint8_t)(Dst_Addr >> 24);
		tx[2] = (uint8_t)(Dst_Addr >> 16);
		tx[3] = (uint8_t)(Dst_Addr >> 8);
		tx[4] = (uint8_t)(Dst_Addr >> 0);
	}
	else
	{
		txlen = 4;
		tx[1] = (uint8_t)(Dst_Addr >> 16);
		tx[2] = (uint8_t)(Dst_Addr >> 8);
		tx[3] = (uint8_t)(Dst_Addr >> 0);
	}
	
	W25QXX_Write_Enable(); //SET WEL
	W25QXX_Wait_Busy();
	spi_select(SPI0);				//使能器件
	spi_transfer(SPI0, (void *)&tx[0], NULL, txlen); 
	spi_deselect(SPI0);		//取消片选
	W25QXX_Wait_Busy(); //等待擦除完成
}

//擦除一个区域扇区
//addr:扇区号, num: 擦拭数量
void W25QXX_Erase_Sector_Range(u32_t Dst_Addr, u16_t num)
{
	if(num==0)
		return;
	for (u16 i = 0; i < num; i++)
	{
		W25QXX_Erase_Sector(Dst_Addr+i);
		// w25qxx_debug("%d\r\n", Dst_Addr+i);
	}
}

//等待空闲
void W25QXX_Wait_Busy(void)
{
	while ((W25QXX_ReadSR() & 0x01) == 0x01); // 等待BUSY位清空
}

//进入掉电模式
void W25QXX_PowerDown(void)
{
	u8_t i = W25X_PowerDown;
	volatile u8_t us = 6;

	spi_select(SPI0);					   //使能器件
	spi_transfer(SPI0, (void *)&i, NULL, 1); //发送掉电命令
	spi_deselect(SPI0);					   //取消片选                              
	while(us--)	//等待TPD  about:6us
	{
		volatile u32_t n = 36;
		while(n--);
	}
}

//唤醒
void W25QXX_WAKEUP(void)
{
	volatile u8_t us = 6;
	u8_t i = W25X_ReleasePowerDown;

	spi_select(SPI0);					   //使能器件
	spi_transfer(SPI0, (void *)&i, NULL, 1); //send W25X_PowerDown command 0xAB
	spi_deselect(SPI0);					   //取消片选
	while(us--)	//等待TRES1 about:6us
	{
		volatile u32_t n = 36;
		while(n--);
	}
}

/******************************************************
 * test w25q
 * **************************************************/
void test_w25q(void)
{
	struct w25q_flash_dev flash_dev;
    u64_t UID = 0;
    u32_t iooi[] = {0, 0};
	u8_t str1[] = "TEST w25q";
	u8_t str2[] = "TEST w25q";
	memset(str2, 0, sizeof(str2));

    flash_dev = W25QXX_Init(); //初始化
	//IC信息
    w25qxx_debug("type: %#X, size %d, sector: %d\r\n", flash_dev.type, flash_dev.size, flash_dev.sector);
    UID = W25QXX_UniqueID();
    iooi[0] = UID>>32;
    iooi[1] = UID;
    w25qxx_debug("w25q uid: %#X %#X\r\n", iooi[0], iooi[1]);
	w25qxx_debug("w25q jedc id: %#X\r\n", ReadFlashJedecID());
	//读写
	w25qxx_debug("write: %s\r\n", str1);
    W25QXX_Write(&str1[0], 1024 * 1024 * 4, sizeof(str1)); //4M地址
    W25QXX_Read(&str2[0], 1024 * 1024 * 4, sizeof(str2));
    w25qxx_debug("Read result: %s\r\n", str2);
}

