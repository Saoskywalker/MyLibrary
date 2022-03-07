#include "T5UIC2_agreement.h"
#include "Sagittarius_global.h"
#include "uart_port.h"
#include "GPIO_port.h"
#include "stdio.h"

//fifo full pin
//IO定义为移植方便, 放于GPIO_port.h

static u8 T5UIC2_check_sum = 0;
static u8 T5UIC2_head[] = {0XAA};
static u8 T5UIC2_end[] = {0XCC, 0X33, 0XC3, 0X3C};
static int byteCnt = 0, recInPrt = 0, recOutPrt = 0;
static u8 recBuffer[T5_BUFF_LEN+32]; //加32为了在2级FIFO满时,清除硬件fifo剩余的数据
static u8 _fifoReset = 0;

u8 T5UIC2_init(u8 check)
{
    T5L_BUSY_PIN(1);
    T5UIC2_check_sum = check;
    return 0;
}

int T5UIC2_getState(void)
{
    return byteCnt;
}

u8 T5UIC2_read(u8 *data, int *num)
{
    static u8 step = 0;
    static int i = 0;
    static u8 overTime = 0, frameStart = 0;
    u8 rec = 0;
    u16 sum = 0;
    u16 k = 0;

    if(frameStart)
    {
        if(++overTime>2) //about 6ms
        { 
            _fifoReset = 1;   
            T5L_BUSY_PIN(0); //通知有错    
            *num = 0;
            i = 0;
            overTime = 0;      
            frameStart = 0;
            step = 0;
            printf("uart_reset\r\n");
            MTF_UART_Reset(&uart_hmi);
            recInPrt = 0;
            recOutPrt = 0;
            byteCnt = 0;
            T5L_BUSY_PIN(1);
            _fifoReset = 0;
        }
    }
    else
    {
        overTime = 0;
    }
    
    while(byteCnt) //fifo has data
    {
        if(*num>=T5_BUFF_LEN)
            return 2; //帧过大
        overTime = 0;
        rec = recBuffer[recOutPrt];
        // printf("%#X, ", rec);
        byteCnt--;
        switch (step)
        {
        case 0:
            if (rec == 0xAA)
            {
                *num = 0;
                frameStart = 1;
                step++;
                i = 0;
                // printf("0xaa, ");
            }
            break;
        case 1:
            *num = *num+1;
            data[*num-1] = rec;
            break;
        default:
            step = 0;
            break;
        }

        if(frameStart)
        {
            if (i == 1)
            {
                if (rec == 0x33)
                    i++;
                else
                    i = 0;
            }
            else if (i == 2)
            {
                if (rec == 0xC3)
                    i++;
                else
                    i = 0;
            }
            else if (i == 3)
            {
                if (rec == 0x3C)
                    i++;
                else
                    i = 0;
            }
            if (i == 0 && rec == 0xCC)
            {
                i++;
            }
        }
        if(++recOutPrt>T5_BUFF_LEN-1)
            recOutPrt = 0;
        // printf(" i: %d", i);
        if (i == 4)
        {
            if (T5UIC2_check_sum) //开启和校验
            {
                for (k = 0; k < *num - 6; k++)
                    sum += data[k];
                if (sum == (((u16)data[*num - 6] << 8) + data[*num - 5]))
                {
                    i = 0;
                    step = 0;
                    *num -= 6;
                    frameStart = 0;
                    return 0; //有一帧
                }
                else
                {
                    i = 0;
                    step = 0;
                    *num -= 6;
                    frameStart = 0;
                    return 2; //帧错误
                }
            }
            i = 0;
            step = 0;
            *num -= 4;
            frameStart = 0;
            // printf("check: %d\r\n", byteCnt);
            return 0; //有一帧
        }
    }
    return 1; //没一帧
}

u8 T5UIC2_sendData(u8 *data, int *num)
{
    u16 i = 0;
    u16 sum = 0;
    u8 j = 0;

    for (i = 0; i<sizeof(T5UIC2_head); i++)
        MTF_UART_Transmit(&uart_hmi, &T5UIC2_head[i], 1);

    //BODY
    for (i = 0; i<*num; i++)
    {
        MTF_UART_Transmit(&uart_hmi, &data[i], 1);
    }

    if (T5UIC2_check_sum) //开启和校验
    {
        for (i = 0; i < *num; i++)
            sum += data[i];
        j = (u8)(sum>>8);
        MTF_UART_Transmit(&uart_hmi, &j, 1);
        j = (u8)sum;
        MTF_UART_Transmit(&uart_hmi, &j, 1);
    }

    for (i = 0; i<sizeof(T5UIC2_end); i++)
        MTF_UART_Transmit(&uart_hmi, &T5UIC2_end[i], 1);

    return 0;
}

u8 T5UIC2_recData(void)
{
    u8 byte, cnt, i;

    if (_fifoReset)
        return 2;
        
    if(byteCnt<T5_BUFF_LEN)
    {
        cnt = MTF_UART_Receive_FIFO_Count(&uart_hmi);
        if (cnt)
        {
            for (i = cnt; i > 0; i--)
            {
                MTF_UART_Receive(&uart_hmi, &byte, 1);
                recBuffer[recInPrt] = byte; //2级fifo
                if (++recInPrt > T5_BUFF_LEN - 1)
                    recInPrt = 0;
                if (++byteCnt >= T5_BUFF_LEN) //2级fifo满
                {
                    //清除硬件fifo剩余的数据, buffer已为此预留32B
                    MTF_UART_Receive(&uart_hmi, &recBuffer[recInPrt], i - 1);
                    T5L_BUSY_PIN(0);
                    return 1;
                }
            }
            // printf("%#X, ", byte);
            // printf("rec: %d\r\n", byteCnt);
        }
        T5L_BUSY_PIN(1);
    }
    return 0;
}

void T5UIC2_recData_irq(void)
{
    T5UIC2_recData();
}
