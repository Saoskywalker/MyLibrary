#include "gt9147.h"
#include "touch.h"
#include "ctiic.h"
#include "stdio.h"
#include "delay.h" 
#include "string.h" 
#include "GPIO_port.h"
#include "lcd.h"

//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//4.3寸电容触摸屏-GT9147 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/1/15
//版本：V1.1
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved		
//*******************************************************************************
//修改信息
//V1.1 20150320
//修改GT9147_Scan函数,添加对非法数据的处理,防止非法数据干扰
////////////////////////////////////////////////////////////////////////////////// 

//IO操作函数	   
//IO定义为移植方便, 放于GPIO_port.h

//I2C读写命令
//RST低电平复位
#define _ADDR_28 1
#if _ADDR_28 == 1
//INT复位前高电平
#define GT_CMD_WR 		0X28     	//写命令
#define GT_CMD_RD 		0X29		//读命令
#else
//INT复位前低电平
#define GT_CMD_WR 		0XBA     	//写命令
#define GT_CMD_RD 		0XBB		//读命令
#endif
  
//GT9147 部分寄存器定义 
#define GT_CTRL_REG 	0X8040   	//GT9147控制寄存器
#define GT_CFGS_REG 	0X8047   	//GT9147配置起始地址寄存器
#define GT_CHECK_REG 	0X80FF   	//GT9147校验和寄存器
#define GT_PID_REG 		0X8140   	//GT9147产品ID寄存器

#define GT_GSTID_REG 	0X814E   	//GT9147当前检测到的触摸情况
#define GT_TP1_REG 		0X8150  	//第一个触摸点数据地址
#define GT_TP2_REG 		0X8158		//第二个触摸点数据地址
#define GT_TP3_REG 		0X8160		//第三个触摸点数据地址
#define GT_TP4_REG 		0X8168		//第四个触摸点数据地址
#define GT_TP5_REG 		0X8170		//第五个触摸点数据地址  

//GT9147配置参数表
//第一个字节为版本号(0X60),必须保证新的版本号大于等于GT9147内部
//flash原有版本号,才会更新配置,写0x00初始化版本号
//0X8048 L,0X8049 H(X轴分辨率),,,,0X804A L,0X804B H(Y轴分辨率)
//地址从0X8047~0X8100, 共186字节, 0X80FF为0X8047~0X80FE的校验码
//0X8100为是否保存配置标记, 0不保存, 1保存
const u8 GT9147_CFG_TBL[]=
{ 
	0x60,0x00,0x04,0x00,0x03,0x02,0x3D,0x00,0x22,0x08,
	0x28,0x08,0x5A,0x3C,0x03,0x05,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x18,0x1A,0x1E,0x14,0x90,0x30,0xAA,
	0x74,0x76,0x82,0x0A,0x00,0x00,0x00,0xA0,0x02,0x11,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x5A,0x96,0x94,0xD5,0x02,0x07,0x00,0x00,0x04,
	0x8B,0x5E,0x00,0x84,0x69,0x00,0x80,0x74,0x00,0x7C,
	0x80,0x00,0x7A,0x8E,0x00,0x7A,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x01,0x04,0x05,0x06,0x07,0x08,0x09,
	0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x14,0x15,0x16,0x17,
	0x18,0x19,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x02,0x04,0x06,0x07,0x08,0x0A,0x0C,
	0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x19,0x1B,
	0x1C,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,
	0x27,0x28,0x29,0x2A,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00//,0xFB,0x01
};  

//发送GT9147配置参数
//mode:0,参数不保存到flash
//     1,参数保存到flash
u8 GT9147_Send_Cfg(u8 mode)
{
	u8 buf[2];
	u8 i = 0;
	buf[0] = 0;
	buf[1] = mode; //是否写入到GT9147 FLASH?  即是否掉电保存
	for (i = 0; i < sizeof(GT9147_CFG_TBL); i++)
		buf[0] += GT9147_CFG_TBL[i]; //计算校验和
	buf[0] = (~buf[0]) + 1;
	GT9147_WR_Reg(GT_CFGS_REG, (u8 *)GT9147_CFG_TBL, sizeof(GT9147_CFG_TBL)); //发送寄存器配置
	GT9147_WR_Reg(GT_CHECK_REG, buf, 2);									  //写入校验和,和配置更新标记
	return 0;
}
//向GT9147写入一次数据
//reg:起始寄存器地址
//buf:数据缓缓存区
//len:写数据长度
//返回值:0,成功;1,失败.
u8 GT9147_WR_Reg(u16 reg,u8 *buf,u8 len)
{
	u8 i;
	u8 ret=0;
	CT_IIC_Start();	
 	CT_IIC_Send_Byte(GT_CMD_WR);   	//发送写命令 	 
	CT_IIC_Wait_Ack();
	CT_IIC_Send_Byte(reg>>8);   	//发送高8位地址
	CT_IIC_Wait_Ack(); 	 										  		   
	CT_IIC_Send_Byte(reg&0XFF);   	//发送低8位地址
	CT_IIC_Wait_Ack();  
	for(i=0;i<len;i++)
	{	   
    	CT_IIC_Send_Byte(buf[i]);  	//发数据
		ret=CT_IIC_Wait_Ack();
		if(ret)break;  
	}
    CT_IIC_Stop();					//产生一个停止条件	    
	return ret; 
}
//从GT9147读出一次数据
//reg:起始寄存器地址
//buf:数据缓缓存区
//len:读数据长度			  
void GT9147_RD_Reg(u16 reg,u8 *buf,u8 len)
{
	u8 i; 
 	CT_IIC_Start();	
 	CT_IIC_Send_Byte(GT_CMD_WR);   //发送写命令 	 
	CT_IIC_Wait_Ack();
 	CT_IIC_Send_Byte(reg>>8);   	//发送高8位地址
	CT_IIC_Wait_Ack(); 	 										  		   
 	CT_IIC_Send_Byte(reg&0XFF);   	//发送低8位地址
	CT_IIC_Wait_Ack();  
 	CT_IIC_Start();  	 	   
	CT_IIC_Send_Byte(GT_CMD_RD);   //发送读命令		   
	CT_IIC_Wait_Ack();	   
	for(i=0;i<len;i++)
	{	   
    	buf[i]=CT_IIC_Read_Byte(i==(len-1)?0:1); //发数据	  
	} 
    CT_IIC_Stop();//产生一个停止条件    
} 
//初始化GT9147触摸屏
//返回值:0,初始化成功;1,初始化失败 
u8 GT9147_Init(void)
{
	u8 temp[] = {0, 0, 0, 0, 0};
	u8 cnt = 0;

	//INT RST IO INIT
	GT9147_IO_init();	
	GT_RST(1);
	GT_INT_SET(1);
	CT_IIC_Init();      	//初始化电容屏的I2C总线
	
	for (cnt = 0; cnt < 4; cnt++)
	{
		GT_RST(0);
		GT_INT_SET(0);
		delay_ms(1);
		GT_RST(0); //复位 ,,,地址选择
#if _ADDR_28 == 1
		GT_INT_SET(1);
#else
		GT_INT_SET(0);
#endif
		delay_ms(8);
		GT_RST(1); //释放复位
		delay_ms(8);

		//change int io mode
		GT9147_int_pin_set();
		delay_ms(90);

		GT9147_RD_Reg(GT_PID_REG, temp, 4); //读取产品ID
		temp[4] = 0;
		printf("CTP ID:%s\r\n", temp);		  //打印ID
		// if (strcmp((char *)temp, "928") == 0) //ID==9147
		if (temp[0] != 0 && temp[0] != 0XFF)
		{
			temp[0] = 0X02;
			GT9147_WR_Reg(GT_CTRL_REG, temp, 1); //软复位GT9147
			GT9147_RD_Reg(GT_CFGS_REG, temp, 1); //读取GT_CFGS_REG寄存器
/* 			if (temp[0] < 0X60)					 //默认版本比较低,需要更新flash配置
			{
				u8 i = 0;
				for (i = 0; i < sizeof(GT9147_CFG_TBL); i++) //读出配置表, 以防丢失当前配置
				{
					GT9147_RD_Reg(0X8047 + i, temp, 1);
					printf("%#x, ", temp[0]);
					// printf("ADDR[%x]\r\n", (0X8047 + i));
				}
				printf("Default Ver:%d\r\n", temp[0]);
				GT9147_Send_Cfg(1); //更新并保存配置
			} */
			delay_ms(10);
			temp[0] = 0X00;
			GT9147_WR_Reg(GT_CTRL_REG, temp, 1); //结束复位
			return 0; //成功
		}
	}
	return 1; //失败
}
const u16 GT9147_TPX_TBL[]={GT_TP1_REG,GT_TP2_REG,GT_TP3_REG,GT_TP4_REG,GT_TP5_REG};
//扫描触摸屏(采用查询方式), GT9xx的触摸检测频率为100Hz
//mode:0,正常扫描.
//返回值:当前触屏状态.
u8 GT9147_Scan(u8 mode)
{
	u8 buf[5];
	u8 i=0;
	u8 temp;
	u8 tempsta = tp_dev.sta; //保存当前的tp_dev.sta值;
	int temp2;
 	static u8 t=0;//控制查询间隔,从而降低CPU占用率
	static u8 Press_Times = 0;   

	t++;
	if((t%10)==0||t<10)//空闲时,每进入10次CTP_Scan函数才检测1次,从而节省CPU使用率
	{
		GT9147_RD_Reg(GT_GSTID_REG,&buf[4],1);	//读取触摸点的状态  
 		if(buf[4]&0X80&&((buf[4]&0XF)<=CT_MAX_TOUCH))
		{
			temp=0;
			GT9147_WR_Reg(GT_GSTID_REG,&temp,1);//清标志 		
		}		
		if((buf[4]&0XF)&&((buf[4]&0XF)<=CT_MAX_TOUCH))
		{
			Press_Times = 0;
			temp=0XFF<<(buf[4]&0XF);		//将点的个数转换为1的位数,匹配tp_dev.sta定义 
			tp_dev.sta=(~temp)|TP_PRES_DOWN|TP_CATH_PRES; 
			tp_dev.x[4]=tp_dev.x[0];	//保存触点0的数据
			tp_dev.y[4]=tp_dev.y[0];
			// for(i=0;i<(buf[4]&0XF);i++) //读多个点
			for(i=0;i<1;i++) //只读第一个点
			{
				GT9147_RD_Reg(GT9147_TPX_TBL[i], buf, 4); //读取XY坐标值
				tp_dev.x[i] = ((u16)buf[1] << 8) + buf[0];
				tp_dev.y[i] = ((u16)buf[3] << 8) + buf[2];
				//方向转换
				if (((tp_dev.touchtype & 0x06) >> 1) == 0) //90度
				{
					temp2 = tp_dev.y[i];
					tp_dev.y[i] = tp_dev.x[i];
					tp_dev.x[i] = tp_dev.width - 1 - temp2;
				}
				else if (((tp_dev.touchtype & 0x06) >> 1) == 2) //270度
				{
					temp2 = tp_dev.x[i];
					tp_dev.x[i] = tp_dev.y[i];
					tp_dev.y[i] = tp_dev.height - 1 - temp2;
				}
				else if (((tp_dev.touchtype & 0x06) >> 1) == 3) //180度
				{
					tp_dev.x[i] = tp_dev.width - 1 - tp_dev.x[i];
					tp_dev.y[i] = tp_dev.height - 1 - tp_dev.y[i];
				}
				else //0度
				{
				}
				// printf("x[%d]:%d,y[%d]:%d\r\n",i,tp_dev.x[i],i,tp_dev.y[i]);		
			} 
			//if(tp_dev.x[0]>lcddev.width||tp_dev.y[0]>lcddev.height)//非法数据(坐标超出了)
			// { 
			// 	if((buf[4]&0XF)>1)		//有其他点有数据,则复第二个触点的数据到第一个触点.
			// 	{
			// 		tp_dev.x[0]=tp_dev.x[1];
			// 		tp_dev.y[0]=tp_dev.y[1];
			// 		t=0;				//触发一次,则会最少连续监测10次,从而提高命中率
			// 	}else					//非法数据,则忽略此次数据(还原原来的)  
			// 	{
			// 		tp_dev.x[0]=tp_dev.x[4];
			// 		tp_dev.y[0]=tp_dev.y[4];
			// 		buf[4]=0X80;		
			// 		tp_dev.sta=tempsta;	//恢复tp_dev.sta
			// 	}
			// }
			// else 
			// {
				t=0;					//触发一次,则会最少连续监测10次,从而提高命中率
			// }
		}
		else if(++Press_Times>=4) 
		{
			//检测触摸是否一直按着没放，需要做一下额外的处理
			//0x814E的最高位被清零后，要等一会儿才变为1, 延时等待0X814E更新
			Press_Times = 0;
			tp_dev.sta&=~(1<<7);	//标记按键松开
		}
	}
	// if((buf[4]&0X8F)==0X80)//无触摸点按下
	// { 
	// 	if(tp_dev.sta&TP_PRES_DOWN)	//之前是被按下的
	// 	{
	// 		tp_dev.sta&=~(1<<7);	//标记按键松开
	// 	}else						//之前就没有被按下
	// 	{ 
	// 		tp_dev.x[0]=0xffff;
	// 		tp_dev.y[0]=0xffff;
	// 		tp_dev.sta&=0XE0;	//清除点有效标记	
	// 	}	 
	// } 	
	if(t>240)
		t=10;//重新从10开始计数
	return tp_dev.sta & TP_PRES_DOWN; //返回当前的触屏状态
}


 



























