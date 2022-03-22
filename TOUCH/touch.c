//触摸屏驱动(支持ADS7843/7846/UH7843/7846/XPT2046/TSC2046/OTT2001A/GT9147/FT5206等)代码	  
//改自原子,,,主要将电阻驱动和触屏控制器文件分离

#include "touch.h"
#include "lcd.h"
#include "ROM_port.h"
#include "delay.h"
#include "stdio.h"
#include "Sagittarius_global.h"

#include "ts_calibrate.h"
#include "touch_port.h"

// #define touch_debug(...) printf(__VA_ARGS__)
#define touch_debug(...)

static u8 TP_Init(void);
static uint8_t None_Scan(int *x, int *y, uint8_t target_num, uint8_t *result_num)
{
    return 0;
}

_m_tp_dev tp_dev =
	{
		TP_Init,
		None_Scan,
		TP_Adjust,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
        0,
        0,
	};

//触摸屏初始化
//电阻/电容屏使用函数: XX_Scan(); XX_Init();
static u8 TP_Init(void)
{
    tp_dev.touchtype |= (lcddev.dir & 0X03)<<1; //屏幕方向
    tp_dev.width = lcddev.width;
    tp_dev.height = lcddev.height;

#if TOUCH_TYPE == 0
    //////电阻屏////////
    // XPT2046_Init(); //XPT2046触屏IC
    // tp_dev.scan = TP_Scan;
    touch_init(); //F1C100S自带电阻屏外设
    tp_dev.scan = touch_scan;

    if (TP_Get_Adjdata())
    {
        return 0; //已经校准
    }
    else //未校准
    {
        TP_Adjust(); //屏幕校准
    }
    return 1;
#else
    //////电容屏///////
    tp_dev.touchtype |= 0X80; //电容屏
    if (touch_init() == 0) // if (GT9147_Init() == 0)   //是GT9147
    {
        tp_dev.scan = touch_scan; // tp_dev.scan = GT9147_Scan; //扫描函数指向GT9147触摸屏扫描
        return 0;
    }
    else
    {
        tp_dev.scan = None_Scan; //初始化失败
        return 2;
    }
#endif
}

//////////////////////////////////////////////////////////////////////////////////
//与LCD部分有关的函数
//画一个触摸点
//用来校准用的
//x,y:坐标
//color:颜色
static void TP_Drow_Touch_Point(u16 x, u16 y, ColorClass color)
{
    POINT_COLOR = color;
    LCD_DrawLine(x - 12, y, x + 13, y); //横线
    LCD_DrawLine(x, y - 12, x, y + 13); //竖线
    LCD_DrawPoint(x + 1, y + 1);
    LCD_DrawPoint(x - 1, y + 1);
    LCD_DrawPoint(x + 1, y - 1);
    LCD_DrawPoint(x - 1, y - 1);
    LCD_Draw_Circle(x, y, 6); //画中心圈
}

//提示字符串
static char *const TP_REMIND_MSG_TBL = "Please use the stylus click the cross \
								on the screen.The cross will always \
								move until the screen adjustment is completed.";
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
/***************电阻屏校准功能****************/
//保存的地址区间基址
/*FLASH ADDR*/
#define SAVE_ADDR_BASE 916173 //flash 894.7KB
#define ADJUST_COMMAND_ADDR 951296 //929KB
u8 TPAjustFlag = 0;

/*重启后校准*/
static u8 TPAdjustReset(void)
{
    u8 i = 0;
    MTF_ROM_read(&i, ADJUST_COMMAND_ADDR, 1);
    if(i==0XAA)
        return 1;
    else
        return 0;
}
static void TPAdjustReady(void)
{
    u8 i = 0;

    MTF_ROM_read(&i, ADJUST_COMMAND_ADDR, 1);
    if(i==0XAA)
    {
        i = 0XFF;
        MTF_ROM_write(&i, ADJUST_COMMAND_ADDR, 1);
    }
    else
    {
        i = 0XAA;
        MTF_ROM_write(&i, ADJUST_COMMAND_ADDR, 1);
        reset_flag = 1;
        while (1);
    }
}

/*********四点校准*************/
/*********************************
typedef struct __attribute__((packed))
{
    /////////////////////触摸屏校准参数(电容屏不需要校准)//////////////////////
    float xfac;
    float yfac;
    short xoff;
    short yoff;
    //b0(用于电阻屏XY反转):0,竖屏(适合左右为X坐标,上下为Y坐标的TP)
    //                    1,横屏(适合左右为Y坐标,上下为X坐标的TP)
    //b1~6:保留.
    //b7:0,电阻屏
    //   1,电容屏
    u8 touchtype;
    u8 flag; //校正确认标志
} _adjust_data;

//PS:注意储存系统要先初始化
//保存校准参数
void TP_Save_Adjdata(void)
{
    _adjust_data adjust_data_temp = {
        tp_dev.xfac,
        tp_dev.yfac,
        tp_dev.xoff,
        tp_dev.yoff,
        tp_dev.touchtype,
        0X0A //写0X0A标记校准过了
    };
    MTF_ROM_write((u8 *)&adjust_data_temp, SAVE_ADDR_BASE, sizeof(_adjust_data)); 
}

//提示校准结果(各个参数)
static void TP_Adj_Info_Show(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2,
                      u16 y2, u16 x3, u16 y3, u16 fac)
{
    POINT_COLOR = RED;
    LCD_ShowString(40, 160, lcddev.width, lcddev.height, 16, (u8 *)"x1:");
    LCD_ShowString(40 + 80, 160, lcddev.width, lcddev.height, 16, (u8 *)"y1:");
    LCD_ShowString(40, 180, lcddev.width, lcddev.height, 16, (u8 *)"x2:");
    LCD_ShowString(40 + 80, 180, lcddev.width, lcddev.height, 16, (u8 *)"y2:");
    LCD_ShowString(40, 200, lcddev.width, lcddev.height, 16, (u8 *)"x3:");
    LCD_ShowString(40 + 80, 200, lcddev.width, lcddev.height, 16, (u8 *)"y3:");
    LCD_ShowString(40, 220, lcddev.width, lcddev.height, 16, (u8 *)"x4:");
    LCD_ShowString(40 + 80, 220, lcddev.width, lcddev.height, 16, (u8 *)"y4:");
    LCD_ShowString(40, 240, lcddev.width, lcddev.height, 16, (u8 *)"fac is:");
    LCD_ShowNum(40 + 24, 160, x0, 4, 16);      //显示数值
    LCD_ShowNum(40 + 24 + 80, 160, y0, 4, 16); //显示数值
    LCD_ShowNum(40 + 24, 180, x1, 4, 16);      //显示数值
    LCD_ShowNum(40 + 24 + 80, 180, y1, 4, 16); //显示数值
    LCD_ShowNum(40 + 24, 200, x2, 4, 16);      //显示数值
    LCD_ShowNum(40 + 24 + 80, 200, y2, 4, 16); //显示数值
    LCD_ShowNum(40 + 24, 220, x3, 4, 16);      //显示数值
    LCD_ShowNum(40 + 24 + 80, 220, y3, 4, 16); //显示数值
    LCD_ShowNum(40 + 56, 240, fac, 3, 16);     //显示数值,该数值必须在95~105范围之内.
}

//得到保存的校准值
//返回值：1，成功获取数据
//        0，获取失败，要重新校准
u8 TP_Get_Adjdata(void)
{
    _adjust_data adjust_data_temp;
    MTF_ROM_read((u8 *)&adjust_data_temp, SAVE_ADDR_BASE, sizeof(_adjust_data)); //读取
    if (adjust_data_temp.flag == 0X0A)                                //触摸屏已经校准过了
    {
        tp_dev.xfac = adjust_data_temp.xfac;
        tp_dev.yfac = adjust_data_temp.yfac;
        tp_dev.xoff = adjust_data_temp.xoff;
        tp_dev.yoff = adjust_data_temp.yoff;
        tp_dev.touchtype = tp_dev.touchtype|(adjust_data_temp.touchtype&0X01); //获取X,Y轴位置
        return 1;
    }
    return 0;
}

//数学函数,,,可用math.h标准库
static int T_abs(int n)
{
	return ((n < 0) ? -n : n);
}

static float T_sqrt(float a)
{
	double x,y;
	x=0.0;
	y=a/2;
	while(x!=y)
	{
		x=y;
		y=(x+a/x)/2;
	}
		return x;
}
//触摸屏校准代码
void TP_Adjust(void)
{
    u16 pos_temp[4][2]; //坐标缓存值
    u8 cnt = 0;
    u16 d1, d2;
    u32 tem1, tem2;
    double fac;
    u16 outtime = 0;
    u8 DisDirTemp = 0;
    
    if(tp_dev.touchtype&0X80)
        return; //电容屏不用校正
    //Readying
    TPAdjustReady(); //先重启再校准
    DisDirTemp = lcddev.dir;
    LCD_Display_Dir(DIS_DIR_0); //默认调整配置
    tp_dev.touchtype = tp_dev.touchtype&(~0X07);
    tp_dev.touchtype |= (lcddev.dir & 0X03)<<1; //屏幕方向
    tp_dev.width = lcddev.width;
    tp_dev.height = lcddev.height;
    TPAjustFlag = 1; //标记进入TP_Adjust();

    cnt = 0;
    BACK_COLOR = WHITE;
    LCD_Clear(WHITE);  //清屏
    LCD_Clear(WHITE);  //清屏
    POINT_COLOR = BLACK;
    LCD_ShowString(40, 40, 160, 100, 16, (u8 *)TP_REMIND_MSG_TBL); //显示提示信息
    TP_Drow_Touch_Point(20, 20, RED);                              //画点1
    LCD_Exec();                                                    //执行显示
    tp_dev.sta = 0;                                                //消除触发信号
    tp_dev.xfac = 0;               //xfac用来标记是否校准过,所以校准之前必须清掉!以免错误
    while (1)                      //如果连续10秒钟没有按下,则自动退出
    {
        // tp_dev.scan(1);                          //扫描物理坐标
        if ((tp_dev.sta & 0xc0) == TP_CATH_PRES) //按键按下了一次(此时按键松开了.)
        {
            outtime = 0;
            tp_dev.sta &= ~(1 << 6); //标记按键已经被处理过了.

            pos_temp[cnt][0] = tp_dev.x[0];
            pos_temp[cnt][1] = tp_dev.y[0];
            // touch_debug("cnt: %d\r\n", cnt);
            // touch_debug("x: %d, y: %d\r\n", pos_temp[cnt][0], pos_temp[cnt][1]);
            cnt++;
            switch (cnt)
            {
            case 1:
                TP_Drow_Touch_Point(20, 20, WHITE);              //清除点1
                TP_Drow_Touch_Point(lcddev.width - 20, 20, RED); //画点2
                LCD_Exec();
                break;
            case 2:
                TP_Drow_Touch_Point(lcddev.width - 20, 20, WHITE); //清除点2
                TP_Drow_Touch_Point(20, lcddev.height - 20, RED);  //画点3
                LCD_Exec();
                break;
            case 3:
                TP_Drow_Touch_Point(20, lcddev.height - 20, WHITE);              //清除点3
                TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, RED); //画点4
                LCD_Exec();
                break;
            case 4:                                          //全部四个点已经得到
                                                             //对边相等
                tem1 = T_abs(pos_temp[0][0] - pos_temp[1][0]); //x1-x2
                tem2 = T_abs(pos_temp[0][1] - pos_temp[1][1]); //y1-y2
                tem1 *= tem1;
                tem2 *= tem2;
                d1 = T_sqrt(tem1 + tem2); //得到1,2的距离

                tem1 = T_abs(pos_temp[2][0] - pos_temp[3][0]); //x3-x4
                tem2 = T_abs(pos_temp[2][1] - pos_temp[3][1]); //y3-y4
                tem1 *= tem1;
                tem2 *= tem2;
                d2 = T_sqrt(tem1 + tem2); //得到3,4的距离
                fac = (float)d1 / d2;
                if (fac < 0.95 || fac > 1.05 || d1 == 0 || d2 == 0) //不合格
                {
                    cnt = 0;
                    TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE); //清除点4
                    TP_Drow_Touch_Point(20, 20, RED);                                  //画点1
                    TP_Adj_Info_Show(pos_temp[0][0], pos_temp[0][1], pos_temp[1][0], pos_temp[1][1],
                    pos_temp[2][0], pos_temp[2][1], pos_temp[3][0], pos_temp[3][1], fac * 100); //显示数据
                    LCD_Exec();
                    // touch_debug("1: %f, %d, %d\r\n", fac, d1, d2);
                    continue;
                }
                tem1 = T_abs(pos_temp[0][0] - pos_temp[2][0]); //x1-x3
                tem2 = T_abs(pos_temp[0][1] - pos_temp[2][1]); //y1-y3
                tem1 *= tem1;
                tem2 *= tem2;
                d1 = T_sqrt(tem1 + tem2); //得到1,3的距离

                tem1 = T_abs(pos_temp[1][0] - pos_temp[3][0]); //x2-x4
                tem2 = T_abs(pos_temp[1][1] - pos_temp[3][1]); //y2-y4
                tem1 *= tem1;
                tem2 *= tem2;
                d2 = T_sqrt(tem1 + tem2); //得到2,4的距离
                fac = (float)d1 / d2;
                if (fac < 0.95 || fac > 1.05) //不合格
                {
                    cnt = 0;
                    TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE); //清除点4
                    TP_Drow_Touch_Point(20, 20, RED);                                  //画点1
                    TP_Adj_Info_Show(pos_temp[0][0], pos_temp[0][1], pos_temp[1][0], pos_temp[1][1],
                    pos_temp[2][0], pos_temp[2][1], pos_temp[3][0], pos_temp[3][1], fac * 100); //显示数据
                    LCD_Exec();
                    // touch_debug("2: %f\r\n", fac);
                    continue;
                } //正确了

                //对角线相等
                tem1 = T_abs(pos_temp[1][0] - pos_temp[2][0]); //x1-x3
                tem2 = T_abs(pos_temp[1][1] - pos_temp[2][1]); //y1-y3
                tem1 *= tem1;
                tem2 *= tem2;
                d1 = T_sqrt(tem1 + tem2); //得到1,4的距离

                tem1 = T_abs(pos_temp[0][0] - pos_temp[3][0]); //x2-x4
                tem2 = T_abs(pos_temp[0][1] - pos_temp[3][1]); //y2-y4
                tem1 *= tem1;
                tem2 *= tem2;
                d2 = T_sqrt(tem1 + tem2); //得到2,3的距离
                fac = (float)d1 / d2;
                if (fac < 0.95 || fac > 1.05) //不合格
                {
                    cnt = 0;
                    TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE); //清除点4
                    TP_Drow_Touch_Point(20, 20, RED);                                  //画点1
                    TP_Adj_Info_Show(pos_temp[0][0], pos_temp[0][1], pos_temp[1][0], pos_temp[1][1],
                    pos_temp[2][0], pos_temp[2][1], pos_temp[3][0], pos_temp[3][1], fac * 100); //显示数据
                    LCD_Exec();
                    // touch_debug("3: %f\r\n", fac);
                    continue;
                } //正确了
                //计算结果
                tp_dev.xfac = (float)(lcddev.width - 40) / (pos_temp[1][0] - pos_temp[0][0]);       //得到xfac
                tp_dev.xoff = (lcddev.width - tp_dev.xfac * (pos_temp[1][0] + pos_temp[0][0])) / 2; //得到xoff

                tp_dev.yfac = (float)(lcddev.height - 40) / (pos_temp[2][1] - pos_temp[0][1]);       //得到yfac
                tp_dev.yoff = (lcddev.height - tp_dev.yfac * (pos_temp[2][1] + pos_temp[0][1])) / 2; //得到yoff
                if (T_abs(tp_dev.xfac) > 2 || T_abs(tp_dev.yfac) > 2)                       //触屏和预设的相反了.
                {
                    cnt = 0;
                    TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE); //清除点4
                    TP_Drow_Touch_Point(20, 20, RED);                                  //画点1
                    LCD_ShowString(40, 26, lcddev.width, lcddev.height, 16, (u8 *)"TP Need readjust!");
                    LCD_Exec();
                    tp_dev.touchtype = (tp_dev.touchtype&0XFE)+((~tp_dev.touchtype)&0x01); //X,Y轴调转
                    // touch_debug("4: \r\n");
                    continue;
                }
				// touch_debug("OK fac: %f\r\n", fac);
                POINT_COLOR = BLUE;
                LCD_Clear(WHITE);                                                                    //清屏
                LCD_ShowString(35, 110, lcddev.width, lcddev.height, 16, (u8 *)"Touch Screen Adjust OK!"); //校正完成
                LCD_Exec();
                delay_ms(1000);
                TP_Save_Adjdata(); //保存校正值
                LCD_Clear(WHITE);  //清屏
                LCD_Exec();
                LCD_Display_Dir(DisDirTemp);                  //恢复配置
                tp_dev.touchtype = tp_dev.touchtype&(~0X06);
                tp_dev.touchtype |= (lcddev.dir & 0X03) << 1; //屏幕方向
                tp_dev.width = lcddev.width;
                tp_dev.height = lcddev.height;
                TPAjustFlag = 0; //标记退出TP_Adjust();
                rtp_test(); //校准完成后测试效果
                return; //校正完成
            }
        }
        delay_ms(10);
        outtime++;
        if (outtime > 1000)
        {
            // TP_Get_Adjdata(); //取消, 超时照使用此次校准, 不保存
            LCD_Display_Dir(DisDirTemp); //恢复配置
            tp_dev.touchtype = tp_dev.touchtype&(~0X06);
            tp_dev.touchtype |= (lcddev.dir & 0X03)<<1; //屏幕方向
            tp_dev.width = lcddev.width;
            tp_dev.height = lcddev.height;
            break;
        }
    }
    TPAjustFlag = 0; //标记退出TP_Adjust();
}
****************************************************/

/***********五点校准*************/
typedef struct __attribute__((packed))
{
    /////////////////////触摸屏校准参数(电容屏不需要校准)//////////////////////
    int a[7];
    u8 touchtype;
    u8 flag; //校正确认标志
} _adjust_data;

//PS:注意储存系统要先初始化
//保存校准参数
void TP_Save_Adjdata(void)
{
    _adjust_data adjust_data_temp = {
        {
            touch_cal.a[0],
            touch_cal.a[1],
            touch_cal.a[2],
            touch_cal.a[3],
            touch_cal.a[4],
            touch_cal.a[5],
            touch_cal.a[6],
        },
        tp_dev.touchtype,
        0X0A //写0X0A标记校准过了
    };
    MTF_ROM_write((u8 *)&adjust_data_temp, SAVE_ADDR_BASE, sizeof(_adjust_data)); 
}

//得到保存的校准值
//返回值：1，成功获取数据
//        0，获取失败，要重新校准
u8 TP_Get_Adjdata(void)
{
    _adjust_data adjust_data_temp;
    MTF_ROM_read((u8 *)&adjust_data_temp, SAVE_ADDR_BASE, sizeof(_adjust_data)); //读取
    if (adjust_data_temp.flag == 0X0A)                                //触摸屏已经校准过了
    {
        touch_cal.a[0] = adjust_data_temp.a[0];
        touch_cal.a[1] = adjust_data_temp.a[1];
        touch_cal.a[2] = adjust_data_temp.a[2];
        touch_cal.a[3] = adjust_data_temp.a[3];
        touch_cal.a[4] = adjust_data_temp.a[4];
        touch_cal.a[5] = adjust_data_temp.a[5];
        touch_cal.a[6] = adjust_data_temp.a[6];
        tp_dev.touchtype = tp_dev.touchtype|(adjust_data_temp.touchtype&0X01); //X,Y轴位置
        if(TPAdjustReset())
            return 0;
        else
            return 1;
    }
    return 0;
}

//触摸屏校准代码
void TP_Adjust(void)
{
    u8 cnt = 0;
    u16 outtime = 0;
    u8 DisDirTemp = 0;
    
    if(tp_dev.touchtype&0X80)
    {
        rtp_test(); //测试触摸
        return; //电容屏不用校正
    }
    //Readying
    TPAdjustReady(); //先重启再校准
    DisDirTemp = lcddev.dir;
    LCD_Display_Dir(DIS_DIR_0); //默认调整配置
    tp_dev.touchtype = tp_dev.touchtype&(~0X07);
    tp_dev.touchtype |= (lcddev.dir & 0X03)<<1; //屏幕方向
    tp_dev.width = lcddev.width;
    tp_dev.height = lcddev.height;
    TPAjustFlag = 1; //标记进入TP_Adjust();

    BACK_COLOR = WHITE;
    LCD_Clear(WHITE);  //清屏
    LCD_Clear(WHITE);  //清屏
    POINT_COLOR = BLACK;
    LCD_ShowString(40, 40, 160, 100, 16, (u8 *)TP_REMIND_MSG_TBL); //显示提示信息
    TP_Drow_Touch_Point(20, 20, RED);                              //画点1
    cnt = 0;
    touch_cal.xfb[cnt] = 20;
    touch_cal.yfb[cnt] = 20;
    LCD_Exec();                                                    //执行显示
    tp_dev.sta = 0;                                                //消除触发信号
    tp_dev.xfac = 0;               //xfac用来标记是否校准过,所以校准之前必须清掉!以免错误
    while (1)                      //如果连续10秒钟没有按下,则自动退出
    {
        // tp_dev.scan(1);                          //扫描物理坐标
        if ((tp_dev.sta & 0xc0) == TP_CATH_PRES) //按键按下了一次(此时按键松开了.)
        {
            outtime = 0;
            tp_dev.sta &= ~(1 << 6); //标记按键已经被处理过了.
            if (cnt <= 4)
            {
                touch_cal.x[cnt] = tp_dev.x[0];
                touch_cal.y[cnt] = tp_dev.y[0];
                // touch_debug("cnt: %d, x: %d, y: %d\r\n", cnt, touch_cal.x[cnt], touch_cal.y[cnt]);
            }
            cnt++;
            switch (cnt)
            {
            case 1:
                TP_Drow_Touch_Point(20, 20, WHITE);              //清除点1
                TP_Drow_Touch_Point(lcddev.width - 20, 20, RED); //画点2
                touch_cal.xfb[cnt] = lcddev.width - 20;
                touch_cal.yfb[cnt] = 20;
                LCD_Exec();
                break;
            case 2:

                TP_Drow_Touch_Point(lcddev.width - 20, 20, WHITE); //清除点2
                TP_Drow_Touch_Point(20, lcddev.height - 20, RED);  //画点3
                touch_cal.xfb[cnt] = 20;
                touch_cal.yfb[cnt] = lcddev.height - 20;
                LCD_Exec();
                break;
            case 3:
                TP_Drow_Touch_Point(20, lcddev.height - 20, WHITE);              //清除点3
                TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, RED); //画点4
                touch_cal.xfb[cnt] = lcddev.width - 20;
                touch_cal.yfb[cnt] = lcddev.height - 20;
                LCD_Exec();
                break;
            case 4:
                TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE); //清除点4
                TP_Drow_Touch_Point(lcddev.width / 2, lcddev.height / 2, RED);     //画点5
                touch_cal.xfb[cnt] = lcddev.width / 2;
                touch_cal.yfb[cnt] = lcddev.height / 2;
                LCD_Exec();
                break;
            case 5:     //五个点已经得到
                if(perform_calibration(&touch_cal)==0)
                { //失败
                    cnt = 0;
                    TP_Drow_Touch_Point(lcddev.width/2, lcddev.height/2, WHITE); //清除点5
                    TP_Drow_Touch_Point(20, 20, RED);                            //画点1
                    LCD_ShowString(40, 145, 160, 20, 16, (u8 *)"cal fail");
                    LCD_Exec();
                    continue;
                }
                else
                {
                    TPAjustFlag = 0; //转换为屏幕坐标,以校验X, Y轴有没反转
                    TP_Drow_Touch_Point(lcddev.width/2, lcddev.height/2, WHITE); //清除点5
                    TP_Drow_Touch_Point(20, 20, RED);                            //画点1
                    LCD_Exec();
                }
                break;
            case 6: 
                //确认X, Y轴有没反转
                if (tp_dev.x[0] >= 10 && tp_dev.x[0] <= 30 && 
                    tp_dev.y[0] >= 10 && tp_dev.y[0] <= 30)
                    {

                    }
                    else
                    {
                        //X,Y轴调转
                        tp_dev.touchtype = (tp_dev.touchtype & 0XFE) + ((~tp_dev.touchtype) & 0x01);
                        cnt = 0;
                        LCD_ShowString(40, 145, 160, 20, 16, (u8 *)"x, y invert, do again");
                        LCD_Exec();
                        continue;
                    }
                POINT_COLOR = BLUE;
                LCD_Clear(WHITE);    //清屏
                LCD_ShowString(35, 110, lcddev.width, lcddev.height, 16, 
                                (u8 *)"Touch Screen Adjust OK!"); //校正完成
                LCD_Exec();
                delay_ms(1000);
                TP_Save_Adjdata(); //保存校正值
                LCD_Clear(WHITE);  //清屏
                LCD_Exec();
                LCD_Display_Dir(DisDirTemp);                  //恢复配置
                tp_dev.touchtype = tp_dev.touchtype&(~0X06);
                tp_dev.touchtype |= (lcddev.dir & 0X03) << 1; //屏幕方向
                tp_dev.width = lcddev.width;
                tp_dev.height = lcddev.height;
                TPAjustFlag = 0; //标记退出TP_Adjust();
                rtp_test(); //校准完成后测试效果
                return; //校正完成
            }
        }
        delay_ms(10);
        outtime++;
        if (outtime > 1000)
        {
            // TP_Get_Adjdata(); //取消, 超时照使用此次校准, 不保存
            LCD_Display_Dir(DisDirTemp); //恢复配置
            tp_dev.touchtype = tp_dev.touchtype&(~0X06);
            tp_dev.touchtype |= (lcddev.dir & 0X03)<<1; //屏幕方向
            tp_dev.width = lcddev.width;
            tp_dev.height = lcddev.height;
            break;
        }
    }
    TPAjustFlag = 0; //标记退出TP_Adjust();
}
///////////////////////////////////////////////////////////////////////////

/****************************
校准触发, 定时运行, 参数:输入状态(按下/松开)
****************************/
void TP_Adjust_trigger(void)
{
    static u8 count = 0, old_state = 0;
    static u16 time_count = 0;

    if(tp_dev.touchtype&0X80)
        return; //电容屏不用校正
    if(++time_count>=8000)
    {
        time_count = 0;
        count = 0;
    }
    if (tp_dev.sta & TP_PRES_DOWN) //触摸屏被按下
    {
        if(old_state)
        {
            old_state = 0;
            if(++count>=120)
            {
                TP_Adjust();
            }
        }
    }
    else
    {
        old_state = 1;
    }
}

/**************************************************************
 * touch test
 ************************************************************/
//draw photo
//画一个大点(2*2的点)
//x,y:坐标
//color:颜色
static void TP_Draw_Big_Point(u16 x, u16 y, ColorClass color)
{
    POINT_COLOR = color;
    LCD_DrawPoint(x, y); //中心点
    LCD_DrawPoint(x + 1, y);
    LCD_DrawPoint(x, y + 1);
    LCD_DrawPoint(x + 1, y + 1);
    LCD_Exec();
}

static void Load_Drow_Dialog(void)
{
	LCD_Clear(WHITE);	//清屏   
 	POINT_COLOR=BLUE;	//设置字体为蓝色 
	LCD_ShowString(lcddev.width-32,0,200,16,16,(u8 *)"RST ");//显示清屏区域
    LCD_ShowString(lcddev.width-32,24,200,16,16,(u8 *)"ADJ ");//显示退出区域
    LCD_ShowString(lcddev.width-32,48,200,16,16,(u8 *)"EXIT");//显示退出区域
  	POINT_COLOR=RED;	//设置画笔蓝色 
    LCD_Exec();
}

void rtp_test(void)
{  
    Load_Drow_Dialog();
	while(1)
	{
#ifndef _USER_DEBUG
        break;
#endif
        // tp_dev.scan(0); 		 
		if(tp_dev.sta&TP_PRES_DOWN)			//触摸屏被按下
		{	
		 	if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{	
				if(tp_dev.x[0]>(lcddev.width-32)&&tp_dev.y[0]<16)
                    Load_Drow_Dialog();//清除
                else if(tp_dev.x[0]>(lcddev.width-32)&&tp_dev.y[0]<40)
                    TP_Adjust(); //屏幕校准
                else if(tp_dev.x[0]>(lcddev.width-32)&&tp_dev.y[0]<64)
                    break;//退出
				else 
                    TP_Draw_Big_Point(tp_dev.x[0],tp_dev.y[0],RED);		//画图  			   
			}
		}
        else
        { 
            delay_ms(20);	//没有按键按下的时候 	    
        }
	}
    LCD_Clear(BLACK);
    LCD_Exec();
}
