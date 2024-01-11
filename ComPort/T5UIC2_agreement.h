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

#ifndef _T5UIC2_H
#define _T5UIC2_H

#include "types_plus.h"

#define T5_BUFF_LEN 1024 //缓存大小

u8 T5UIC2_recData(void);
u8 T5UIC2_read(u8 *data, int *num);
u8 T5UIC2_sendData(u8 *data, int *num);
int T5UIC2_getState(void);
u8 T5UIC2_init(u8 check);

#endif
