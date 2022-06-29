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

#ifndef __DRV_GPIO_H__
#define __DRV_GPIO_H__

#include "mhscpu.h"
#include "mhscpu_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define __MH_PORT(port)  GPIO##port

#define GET_PIN(PORTx,PIN) (rt_base_t)((16 * ( ((rt_base_t)__MH_PORT(PORTx) - (rt_base_t)GPIO_BASE)/(0x10UL) )) + PIN)

struct port_irq_map
{
    rt_uint32_t line;
    IRQn_Type irqno;
};

int rt_hw_pin_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_GPIO_H__ */

