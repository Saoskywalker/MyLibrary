/*
Copyright (c) 2019-2023 Aysi 773917760@qq.com. All right reserved
Official site: www.mtf123.club

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

It under the terms of the Apache as published;
either version 2 of the License,
or (at your option) any later version.
*/

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
