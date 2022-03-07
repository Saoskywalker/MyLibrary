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
