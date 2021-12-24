#ifndef VM_H
#define VM_H

#include <stdint.h>

typedef uint8_t UINT8;

#define OP_NOP 0x0      // 单长度扩展操作
#define OP_SET_IO 0x1   // 设置IO
#define OP_DELAY_US 0x2 // 延时微妙
#define OP_DELAY_MS 0x3 // 延时毫秒
#define OP_CMD_4 0x4    // 写4bit命令
#define OP_CMD_8 0x5    // 写8bit命令
#define OP_DAT_8 0x6    // 写8bit数据

#define GET_DATA(ins) (((ins)&0xf0) >> 4)
#define GET_OP(ins) ((ins)&0x0f)
#define MAKE_INS(op, dat) (((op)&0x0f) | ((dat) << 4))

void VM_Execute(UINT8 ins);

#endif