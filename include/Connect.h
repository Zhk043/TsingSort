#pragma once
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "DefineUtils.h"

#define NET_BUFFER_SIZE (9000) //数据包大小

/************* 以太网数据段用户自定义数据结构  Packet Header   与屏之间通讯协议********/
#pragma pack(push, 1)
typedef struct
{
    u8 boardId;       // boardId 和本地IP的最后一位数相同
    u8 operatorID;    // operatorID 0x01/0x02表示读写命令；0x81/0x82表示读写数据
    u32 startAddress; // startAddress 在命令模式下为寄存器地址，在数据模式下为数据类型
    u16 len;          // 在命令模式下为寄存器个数（寄存器32bit），在数据模式下为数据长度（字节数）
    u16 packetId;     // 包号，屏产生，每下发一帧包号加1，如此循环，响应帧包号不改变；
    u8  rev1;           // 预留
    u32 rev2; // 预留
    union
    {
        u8 Bbuffer[NET_BUFFER_SIZE];
        u32 Dbuffer[NET_BUFFER_SIZE / 4];
    };
} YLS_ENET_FRAME;
#pragma pack(pop)
//包头长度
#define YLS_NET_HEADER 15
