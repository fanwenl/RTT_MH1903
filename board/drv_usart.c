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

#ifdef RT_USING_SERIAL
#include "drv_usart.h"

#define DRV_DEBUG
#define LOG_TAG             "drv.usart"
#include <drv_log.h>

#if !defined(BSP_USING_UART0) && !defined(BSP_USING_UART1) && !defined(BSP_USING_UART2) && !defined(BSP_USING_UART3)
#error "Please define at least one BSP_USING_UARTx"
/* this driver can be disabled at menuconfig -> RT-Thread Components -> Device Drivers */
#endif
enum
{
#ifdef BSP_USING_UART0
    UART0_INDEX,
#endif
#ifdef BSP_USING_UART1
    UART1_INDEX,
#endif
#ifdef BSP_USING_UART2
    UART2_INDEX,
#endif
#ifdef BSP_USING_UART3
    UART3_INDEX,
#endif
};

static struct mh_uart_config uart_config[] =
{
#ifdef BSP_USING_UART0
    {"uart0",UART0, UART0_IRQn},
#endif
#ifdef BSP_USING_UART1
    {"uart1",UART1, UART1_IRQn},
#endif
#ifdef BSP_USING_UART2
    {"uart2",UART2, UART2_IRQn},
#endif
#ifdef BSP_USING_UART3
    {"uart3",UART3, UART3_IRQn},
#endif
};

static struct mh_uart uart_obj[sizeof(uart_config) / sizeof(uart_config[0])] = {0};

static rt_err_t mh_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    struct mh_uart *uart;
    UART_InitTypeDef UART_InitStruct;
    NVIC_InitTypeDef NVIC_InitStructure;

    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    uart = rt_container_of(serial, struct mh_uart, serial);
    UART_InitStruct.UART_BaudRate = cfg->baud_rate;

    if(uart->config->Instance == UART0)
    {
        SYSCTRL_APBPeriphClockCmd(SYSCTRL_APBPeriph_UART0 | SYSCTRL_APBPeriph_GPIO, ENABLE);
        SYSCTRL_APBPeriphResetCmd(SYSCTRL_APBPeriph_UART0, ENABLE);
        GPIO_PinRemapConfig(GPIOA, GPIO_Pin_0 | GPIO_Pin_1, GPIO_Remap_0);
    }
    else if(uart->config->Instance == UART1)
    {
        SYSCTRL_APBPeriphClockCmd(SYSCTRL_APBPeriph_UART1 | SYSCTRL_APBPeriph_GPIO, ENABLE);
        SYSCTRL_APBPeriphResetCmd(SYSCTRL_APBPeriph_UART1, ENABLE);
        GPIO_PinRemapConfig(GPIOB, GPIO_Pin_12 | GPIO_Pin_13, GPIO_Remap_3);
    }
    else if(uart->config->Instance == UART2)
    {
        SYSCTRL_APBPeriphClockCmd(SYSCTRL_APBPeriph_UART2 | SYSCTRL_APBPeriph_GPIO, ENABLE);
        SYSCTRL_APBPeriphResetCmd(SYSCTRL_APBPeriph_UART2, ENABLE);
        GPIO_PinRemapConfig(GPIOB, GPIO_Pin_2 | GPIO_Pin_3, GPIO_Remap_3);
    }
    else if(uart->config->Instance == UART3)
    {
        SYSCTRL_APBPeriphClockCmd(SYSCTRL_APBPeriph_UART3 | SYSCTRL_APBPeriph_GPIO, ENABLE);
        SYSCTRL_APBPeriphResetCmd(SYSCTRL_APBPeriph_UART3 , ENABLE);
        GPIO_PinRemapConfig(GPIOE, GPIO_Pin_8 | GPIO_Pin_9 , GPIO_Remap_2);
    }

    switch (cfg->data_bits)
    {
    case DATA_BITS_5:
        UART_InitStruct.UART_WordLength = UART_WordLength_5b;
        break;
    case DATA_BITS_6:
        UART_InitStruct.UART_WordLength = UART_WordLength_6b;
        break;
    case DATA_BITS_7:
        UART_InitStruct.UART_WordLength = UART_WordLength_7b;
        break;
    case DATA_BITS_8:
        UART_InitStruct.UART_WordLength = UART_WordLength_8b;
        break;
    default:
        UART_InitStruct.UART_WordLength = UART_WordLength_8b;
        break;
    }

    switch (cfg->stop_bits)
    {
    case STOP_BITS_1:
        UART_InitStruct.UART_StopBits = UART_StopBits_1;
        break;
    case STOP_BITS_2:
        UART_InitStruct.UART_StopBits = UART_StopBits_2;
        break;
    default:
        UART_InitStruct.UART_StopBits = UART_StopBits_1;
        break;
    }

    switch (cfg->parity)
    {
    case PARITY_NONE:
        UART_InitStruct.UART_Parity = UART_Parity_No;
        break;
    case PARITY_ODD:
        UART_InitStruct.UART_Parity = UART_Parity_Odd;
        break;
    case PARITY_EVEN:
        UART_InitStruct.UART_Parity = UART_Parity_Even;
        break;
    default:
        UART_InitStruct.UART_Parity = UART_Parity_No;
        break;
    }

    NVIC_InitStructure.NVIC_IRQChannel = uart->config->irq_type;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    UART_Init(uart->config->Instance, &UART_InitStruct);

    return RT_EOK;
}

static rt_err_t mh_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct mh_uart *uart;
    uint32_t uart_it = 0;

    RT_ASSERT(serial != RT_NULL);
    uart = rt_container_of(serial, struct mh_uart, serial);

    if((rt_uint32_t)arg == RT_DEVICE_FLAG_INT_RX)
    {
        uart_it = UART_IT_RX_RECVD;
    }
    else
    {
        uart_it = UART_IT_TX_EMPTY;
    }

    switch (cmd)
    {
    /* disable interrupt */
    case RT_DEVICE_CTRL_CLR_INT:
        UART_ITConfig(uart->config->Instance, uart_it, DISABLE);
        break;

    /* enable interrupt */
    case RT_DEVICE_CTRL_SET_INT:
        UART_ITConfig(uart->config->Instance, uart_it, ENABLE);
        break;

    case RT_DEVICE_CTRL_CLOSE:
        UART_DeInit(uart->config->Instance);
        break;

    }
    return RT_EOK;
}

static int mh_putc(struct rt_serial_device *serial, char c)
{
    struct mh_uart *uart;
    RT_ASSERT(serial != RT_NULL);

    uart = rt_container_of(serial, struct mh_uart, serial);

    while(!UART_IsTXEmpty(uart->config->Instance));
    UART_SendData(uart->config->Instance, (uint8_t)c);
    return 1;
}

static int mh_getc(struct rt_serial_device *serial)
{
    int ch;
    struct mh_uart *uart;
    RT_ASSERT(serial != RT_NULL);
    uart = rt_container_of(serial, struct mh_uart, serial);

    ch = -1;
    if ((UART_GetITIdentity(uart->config->Instance) & 0x0f) == UART_IT_ID_RX_RECVD)
    {
        ch = UART_ReceiveData(uart->config->Instance);
    }

    return ch;
}

/**
 * Uart common interrupt process. This need add to uart ISR.
 *
 * @param serial serial device
 */
static void uart_isr(struct rt_serial_device *serial)
{
    rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
}

#if defined(BSP_USING_UART0)
void UART0_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    uart_isr(&(uart_obj[UART0_INDEX].serial));

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* BSP_USING_UART1 */

#if defined(BSP_USING_UART1)
void UART1_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    uart_isr(&(uart_obj[UART1_INDEX].serial));

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* BSP_USING_UART2 */

#if defined(BSP_USING_UART2)
void UART2_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    uart_isr(&(uart_obj[UART2_INDEX].serial));

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* BSP_USING_UART3*/

#if defined(BSP_USING_UART3)
void UART3_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    uart_isr(&(uart_obj[UART3_INDEX].serial));

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* BSP_USING_UART4*/

static const struct rt_uart_ops mh_uart_ops =
{
    .configure = mh_configure,
    .control = mh_control,
    .putc = mh_putc,
    .getc = mh_getc,
    .dma_transmit = RT_NULL
};

int rt_hw_usart_init(void)
{
    rt_size_t obj_num = sizeof(uart_obj) / sizeof(struct mh_uart);
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    rt_err_t result = 0;

    for (int i = 0; i < obj_num; i++)
    {
        /* init UART object */
        uart_obj[i].config = &uart_config[i];
        uart_obj[i].serial.ops    = &mh_uart_ops;
        uart_obj[i].serial.config = config;

        //uart_obj[0].serial.config.baud_rate = 230400;

        /* register UART device */
        result = rt_hw_serial_register(&uart_obj[i].serial, uart_obj[i].config->name,
                                       RT_DEVICE_FLAG_RDWR
                                       | RT_DEVICE_FLAG_INT_RX
                                       | RT_DEVICE_FLAG_INT_TX
                                       , NULL);
        RT_ASSERT(result == RT_EOK);
    }

    return result;
}

#endif /* RT_USING_SERIAL */
