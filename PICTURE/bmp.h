#ifndef __BMP_H__
#define __BMP_H__
#include "types_plus.h"

/////////////////////////////////////////////////////////////////////////////////////
//BMP编解码函数
u8 stdbmp_decode(const u8 *filename);
//尺寸小于240*320的bmp图片解码.
u8 minibmp_decode(u8 *filename, u16 x, u16 y, u16 width, u16 height, u16 acolor, u8 mode); 
u8 bmp_encode(u8 *filename, u16 x, u16 y, u16 width, u16 height);
u8 lodebmp_decode(unsigned char** out, unsigned* w, 
                        unsigned* h, const char *filename);
u8 lodebmp_decode_cut(unsigned char **out, const char *filename, 
                        u16 x1, u16 y1, u16 x2, u16 y2);
/////////////////////////////////////////////////////////////////////////////////////

#endif
