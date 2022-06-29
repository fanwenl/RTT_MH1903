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

#ifdef RT_USING_PWM
#include <drivers/rt_drv_pwm.h>

// #define DRV_DEBUG
#define LOG_TAG             "drv.pwm"
#include <drv_log.h>

#define MAX_PERIOD (UINT32_MAX)
#define MIN_PERIOD 3
#define MIN_PULSE 2

struct mh_pwm
{
    struct rt_device_pwm pwm_device;
    TIM_PWMInitTypeDef   tim_handle;
    char *name;
};

static struct mh_pwm mh_pwm_obj[] =
{
#ifdef BSP_USING_PWM0
    {.tim_handle.TIMx = TIM_0, .tim_handle.TIM_HighLevelPeriod = 100, .tim_handle.TIM_LowLevelPeriod = 100, .name = "pwm0"},
#endif

#ifdef BSP_USING_PWM1
    {.tim_handle.TIMx = TIM_1, .tim_handle.TIM_HighLevelPeriod = 100, .tim_handle.TIM_LowLevelPeriod = 100, .name = "pwm1"},
#endif

#ifdef BSP_USING_PWM2
    {.tim_handle.TIMx = TIM_2, .tim_handle.TIM_HighLevelPeriod = 100, .tim_handle.TIM_LowLevelPeriod = 100, .name = "pwm2"},
#endif

#ifdef BSP_USING_PWM3
    {.tim_handle.TIMx = TIM_3, .tim_handle.TIM_HighLevelPeriod = 100, .tim_handle.TIM_LowLevelPeriod = 100, .name = "pwm3"},
#endif

#ifdef BSP_USING_PWM4
    {.tim_handle.TIMx = TIM_4, .tim_handle.TIM_HighLevelPeriod = 100, .tim_handle.TIM_LowLevelPeriod = 100, .name = "pwm4"},
#endif

#ifdef BSP_USING_PWM5
    {.tim_handle.TIMx = TIM_5, .tim_handle.TIM_HighLevelPeriod = 100, .tim_handle.TIM_LowLevelPeriod = 100, .name = "pwm5"},
#endif

#ifdef BSP_USING_PWM6
    {.tim_handle.TIMx = TIM_6, .tim_handle.TIM_HighLevelPeriod = 100, .tim_handle.TIM_LowLevelPeriod = 100, .name = "pwm6"},
#endif

#ifdef BSP_USING_PWM7
    {.tim_handle.TIMx = TIM_7, .tim_handle.TIM_HighLevelPeriod = 100, .tim_handle.TIM_LowLevelPeriod = 100, .name = "pwm7"},
#endif
};


static rt_err_t drv_pwm_control(struct rt_device_pwm *device, int cmd, void *arg);
static struct rt_pwm_ops drv_ops =
{
    drv_pwm_control
};

static rt_err_t drv_pwm_get(TIM_PWMInitTypeDef *htim, struct rt_pwm_configuration *configuration)
{
    rt_uint64_t tim_clock;
    SYSCTRL_ClocksTypeDef clock;

    SYSCTRL_GetClocksFreq(&clock);
    tim_clock = (rt_uint32_t)clock.PCLK_Frequency;

    /* Convert nanosecond to frequency and duty cycle. 1s = 1 * 1000 * 1000 * 1000 ns */
    tim_clock /= 1000000UL;
    configuration->period = (htim->TIM_HighLevelPeriod +  htim->TIM_LowLevelPeriod + 1) * 1000UL / tim_clock;
    configuration->pulse = (htim->TIM_HighLevelPeriod + 1) * 1000UL / tim_clock;

    return RT_EOK;
}

static rt_err_t drv_pwm_set(TIM_PWMInitTypeDef *htim, struct rt_pwm_configuration *configuration)
{
    rt_uint32_t period, pulse;
    rt_uint64_t tim_clock;
    SYSCTRL_ClocksTypeDef clock;

    SYSCTRL_GetClocksFreq(&clock);
    tim_clock = (rt_uint32_t)clock.PCLK_Frequency;

    /* Convert nanosecond to frequency and duty cycle. 1s = 1 * 1000 * 1000 * 1000 ns */
    LOG_D("PWM config period:%u pulse:%u", configuration->period, configuration->pulse);

    if(tim_clock < (rt_uint64_t)(1000000000ULL / configuration->period))
    {
        LOG_E("PWM period must be > %u", 1000000000ULL / tim_clock);
        return RT_EINVAL;
    }

    tim_clock /= 1000000UL;
    period = (unsigned long long)configuration->period * tim_clock / 1000ULL ;
    if (period < MIN_PERIOD)
    {
        period = MIN_PERIOD;
    }

    if (period > MAX_PERIOD)
    {
        period = MAX_PERIOD;
    }

    pulse = (unsigned long long)configuration->pulse * tim_clock / 1000ULL;
    if (pulse < MIN_PULSE)
    {
        pulse = MIN_PULSE;
    }
    else if (pulse > period)
    {
        pulse = period;
    }
    LOG_D("PWM set tim period:%d pulse:%d", period, pulse);

    htim->TIM_HighLevelPeriod = pulse - 1;
    htim->TIM_LowLevelPeriod = period - pulse - 1;
    LOG_D("PWM set htim: %d low:%d high %d", htim->TIMx, htim->TIM_LowLevelPeriod, htim->TIM_HighLevelPeriod);
    TIM_SetPWMPeriod(TIMM0, htim->TIMx, htim->TIM_LowLevelPeriod, htim->TIM_HighLevelPeriod);
    return RT_EOK;
}

static rt_err_t drv_pwm_control(struct rt_device_pwm *device, int cmd, void *arg)
{
    struct rt_pwm_configuration *configuration = (struct rt_pwm_configuration *)arg;
    TIM_PWMInitTypeDef *htim = (TIM_PWMInitTypeDef *)device->parent.user_data;

    switch (cmd)
    {
    case PWMN_CMD_ENABLE:
        return RT_EINVAL;
    case PWM_CMD_ENABLE:
        TIM_Cmd(TIMM0, htim->TIMx, ENABLE);
        return RT_EOK;
    case PWMN_CMD_DISABLE:
        return RT_EINVAL;
    case PWM_CMD_DISABLE:
        TIM_Cmd(TIMM0, htim->TIMx, DISABLE);
        return RT_EOK;
    case PWM_CMD_SET:
        return drv_pwm_set(htim, configuration);
    case PWM_CMD_GET:
        return drv_pwm_get(htim, configuration);
    default:
        return RT_EINVAL;
    }
}

static rt_err_t mh_hw_pwm_init(struct mh_pwm *device)
{
    rt_err_t result = RT_EOK;
    TIM_PWMInitTypeDef *tim = RT_NULL;

    RT_ASSERT(device != RT_NULL);

    tim = (TIM_PWMInitTypeDef *)&device->tim_handle;

    /* configure the timer to pwm mode */
    TIM_PWMInit(TIMM0, tim);

    return result;
}

static int mh_pwm_init(void)
{
    int i = 0;
    int result = RT_EOK;

    SYSCTRL_APBPeriphClockCmd(SYSCTRL_APBPeriph_TIMM0, ENABLE);
	SYSCTRL_APBPeriphResetCmd(SYSCTRL_APBPeriph_TIMM0, ENABLE);

    for (i = 0; i < sizeof(mh_pwm_obj) / sizeof(mh_pwm_obj[0]); i++)
    {
        /* pwm init */
        if (mh_hw_pwm_init(&mh_pwm_obj[i]) != RT_EOK)
        {
            LOG_E("%s init failed", mh_pwm_obj[i].name);
            result = -RT_ERROR;
            goto __exit;
        }
        else
        {
            LOG_D("%s init success", mh_pwm_obj[i].name);

            /* register pwm device */
            if (rt_device_pwm_register(&mh_pwm_obj[i].pwm_device, mh_pwm_obj[i].name, &drv_ops, &mh_pwm_obj[i].tim_handle) == RT_EOK)
            {
                LOG_D("%s register success", mh_pwm_obj[i].name);
            }
            else
            {
                LOG_E("%s register failed", mh_pwm_obj[i].name);
                result = -RT_ERROR;
            }
        }
    }

__exit:
    return result;
}
INIT_DEVICE_EXPORT(mh_pwm_init);
#endif /* RT_USING_PWM */
