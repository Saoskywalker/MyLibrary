#ifndef _COMPORT_H
#define _COMPORT_H

#include "types_plus.h" //类型定义

//协议包含
#include "T5UIC2_agreement.h"
// #include "mb_m.h"
#include "MTF_ComProtocol.h"
#include "dgus2Com.h"

typedef u8 (*_pollHander)(void);
typedef u8 (*_readHander)(u8 *data, int *num); //应用读取接收数据
typedef u8 (*_sendHander)(u8 *data, int *num); //应用发送数据
typedef u8 (*_recProcessHander)(void); //后台接收处理
typedef u8 (*_sendProcessHander)(void); //后台发送处理

typedef struct
{
    _pollHander poll;
    _readHander read;
    _sendHander send;
    _recProcessHander recProcess;
    _sendProcessHander sendProcess;
} ComPort;

extern ComPort ComModel;

#endif
