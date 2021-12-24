#include "VM.h"

#include <stdio.h>
#include <string.h>

void VM_Execute(UINT8 ins)
{
    static UINT8 status = OP_NOP;
    UINT8 last_op = GET_OP(status);
    UINT8 last_dat = GET_DATA(status);
    UINT8 curt_dat = 0;
    int v = last_op;
    // printf("last: %d\n", v);
    v = ins & 0xff;
    switch (last_op)
    {
    case OP_NOP:
        switch (GET_OP(ins))
        {
        case OP_NOP:
            status = OP_NOP;
            break;
        case OP_SET_IO:
            curt_dat = GET_DATA(ins);
            printf("set io %x\n", curt_dat);
            // setio(curt_dat);
            break;
        case OP_DELAY_US:
        case OP_DELAY_MS:
            status = ins;
            break;
        case OP_CMD_4:
            // curt_dat = GET_DATA(ins);
            // cmd4(curt_dat);
            v = curt_dat & 0x0f;
            printf("cmd4 %x\n", v);
            break;
        case OP_CMD_8:
        case OP_DAT_8:
            status = ins;
            break;
        default:
            break;
        }
        break;
    case OP_DELAY_US:
        do
        {
            // mDelayuS(ins);
            printf("delay us %d\n", v);
        } while (last_dat--);
        status = OP_NOP;
        break;
    case OP_DELAY_MS:
        do
        {
            // mDelaymS(ins);
            printf("delay ms %d\n", v);
        } while (last_dat--);
        status = OP_NOP;
        break;
    case OP_CMD_8:
        // cmd4(ins >> 4);
        // cmd4(ins & 0x0f);
        printf("cmd 8 %x\n", v);
        if (last_dat)
        {
            status = MAKE_INS(OP_CMD_8, last_dat - 1);
        }
        else
        {
            status = OP_NOP;
        }
        break;
    case OP_DAT_8:
        // dat4(ins >> 4);
        // dat4(ins & 0x0f);
        printf("dat 8 %x\n", v);
        if (last_dat)
        {
            status = MAKE_INS(OP_DAT_8, last_dat - 1);
        }
        else
        {
            status = OP_NOP;
        }
        break;
    default:
        status = OP_NOP;
        break;
    }
}
