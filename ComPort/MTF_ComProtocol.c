#include "MTF_ComProtocol.h"
#include "Sagittarius_global.h"
#include "uart_port.h"
#include "stdio.h"
#include "mbcrc.h"
#include "GPIO_port.h"

//IO定义为移植方便, 放于GPIO_port.h

static u8 _addr = 0X5A, _option = 0XA5, _check_sum = 0;
static u8 ReadSend = 0; //1 send; 0 read 主从状态标志
static int byteCnt = 0, recInPrt = 0, recOutPrt = 0;
static u8 recBuffer[MTF_BUFF_LEN+32]; //加32为了在2级FIFO满时,清除硬件fifo剩余的数据
static u8 _fifoReset = 0;

static inline void _send_mode(void)
{
    while(MTF_UART_Receive_Empty(&uart_hmi)); //可不用
    ReadSend = 1; 
    RDorDE_PIN(1);
}

static inline void _read_mode(void) 
{
    while(MTF_UART_Transmit_Empty(&uart_hmi)==0);
    ReadSend = 0; 
    RDorDE_PIN(0);
}

u8 MTF_Com_init(u8 addr, u8 option, u8 check)
{
    RDorDE_PIN(0);
    ReadSend = 0;
    _addr = addr;
    _option = option;
    _check_sum = check;
    return 0;
}

int MTF_Com_getState(void)
{
    return byteCnt;
}

u8 MTF_Com_read(u8 *data, int *num)
{
    static u8 step = 0;
    static u8 overTime = 0, frameStart = 0;
    static u8 frameLength = 0, frameCnt = 0;
    u8 rec = 0;
    u16 crc16 = 0;
    u8 head[3];

    if(frameStart)
    {
        if(++overTime>2) //about 6ms
        {        
            _fifoReset = 1;  
            *num = 0;
            overTime = 0;      
            frameStart = 0;
            step = 0;
            frameCnt = 0;
            printf("uart_reset\r\n");
            MTF_UART_Reset(&uart_hmi);
            recInPrt = 0;
            recOutPrt = 0;
            byteCnt = 0;
            _fifoReset = 0;
        }
    }
    else
    {
        *num = 0;
        overTime = 0;
        step = 0;
        frameCnt = 0;
    }
    
    while(byteCnt) //fifo has data
    {
        if(*num>=MTF_BUFF_LEN)
            return 2; //帧过大
        overTime = 0;
        rec = recBuffer[recOutPrt];
        // printf("%#X, ", rec);
        byteCnt--;
        switch (step)
        {
        case 0:
            if (rec == _addr) //检头
            {
                frameCnt = 0;
                *num = 0;
                frameStart = 1;
                step++;
            }
            break;
        case 1:
            if (rec == _option)
            {
                step++;
            }
            else
            {
                step = 0;
            }
            break;
        case 2:
            if (rec!=0)
            {
                frameLength = rec;
                step++;
            }
            else
            {
                step = 0;
            }
            break;
        case 3:
            *num = *num + 1;
            data[*num - 1] = rec;
            break;
        default:
            step = 0;
            break;
        }

        if(++recOutPrt>MTF_BUFF_LEN-1) //缓存位置, 到缓存尾部, 回头
            recOutPrt = 0;

        if (step == 3)
        {
            if (++frameCnt > frameLength) //检数量
            {
                if (_check_sum) //开启校验
                {
                    head[0] = _addr;
                    head[1] = _option;
                    head[2] = frameLength;
                    crc16 = usMBCRC16_multi(&head[0], sizeof(head), data, frameLength - 2);
                    if (crc16 == (((u16)data[*num - 1] << 8) + data[*num - 2]))
                    {
                        *num -= 2;
                        frameStart = 0;
                        return 0; //有一帧
                    }
                    else
                    {
                        frameStart = 0;
                        return 2; //帧错误
                    }
                }
                frameStart = 0;
                // printf("check: %d\r\n", byteCnt);
                return 0; //有一帧
            }
        }
    }
    return 1; //没一帧
}

u8 MTF_Com_sendData(u8 *data, int *num)
{
    u16 i = 0;
    u16 crc16 = 0;
    u8 head[3];
    u8 t[2];

    _send_mode();

    MTF_UART_Transmit(&uart_hmi, &_addr, 1);
    MTF_UART_Transmit(&uart_hmi, &_option, 1);

    if (_check_sum) //开启校验
    {
        t[0] = (u8)(*num+2);
        MTF_UART_Transmit(&uart_hmi, &t[0], 1);
    }
    else
    {
        t[0] = (u8)(*num);
        MTF_UART_Transmit(&uart_hmi, &t[0], 1);
    }

    //BODY
    for (i = 0; i<*num; i++)
    {
        MTF_UART_Transmit(&uart_hmi, &data[i], 1);
    }

    if (_check_sum) //开启校验
    {
        head[0] = _addr;
        head[1] = _option;
        head[2] = *num+2;
        crc16 = usMBCRC16_multi(&head[0], sizeof(head), data, *num);
        t[0] = (u8)crc16;
        t[1] = (u8)(crc16 >> 8);
        MTF_UART_Transmit(&uart_hmi, &t[0], 1);
        MTF_UART_Transmit(&uart_hmi, &t[1], 1);
    }

    _read_mode();

    return 0;
}

u8 MTF_Com_recData(void)
{
    u8 byte, cnt, i;

    if(ReadSend) //发状态, 可不用
        return 3;

    if (_fifoReset)
        return 2;

    if(byteCnt<MTF_BUFF_LEN)
    {
        cnt = MTF_UART_Receive_FIFO_Count(&uart_hmi);
        if (cnt)
        {
            for (i = cnt; i > 0; i--)
            {
                MTF_UART_Receive(&uart_hmi, &byte, 1);
                recBuffer[recInPrt] = byte; //2级fifo
                if (++recInPrt > MTF_BUFF_LEN - 1)
                    recInPrt = 0;
                if (++byteCnt >= MTF_BUFF_LEN) //2级fifo满
                {
                    //清除硬件fifo剩余的数据, buffer已为此预留32B
                    MTF_UART_Receive(&uart_hmi, &recBuffer[recInPrt], i - 1);
                    return 1;
                }
            }
            // printf("%#X, ", byte);
            // printf("rec: %d\r\n", byteCnt);
        }
    }
    return 0;
}
