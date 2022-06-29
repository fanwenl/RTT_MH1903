/**
 * Copyright (c) 2022, Wenlong Fan (fanwenl@foxmail.com). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change logs:
 * Date          Author          Notes
 * 2022-06-17    fanwenl         The first version.
 *
 */

#include <board.h>

#ifdef RT_USING_WDT
#include "mhscpu_wdt.h"

//#define DRV_DEBUG
#define LOG_TAG             "drv.wdt"
#include <drv_log.h>

struct mh_wdt_obj
{
    rt_watchdog_t watchdog;
    rt_uint32_t timeout;
};

static struct mh_wdt_obj mh_wdt;
static struct rt_watchdog_ops ops;

static rt_err_t wdt_init(rt_watchdog_t *wdt)
{
    return RT_EOK;
}

static rt_err_t wdt_control(rt_watchdog_t *wdt, int cmd, void *arg)
{
    switch (cmd)
    {
        /* feed the watchdog */
    case RT_DEVICE_CTRL_WDT_KEEPALIVE:
        WDT_ReloadCounter();
        break;
        /* set watchdog timeout */
    case RT_DEVICE_CTRL_WDT_SET_TIMEOUT:
        mh_wdt.timeout = *((rt_uint32_t*)arg);

        if((mh_wdt.timeout * SYSCTRL->PCLK_1MS_VAL * 1000) > 0xFFFFFFFF)
        {
            LOG_E("wdg set timeout parameter too large, please less than %ds",0xFFFFFFFF / (SYSCTRL->PCLK_1MS_VAL * 1000));
            return -RT_EINVAL;
        }

        WDT_SetReload(SYSCTRL->PCLK_1MS_VAL * mh_wdt.timeout * 1000);
        break;
    case RT_DEVICE_CTRL_WDT_GET_TIMEOUT:
        (*((rt_uint32_t*)arg)) = mh_wdt.timeout;
        break;
    case RT_DEVICE_CTRL_WDT_START:
        WDT_ModeConfig(WDT_Mode_CPUReset);
        WDT_Enable();
        break;
    default:
        LOG_W("This command is not supported.");
        return -RT_ERROR;
    }
    return RT_EOK;
}

int rt_wdt_init(void)
{
    ops.init = &wdt_init;
    ops.control = &wdt_control;
    mh_wdt.watchdog.ops = &ops;
    /* register watchdog device */
    if (rt_hw_watchdog_register(&mh_wdt.watchdog, "wdt", RT_DEVICE_FLAG_DEACTIVATE, RT_NULL) != RT_EOK)
    {
        LOG_E("wdt device register failed.");
        return -RT_ERROR;
    }
    LOG_D("wdt device register success.");
    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_wdt_init);

#endif /* RT_USING_WDT */
