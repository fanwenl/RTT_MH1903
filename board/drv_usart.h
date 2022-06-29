/**
 * Copyright (c) 2022, Wenlong Fan (fanwenl@foxmail.com). All rights reserved.
 *
 * Change logs:
 * Date          Author          Notes
 * 2022-06-17    fanwenl         The first version.
 *
 */

#ifndef __DRV_USART_H__
#define __DRV_USART_H__

#include <rtthread.h>
#include "rtdevice.h"
#include <rthw.h>

int rt_hw_usart_init(void);

/* mh uart config class */
struct mh_uart_config
{
    const char *name;
    UART_TypeDef *Instance;
    IRQn_Type irq_type;
};

/* mh uart dirver class */
struct mh_uart
{
    struct mh_uart_config *config;
    struct rt_serial_device serial;
};

#endif  /* __DRV_USART_H__ */
