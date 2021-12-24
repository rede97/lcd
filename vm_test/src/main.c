#include <stdio.h>
#include <memory.h>
#include "usblcd.h"
#include "VM.h"

static int usblcd_emit(struct usblcd *lcd)
{
    int i;
    for (i = 0; i < lcd->buf_idx; i++)
    {
        VM_Execute(lcd->buf_ep2[i]);
    }
    memset(lcd->buf_ep2, 0, sizeof(lcd->maxp_ep2_out));
    lcd->buf_idx = 0;
    lcd->buf_last_ins_idx = 0;
    return 0;
}

static int inline usblcd_code_free_space(const struct usblcd *lcd)
{
    return lcd->maxp_ep2_out - lcd->buf_idx;
}

static int inline usblcd_submit(struct usblcd *lcd, __u8 opcode, __u8 opdat, __u8 *dat, __u8 len)
{
    int submit_len = 0;
    while (submit_len < len)
    {
        int free_size = usblcd_code_free_space(lcd);
        if (free_size)
        {
        }
        else
        {
            
        }
    }
}

int main()
{
    return 0;
}