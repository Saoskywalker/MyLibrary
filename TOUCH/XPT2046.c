#include "XPT2046.h"
#include "types_plus.h"
#include "delay.h"
#include "touch.h"
#include "stdio.h"
#include "gpio_board.h"

//命令
#define CMD_RDX 0XD0
#define CMD_RDY 0X90

#define XPT_SPI 1 //0用硬件SPI,1用模拟SPI , F1C100S SPI硬件速度过大不能调

#if XPT_SPI

//电阻屏芯片连接引脚, 用于模拟spi
//IO定义为移植方便, 放于GPIO_port.h

//SPI写数据
//向触摸屏IC写入1byte数据
//num:要写入的数据
static void TP_Write_Byte(u8 num)
{
    u8 count = 0;
    for (count = 0; count < 8; count++)
    {
        if (num & 0x80)
            TDIN(1);
        else
            TDIN(0);
        num <<= 1;
        TCLK(0);
        delay_us(1);
        TCLK(1); //上升沿有效
    }
}

//SPI读数据
//从触摸屏IC读取adc值
//CMD:指令
//返回值:读到的数据
static u16 TP_Read_AD(u8 CMD)
{
    u8 count = 0;
    u16 Num = 0;
    TCLK(0);            //先拉低时钟
    TDIN(0);            //拉低数据线
    TCS(0);             //选中触摸屏IC
    TP_Write_Byte(CMD); //发送命令字
    delay_us(6);        //ADS7846的转换时间最长为6us
    TCLK(0);
    delay_us(1);
    TCLK(1); //给1个时钟，清除BUSY
    delay_us(1);
    TCLK(0);
    for (count = 0; count < 16; count++) //读出16位数据,只有高12位有效
    {
        Num <<= 1;
        TCLK(0); //下降沿有效
        delay_us(1);
        TCLK(1);
        if (TDOUT)
            Num++;
    }
    Num >>= 4; //只有高12位有效.
    TCS(1);    //释放片选
    return (Num);
}

#else

#define PEN gpio_f1c100s_get_value(&GPIO_PF, 1)			//INT
#define TCS(x) gpio_f1c100s_set_value(&GPIO_PF, 0, x)  // CS

//SPI写数据
//向触摸屏IC写入1byte数据
//num:要写入的数据
static void TP_Write_Byte(u8 num)
{
    spi_transfer(SPI1, &num, 0, 1); 
}

//SPI读数据
//从触摸屏IC读取adc值
//CMD:指令
//返回值:读到的数据
static u16 TP_Read_AD(u8 CMD)
{
    u8 i[2];
    u16 Num = 0;
    TCS(0);             //选中触摸屏IC
    TP_Write_Byte(CMD); //发送命令字
    delay_us(6);     //ADS7846的转换时间最长为6us
    spi_transfer(SPI1, 0, &i[0], 2); 
    Num = (u16)i[0]<<8|(u16)i[1];
    Num >>= 4; //只有高12位有效.
    TCS(1);    //释放片选
    return (Num);
}

#endif

//读取一个坐标值(x或者y)
//连续读取READ_TIMES次数据,对这些数据升序排列,
//然后去掉最低和最高LOST_VAL个数,取平均值
//xy:指令（CMD_RDX/CMD_RDY）
//返回值:读到的数据
#define READ_TIMES 5 //读取次数
#define LOST_VAL 1   //丢弃值
static u16 TP_Read_XOY(u8 xy)
{
    u16 i, j;
    u16 buf[READ_TIMES];
    u16 sum = 0;
    u16 temp;
    for (i = 0; i < READ_TIMES; i++)
        buf[i] = TP_Read_AD(xy);
    for (i = 0; i < READ_TIMES - 1; i++) //排序
    {
        for (j = i + 1; j < READ_TIMES; j++)
        {
            if (buf[i] > buf[j]) //升序排列
            {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }
    sum = 0;
    for (i = LOST_VAL; i < READ_TIMES - LOST_VAL; i++)
        sum += buf[i];
    temp = sum / (READ_TIMES - 2 * LOST_VAL);
    return temp;
}

//读取x,y坐标
//最小值不能少于100.
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
static u8 TP_Read_XY(u16 *x, u16 *y)
{
    u16 xtemp, ytemp;

    if (tp_dev.touchtype&0x01)
    {
        xtemp = TP_Read_XOY(CMD_RDX);
        ytemp = TP_Read_XOY(CMD_RDY);
    }
    else
    {
        ytemp = TP_Read_XOY(CMD_RDX);
        xtemp = TP_Read_XOY(CMD_RDY);
    }
    //if(xtemp<100||ytemp<100)return 0;//读数失败
    *x = xtemp;
    *y = ytemp;
    return 1; //读数成功
}

//连续2次读取触摸屏IC,且这两次的偏差不能超过
//ERR_RANGE,满足条件,则认为读数正确,否则读数错误.
//该函数能大大提高准确度
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
#define ERR_RANGE 50 //误差范围
static u8 TP_Read_XY2(u16 *x, u16 *y)
{
    u16 x1, y1;
    u16 x2, y2;
    u8 flag;
    flag = TP_Read_XY(&x1, &y1);
    if (flag == 0)
        return (0);
    flag = TP_Read_XY(&x2, &y2);
    if (flag == 0)
        return (0);
    // printf("point: %d, %d, %d, %d\r\n", x1, y1, x2, y2);
    if (((x2 <= x1 && x1 < x2 + ERR_RANGE) || (x1 <= x2 && x2 < x1 + ERR_RANGE)) //前后两次采样在+-50内
        && ((y2 <= y1 && y1 < y2 + ERR_RANGE) || (y1 <= y2 && y2 < y1 + ERR_RANGE)))
    {
        *x = (x1 + x2) / 2;
        *y = (y1 + y2) / 2;
        return 1;
    }
    else
        return 0;
}

//触摸按键扫描
//tp:0,屏幕坐标;1,物理坐标(校准等特殊场合用)
//返回值:当前触屏状态.
//0,触屏无触摸;1,触屏有触摸
unsigned char TP_Scan(unsigned char tp)
{
    if (PEN == 0) //有按键按下
    {
        if (tp)
            TP_Read_XY2(&tp_dev.x[0], &tp_dev.y[0]);      //读取物理坐标
        else if (TP_Read_XY2(&tp_dev.x[0], &tp_dev.y[0])) //读取屏幕坐标
        {
            tp_dev.x[0] = tp_dev.xfac * tp_dev.x[0] + tp_dev.xoff; //将结果转换为屏幕坐标
            tp_dev.y[0] = tp_dev.yfac * tp_dev.y[0] + tp_dev.yoff;
            // printf("dis: %d, %d\r\n", tp_dev.x[0],tp_dev.y[0]);
        }
        if ((tp_dev.sta & TP_PRES_DOWN) == 0) //之前没有被按下
        {
            tp_dev.sta = TP_PRES_DOWN | TP_CATH_PRES; //按键按下
            tp_dev.x[4] = tp_dev.x[0];                //记录第一次按下时的坐标
            tp_dev.y[4] = tp_dev.y[0];
        }
    }
    else
    {
        if (tp_dev.sta & TP_PRES_DOWN) //之前是被按下的
        {
            tp_dev.sta &= ~(1 << 7); //标记按键松开
        }
        else //之前就没有被按下
        {
            tp_dev.x[4] = 0;
            tp_dev.y[4] = 0;
            tp_dev.x[0] = 0xffff;
            tp_dev.y[0] = 0xffff;
        }
    }
    return tp_dev.sta & TP_PRES_DOWN; //返回当前的触屏状态
}

unsigned char XPT2046_Init(void) //初始化 PS: XPT2046官文最高spi rate:2.5MHz
{
    #if XPT_SPI
        //模拟SPI
        XPT2046_IO_init();
    #else
        //硬件SPI
        gpio_f1c100s_set_dir(&GPIO_PF, 1, GPIO_DIRECTION_INPUT); //PEN输出
        gpio_f1c100s_set_pull(&GPIO_PF, 1, GPIO_PULL_UP); //上拉
        gpio_f1c100s_set_dir(&GPIO_PF, 0, GPIO_DIRECTION_OUTPUT);
        gpio_f1c100s_set_pull(&GPIO_PF, 0, GPIO_PULL_UP);
        TCS(1); //软件CS
        spi_flash_init(SPI1);		 
	    spi_deselect(SPI1);
        spi_set_rate(SPI1, 0x0000101c); //设置到低速模式 F1C100S最大分频, 过大或其他设置IC会出错
    #endif

    TP_Read_XY(&tp_dev.x[0],&tp_dev.y[0]); //第一次读取初始化
    
    return 0;
}
