/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "types_plus.h"
#include "diskio.h"			/* FatFs lower layer API */
// #include "mmc_sd.h"
#include "w25qxx.h"
#include <malloc.h>
#include "stdio.h"
#include "sys_sd.h"
#include "string.h"

//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板V3
//FATFS底层(diskio) 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/1/20
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 

/***************************
 * FATFS移植处
 **************************/

#define SD_CARD	 0  //SD卡,卷标为0
#define EX_FLASH 1	//外部flash,卷标为1

//对于W25Q128
//14M字节给fatfs用,前2M给客户自己用	 
/*FLASH ADDR*/
#define FLASH_SIZE 14	//FLASH大小MB	
u16	    FLASH_SECTOR_COUNT=2048*FLASH_SIZE; //前2M字节给系统使用
#define FLASH_BLOCK_SIZE   	8     	//每个BLOCK有8个扇区
#define FLASH_ADD_OFFSET 2097152 //分区偏移2M
#define FLASH_SECTOR_SIZE 	512	

static u8 diskBusy = 0;
u8 diskCheckBusy(void)
{
	return diskBusy;
}

//获得磁盘状态
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{ 
	return RES_OK;
}  

//初始化磁盘
DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	u8 res=0;	    
	switch(pdrv)
	{
		case SD_CARD://SD卡
			if (SD_Init(SD0) == 0)
				res = 0;
			else
				res = 1;
			// res = SD_Initialize();//SD_Initialize() 
		 	// if(res)//STM32 SPI的bug,在sd卡操作失败的时候如果不执行下面的语句,可能导致SPI读写异常
			// {
			// 	SD_SPI_SpeedLow();
			// 	SD_SPI_ReadWriteByte(0xff);//提供额外的8个时钟
			// 	SD_SPI_SpeedHigh();
			// }
  			break;

		case EX_FLASH://外部flash
			W25QXX_Init();
			FLASH_SECTOR_COUNT=2048*FLASH_SIZE; //前4M字节给系统使用
			// printf("eee:%d\r\n", FLASH_SECTOR_COUNT);
 			break;

		default:
			res=1; 
			break;
	}		 
	if(res)return  STA_NOINIT;
	else return 0; //初始化成功 
} 

//读扇区
//pdrv:磁盘编号0~9
//*buff:数据接收缓冲首地址
//sector:扇区地址
//count:需要读取的扇区数
DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
	u8 res=0; 
	unsigned char *b;
    if (!count)return RES_PARERR;//count不能等于0，否则返回参数错误		 	 
	switch(pdrv)
	{
		case SD_CARD://SD卡
			diskBusy |= 1<<SD_CARD;
			if ((unsigned int)(long)buff & 0x3)//没有对齐
			{
				/*分配内存*/
				b = (unsigned char *)malloc(count*FLASH_SECTOR_SIZE);
				if(b!=0)
				{
					/*读入内存*/
					if(SD_Read_in(SD0,sector,count,(unsigned char *)b)==0)res =  RES_OK;
					else res =  RES_ERROR;
					/*复制内存到buff*/
					memcpy(buff,b,count*FLASH_SECTOR_SIZE); 
					/*是放内存*/
					free(b);
				}else
				{
					printf("SD malloc ERR...\r\n");
				}
			}else
			{
				if(SD_Read_in(SD0,sector,count,(unsigned char *)buff)==0)res =  RES_OK;
				else res =  RES_ERROR;				
			}
			// res=SD_ReadDisk(buff,sector,count);	 
		 	// if(res)//STM32 SPI的bug,在sd卡操作失败的时候如果不执行下面的语句,可能导致SPI读写异常
			// {
			// 	SD_SPI_SpeedLow();
			// 	SD_SPI_ReadWriteByte(0xff);//提供额外的8个时钟
			// 	SD_SPI_SpeedHigh();
			// }
			diskBusy &= ~(1<<SD_CARD);
			break;

		case EX_FLASH://外部flash  
			diskBusy |= 1<<EX_FLASH;
			// printf("read count: %d\r\n", count);
			if(count>0) //个人版本, 加速
				W25QXX_Read((u8*)buff,FLASH_ADD_OFFSET+sector*FLASH_SECTOR_SIZE,FLASH_SECTOR_SIZE*count);		
			// for(;count>0;count--) //原子版本
			// {
			// 	W25QXX_Read(buff,FLASH_ADD_OFFSET+sector*FLASH_SECTOR_SIZE,FLASH_SECTOR_SIZE);
			// 	sector++;
			// 	buff+=FLASH_SECTOR_SIZE;
			// }
			res=0;
			diskBusy &= ~(1<<EX_FLASH);
			break;

		default:
			res=1; 
			break;
	}
   //处理返回值，将SPI_SD_driver.c的返回值转成ff.c的返回值
    if(res==0x00)return RES_OK;	 
    else return RES_ERROR;	   
}

//写扇区
//pdrv:磁盘编号0~9
//*buff:发送数据首地址
//sector:扇区地址
//count:需要写入的扇区数
#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write */
)
{
	u8 res=0;  
	unsigned char *b;
    if (!count)return RES_PARERR;//count不能等于0，否则返回参数错误		 	 
	switch(pdrv)
	{
		case SD_CARD://SD卡
			diskBusy |= 1<<SD_CARD;
			if ((unsigned int)(long)buff & 0x3)//没有对齐
			{
				/*分配内存*/
				b = (unsigned char *)malloc(count*FLASH_SECTOR_SIZE);
				if(b!=0)
				{
					/*复制内存到buff*/
					memcpy(b,buff,count*FLASH_SECTOR_SIZE); 
					/*读入内存*/
					if(SD_Write_out(SD0,sector,count,(unsigned char *)b)==0)res =  RES_OK;
					else res =  RES_ERROR;
					/*是放内存*/
					free(b);
				}else
				{
					printf("SD malloc ERR...\r\n");
				}
			}else
			{
				if(SD_Write_out(SD0,sector,count,(unsigned char *)buff)==0)res =  RES_OK;
				else res =  RES_ERROR;				
			}
			//res=SD_WriteDisk((u8*)buff,sector,count);
			diskBusy &= ~(1<<SD_CARD);
			break;

		case EX_FLASH://外部flash
			diskBusy |= 1<<EX_FLASH;
			// printf("write count: %d\r\n", count);
			if(count>0) //个人版本, 加速
				W25QXX_Write((u8*)buff,FLASH_ADD_OFFSET+sector*FLASH_SECTOR_SIZE,FLASH_SECTOR_SIZE*count);		
			// for(;count>0;count--) //原子版本
			// {										    
			// 	W25QXX_Write((u8*)buff,FLASH_ADD_OFFSET+sector*FLASH_SECTOR_SIZE,FLASH_SECTOR_SIZE);	
			// 	sector++;
			// 	buff+=FLASH_SECTOR_SIZE;
			// }
			res=0;
			diskBusy &= ~(1<<EX_FLASH);
			break;

		default:
			res=1; 
			break;
	}
    //处理返回值，将SPI_SD_driver.c的返回值转成ff.c的返回值
    if(res == 0x00)return RES_OK;	 
    else return RES_ERROR;	
}
#endif

//其他表参数的获得
//pdrv:磁盘编号0~9
//ctrl:控制代码
//*buff:发送/接收缓冲区指针
#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
DRESULT res;						  			     
	if(pdrv==SD_CARD)//SD卡
	{
		res = RES_ERROR;
			switch (cmd)
			{
				case CTRL_SYNC :		/* Make sure that no pending write process */
					res = RES_OK;
					break;

				case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
					*(DWORD*)buff = SD_INFO.totalSectorN;
					res = RES_OK;
					break;

				case GET_SECTOR_SIZE :	/* Get R/W sector size (WORD) */
					*(WORD*)buff = 512;
					res = RES_OK;
					break;

				case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
					*(DWORD*)buff = 512;
					res = RES_OK;
					break;

				default:
					res = RES_PARERR;
					break;
			}
	    // switch(cmd)
	    // {
		//     case CTRL_SYNC:
		// 		spi_select(SPI1);
		//         if(SD_WaitReady()==0)res = RES_OK; 
		//         else res = RES_ERROR;	  
		// 		spi_deselect(SPI1);
		//         break;	 

		//     case GET_SECTOR_SIZE:
		//         *(WORD*)buff = 512;
		//         res = RES_OK;
		//         break;	 

		//     case GET_BLOCK_SIZE:
		//         *(WORD*)buff = 8;
		//         res = RES_OK;
		//         break;	 

		//     case GET_SECTOR_COUNT:
		//         *(DWORD*)buff = SD_GetSectorCount();
		//         res = RES_OK;
		//         break;

		//     default:
		//         res = RES_PARERR;
		//         break;
	    // }
	}else if(pdrv==EX_FLASH)	//外部FLASH  
	{
	    switch(cmd)
	    {
		    case CTRL_SYNC:
				res = RES_OK; 
		        break;	 
		    case GET_SECTOR_SIZE:
		        *(WORD*)buff = FLASH_SECTOR_SIZE;
		        res = RES_OK;
		        break;	 
		    case GET_BLOCK_SIZE:
		        *(WORD*)buff = FLASH_BLOCK_SIZE;
		        res = RES_OK;
		        break;	 
		    case GET_SECTOR_COUNT:
		        *(DWORD*)buff = FLASH_SECTOR_COUNT;
		        res = RES_OK;
		        break;
		    default:
		        res = RES_PARERR;
		        break;
	    }
	}else res=RES_ERROR;//其他的不支持
    return res;
}
#endif
//获得时间
//User defined function to give a current time to fatfs module      */
//31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */                                                                                                                                                                                                                                          
//15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */                                                                                                                                                                                                                                                
DWORD get_fattime (void)
{				 
	return 0;
}			 
//动态分配内存
void *ff_memalloc (UINT size)			
{
	return (void*)malloc(size);
}
//释放内存
void ff_memfree (void* mf)		 
{
	free(mf);
}

















