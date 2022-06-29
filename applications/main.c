/**
 * Copyright (c) 2022, Wenlong Fan (fanwenl@foxmail.com). All rights reserved.
 *
 * Change logs:
 * Date          Author          Notes
 * 2022-06-17    fanwenl         The first version.
 *
 */

#include <rtthread.h>
#include <board.h>
#include <string.h>
#include <stdio.h>
#include <fal.h>

#include <rtdevice.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

int main(void)
{
    fal_init();

    while (1)
    {
        rt_kprintf("hello world!\r\n");
        rt_thread_mdelay(2000);
    }

    return RT_EOK;
}
