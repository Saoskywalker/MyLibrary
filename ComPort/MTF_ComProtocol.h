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
