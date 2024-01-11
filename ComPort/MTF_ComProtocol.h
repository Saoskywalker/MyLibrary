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

#ifndef _MTF_COMPROTOCOL_H
#define _MTF_COMPROTOCOL_H

/*协议为半双模式, 一定情况下可全双工, MODBUS自定义功能码*/
/*地址1 选项1 后面的字节长度1 指令 数据 CRC16(地址1+选项1+后面的字节长度1+指令+数据,低位在前)*/

#include "types_plus.h"

#define MTF_BUFF_LEN 256 //缓存大小

u8 MTF_Com_recData(void);
u8 MTF_Com_read(u8 *data, int *num);
u8 MTF_Com_sendData(u8 *data, int *num);
int MTF_Com_getState(void);
u8 MTF_Com_init(u8 addr, u8 option, u8 check);

#endif
