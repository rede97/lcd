#include "usblcd.h"

enum SUBMIT_STATUS
{
    SUBMIT_INS,
    SUBMIT_DAT,
    SUBMIT_FINISH,
};

int usblcd_emit(struct usblcd *lcd)
{
    int len = 0;
    int ret = usb_bulk_msg(lcd->usbdev, lcd->pipe_ep2_out, lcd->buf_ep2, lcd->buf_idx, &len, 20 * HZ);
    if (ret)
    {
        dev_alert(lcd->device->parent, "usblcd submit err: %d\n", ret);
        return ret;
    }
    memset(lcd->buf_ep2, 0, sizeof(lcd->maxp_ep2_out));
    lcd->buf_idx = 0;
    lcd->buf_last_ins_idx = -1;
    return 0;
}

static size_t inline usblcd_code_free_space(const struct usblcd *lcd)
{
    return lcd->maxp_ep2_out - lcd->buf_idx;
}

int usblcd_submit(struct usblcd *lcd, __u8 opcode, __u8 opdat, const __u8 *dat, __u8 len)
{
    enum SUBMIT_STATUS status = (OP_NOP == opcode) ? SUBMIT_DAT : SUBMIT_INS;
    __u8 submited_len = 0;
    while (status != SUBMIT_FINISH)
    {
        size_t free_size = usblcd_code_free_space(lcd);
        switch (status)
        {
        case SUBMIT_INS:
            if (free_size--)
            {
                if (OP_CMD_8 == opcode || OP_DAT_8 == opcode)
                {
                    lcd->buf_last_ins_idx = lcd->buf_idx;
                }
                else
                {
                    lcd->buf_last_ins_idx = -1;
                }
                lcd->buf_ep2[lcd->buf_idx++] = MAKE_INS(opcode, opdat);
                status = SUBMIT_DAT;
            }
            else
            {
                break;
            }
        case SUBMIT_DAT:
            if (submited_len < len)
            {
                if (free_size)
                {
                    size_t should_submit_len = min((size_t)(len - submited_len), free_size);
                    memcpy(&lcd->buf_ep2[lcd->buf_idx], &dat[submited_len], should_submit_len);
                    lcd->buf_idx += should_submit_len;
                    submited_len += should_submit_len;
                }
                else
                {
                    break;
                }
            }
            else
            {
                status = SUBMIT_FINISH;
            }
            break;
        default:
            break;
        }
        if (lcd->buf_idx > lcd->maxp_ep2_out / 2)
        {
            int ret = usblcd_emit(lcd);
            if (ret)
            {
                return ret;
            }
        }
    }
    return 0;
}

static inline int usblcd_ins_seq(struct usblcd *lcd, const __u8 SELF_OP, const __u8 *dat, size_t len)
{
    int ret;
    size_t submited_len = 0;
    if (lcd->buf_last_ins_idx >= 0)
    {
        __u8 last_ins = lcd->buf_ep2[lcd->buf_last_ins_idx];
        __u8 ins_cnt = GET_DATA(last_ins);
        if (SELF_OP == GET_OP(last_ins) && ins_cnt < 15)
        {
            __u8 should_submit_len = (__u8)min((size_t)15 - (size_t)ins_cnt, len);
            lcd->buf_ep2[lcd->buf_last_ins_idx] = MAKE_INS(SELF_OP, ins_cnt + should_submit_len);
            ret = usblcd_submit(lcd, OP_NOP, 0, dat, should_submit_len);
            if (ret)
            {
                return ret;
            }
            submited_len += should_submit_len;
        }
    }
    while (submited_len < len)
    {
        __u8 should_submit_len = (__u8)min((size_t)16, len - (size_t)submited_len);
        ret = usblcd_submit(lcd, SELF_OP, should_submit_len - 1, &dat[submited_len], should_submit_len);
        if (ret)
        {
            return ret;
        }
        submited_len += should_submit_len;
    }
    return 0;
}

int usblcd_cmd8(struct usblcd *lcd, __u8 cmd)
{
    return usblcd_ins_seq(lcd, OP_CMD_8, &cmd, 1);
}

int usblcd_dat8(struct usblcd *lcd, const __u8 *dat, size_t len)
{
    return usblcd_ins_seq(lcd, OP_DAT_8, dat, len);
}

int usblcd_cmd4(struct usblcd *lcd, __u8 cmd)
{
    return usblcd_submit(lcd, OP_CMD_4, cmd, NULL, 0);
}

int usblcd_udelay(struct usblcd *lcd, __u8 repeat, __u8 micros)
{
    return usblcd_submit(lcd, OP_DELAY_US, repeat - 1, &micros, 1);
}

int usblcd_mdelay(struct usblcd *lcd, __u8 repeat, __u8 mills)
{
    return usblcd_submit(lcd, OP_DELAY_MS, repeat - 1, &mills, 1);
}
