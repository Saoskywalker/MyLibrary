#include "ComPort.h"

static u8 _init1(void)
{
    return 1; //错误:无初始化
}

static u8 _init2(u8 *data, int *num)
{
    return 1; //错误:无初始化
}

ComPort ComModel = {_init1, _init2, _init2, _init1, _init1};
