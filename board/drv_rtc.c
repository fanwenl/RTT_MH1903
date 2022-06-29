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

#include "board.h"

#ifdef BSP_USING_ONCHIP_RTC

//#define DRV_DEBUG
#define LOG_TAG             "drv.rtc"
#include <drv_log.h>

/* 初始时间 */
static rt_uint32_t time_sec = 1644891451;

static rt_err_t mh_rtc_init(void)
{
    SYSCTRL_APBPeriphClockCmd(SYSCTRL_APBPeriph_BPU, ENABLE);
    BPU->SEN_ANA0 |= BIT(10);

    RTC_ResetCounter();
	RTC_SetRefRegister(20);

    return RT_EOK;
}

static rt_err_t mh_rtc_get_secs(void *args)
{
    *(rt_uint32_t *) args = (RTC_GetCounter() + time_sec);
    LOG_D("RTC: get rtc_time %x\n", *(rt_uint32_t *)args);

    return RT_EOK;
}

static rt_err_t mh_rtc_set_secs(void *args)
{

    time_sec = *(rt_uint32_t *)args;
    RTC_ResetCounter();

    LOG_D("RTC: set rtc_time %x\n", *(rt_uint32_t *)args);

    return RT_EOK;
}

static const struct rt_rtc_ops mh_rtc_ops =
{
    mh_rtc_init,
    mh_rtc_get_secs,
    mh_rtc_set_secs,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
};

static rt_rtc_dev_t mh_rtc_dev;

static int rt_hw_rtc_init(void)
{
    rt_err_t result;

    mh_rtc_dev.ops = &mh_rtc_ops;
    result = rt_hw_rtc_register(&mh_rtc_dev, "rtc", RT_DEVICE_FLAG_RDWR, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("rtc register err code: %d", result);
        return result;
    }
    LOG_D("rtc init success");

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_rtc_init);
#endif /* BSP_USING_ONCHIP_RTC */
