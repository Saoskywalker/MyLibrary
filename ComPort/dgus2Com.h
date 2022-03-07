#ifndef _DGUSII_COM_H
#define _DGUSII_COM_H

/*协议为半双模式, 一定情况下可全双工*/
/*和MTF协议的区别为CRC计算的项目不同*/
/*地址1 选项1 后面的字节长度1 指令 数据 CRC16(指令+数据,低位在前)*/

#include "types_plus.h"

#define DGUSII_BUFF_LEN 256 //缓存大小

u8 DGUSII_Com_recData(void);
u8 DGUSII_Com_read(u8 *data, int *num);
u8 DGUSII_Com_sendData(u8 *data, int *num);
int DGUSII_Com_getState(void);
u8 DGUSII_Com_init(u8 addr, u8 option, u8 check);

#endif
