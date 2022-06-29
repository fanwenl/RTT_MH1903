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

#ifdef RT_USING_PIN

// #define DRV_DEBUG
#define LOG_TAG             "drv.gpio"
#include <drv_log.h>

#define PIN_NUM(port, no) (((((port)&0xFu) << 4) | ((no)&0xFu)))
#define PIN_PORT(pin) ((uint8_t)(((pin) >> 4) & 0xFu))
#define PIN_NO(pin) ((uint8_t)((pin)&0xFu))

#define PIN_STPORT(pin) ((GPIO_TypeDef *)(GPIO_BASE + (0x10u * PIN_PORT(pin))))

#define PIN_STPIN(pin) ((uint16_t)(1u << PIN_NO(pin)))

#if defined(GPIOF)
#define __MH_PORT_MAX 6u
#elif defined(GPIOE)
#define __MH_PORT_MAX 5u
#elif defined(GPIOD)
#define __MH_PORT_MAX 4u
#elif defined(GPIOC)
#define __MH_PORT_MAX 3u
#elif defined(GPIOB)
#define __MH_PORT_MAX 2u
#elif defined(GPIOA)
#define __MH_PORT_MAX 1u
#else
#define __MH_PORT_MAX 0u
#error Unsupported MH GPIO peripheral.
#endif

#define PIN_STPORT_MAX __MH_PORT_MAX

static const struct port_irq_map port_irq_map[] =
{
    {EXTI_Line0, EXTI0_IRQn},
    {EXTI_Line1, EXTI1_IRQn},
    {EXTI_Line2, EXTI2_IRQn},
    {EXTI_Line3, EXTI3_IRQn},
    {EXTI_Line4, EXTI4_IRQn},
    {EXTI_Line5, EXTI5_IRQn},
};

static struct rt_pin_irq_hdr pin_irq_hdr_tab[] =
{
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
};


#define ITEM_NUM(items) sizeof(items) / sizeof(items[0])

static rt_base_t mh_pin_get(const char *name)
{
    rt_base_t pin = 0;
    int hw_port_num, hw_pin_num = 0;
    int i, name_len;

    name_len = rt_strlen(name);

    if ((name_len < 4) || (name_len >= 6))
    {
        return -RT_EINVAL;
    }
    if ((name[0] != 'P') || (name[2] != '.'))
    {
        return -RT_EINVAL;
    }

    if ((name[1] >= 'A') && (name[1] <= 'Z'))
    {
        hw_port_num = (int)(name[1] - 'A');
    }
    else
    {
        return -RT_EINVAL;
    }

    for (i = 3; i < name_len; i++)
    {
        hw_pin_num *= 10;
        hw_pin_num += name[i] - '0';
    }

    pin = PIN_NUM(hw_port_num, hw_pin_num);

    return pin;
}

static void mh_pin_write(rt_device_t dev, rt_base_t pin, rt_base_t value)
{
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;

    if (PIN_PORT(pin) < PIN_STPORT_MAX)
    {
        gpio_port = PIN_STPORT(pin);
        gpio_pin = PIN_STPIN(pin);

        LOG_D("WRITE: port %u (0x%p) pin %u V:%u", PIN_PORT(pin), PIN_STPORT(pin), PIN_NO(pin), value);
        if (value)
        {
            if(PIN_PORT(pin) == 0 && PIN_NO(pin) == 10)
            {
                GPIO_PullUpCmd(PIN_STPORT(pin), PIN_STPIN(pin), ENABLE);
            }
            GPIO_SetBits(gpio_port, gpio_pin);

        }
        else
        {
            if(PIN_PORT(pin) == 0 && PIN_NO(pin) == 10)
            {
                GPIO_PullUpCmd(PIN_STPORT(pin), PIN_STPIN(pin), DISABLE);
            }
            GPIO_ResetBits(gpio_port, gpio_pin);
        }
    }
}

static int mh_pin_read(rt_device_t dev, rt_base_t pin)
{
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;
    int value = PIN_LOW;

    if (PIN_PORT(pin) < PIN_STPORT_MAX)
    {
        gpio_port = PIN_STPORT(pin);
        gpio_pin = PIN_STPIN(pin);
        value = GPIO_ReadInputDataBit(gpio_port, gpio_pin);
    }

    return value;
}

static void mh_pin_mode(rt_device_t dev, rt_base_t pin, rt_base_t mode)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    if (PIN_PORT(pin) >= PIN_STPORT_MAX)
    {
        return;
    }

    /* 特殊引脚处理 */
    if(PIN_PORT(pin) == 0 && (PIN_NO(pin) == 8 || PIN_NO(pin) == 9 || PIN_NO(pin) == 10))
    {
        SYSCTRL->PHER_CTRL &= ~BIT(20);
        LOG_D("MODE SET: port %u (0x%p) pin %u", PIN_PORT(pin), PIN_STPORT(pin), PIN_NO(pin));
    }

    /* Configure GPIO_InitStructure */
    GPIO_InitStruct.GPIO_Pin = PIN_STPIN(pin);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Remap = GPIO_Remap_1;

    if (mode == PIN_MODE_OUTPUT)
    {
        /* output setting */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    }
    else if (mode == PIN_MODE_INPUT)
    {
        /* input setting: not pull. */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    }
    else if (mode == PIN_MODE_INPUT_PULLUP)
    {
        /* input setting: pull up. */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
        GPIO_PullUpCmd(PIN_STPORT(pin), PIN_STPIN(pin), ENABLE);
    }
    else if (mode == PIN_MODE_INPUT_PULLDOWN)
    {
        /* input setting: pull down. */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
        GPIO_PullUpCmd(PIN_STPORT(pin), PIN_STPIN(pin), DISABLE);
    }
    else if (mode == PIN_MODE_OUTPUT_OD)
    {
        /* output setting: od. */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
    }
    GPIO_Init(PIN_STPORT(pin), &GPIO_InitStruct);
}

rt_inline const struct port_irq_map *get_port_irq_map(uint32_t pinbit)
{
    rt_int32_t mapindex = PIN_PORT(pinbit);
    if (mapindex < 0 || mapindex >= ITEM_NUM(port_irq_map))
    {
        return RT_NULL;
    }
    return &port_irq_map[mapindex];
};

static rt_err_t mh_pin_attach_irq(struct rt_device *device, rt_int32_t pin,
                                  rt_uint32_t mode, void (*hdr)(void *args), void *args)
{
    rt_base_t level;
    rt_int32_t irqindex = -1;

    if (PIN_PORT(pin) >= PIN_STPORT_MAX)
    {
        return -RT_ENOSYS;
    }

    irqindex = PIN_PORT(pin);
    if (irqindex < 0 || irqindex >= ITEM_NUM(port_irq_map))
    {
        return RT_ENOSYS;
    }

    level = rt_hw_interrupt_disable();
    if (pin_irq_hdr_tab[irqindex].pin == pin &&
        pin_irq_hdr_tab[irqindex].hdr == hdr &&
        pin_irq_hdr_tab[irqindex].mode == mode &&
        pin_irq_hdr_tab[irqindex].args == args)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }
    if (pin_irq_hdr_tab[irqindex].pin != -1)
    {
        rt_hw_interrupt_enable(level);
        return RT_EBUSY;
    }
    pin_irq_hdr_tab[irqindex].pin = pin;
    pin_irq_hdr_tab[irqindex].hdr = hdr;
    pin_irq_hdr_tab[irqindex].mode = mode;
    pin_irq_hdr_tab[irqindex].args = args;
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

static rt_err_t mh_pin_dettach_irq(struct rt_device *device, rt_int32_t pin)
{
    rt_base_t level;
    rt_int32_t irqindex = -1;

    if (PIN_PORT(pin) >= PIN_STPORT_MAX)
    {
        return -RT_ENOSYS;
    }

    irqindex = PIN_PORT(pin);
    if (irqindex < 0 || irqindex >= ITEM_NUM(port_irq_map))
    {
        return RT_ENOSYS;
    }

    level = rt_hw_interrupt_disable();
    if (pin_irq_hdr_tab[irqindex].pin == -1)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }
    pin_irq_hdr_tab[irqindex].pin = -1;
    pin_irq_hdr_tab[irqindex].hdr = RT_NULL;
    pin_irq_hdr_tab[irqindex].mode = 0;
    pin_irq_hdr_tab[irqindex].args = RT_NULL;
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

static rt_err_t mh_pin_irq_enable(struct rt_device *device, rt_base_t pin,
                                  rt_uint32_t enabled)
{
    const struct port_irq_map *irqmap;
    rt_base_t level;
    rt_int32_t irqindex = -1;
	NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStruct;

    if (PIN_PORT(pin) >= PIN_STPORT_MAX)
    {
        return -RT_ENOSYS;
    }

    if (enabled == PIN_IRQ_ENABLE)
    {
        irqindex = PIN_PORT(pin);
        if (irqindex < 0 || irqindex >= ITEM_NUM(port_irq_map))
        {
            return RT_ENOSYS;
        }

        level = rt_hw_interrupt_disable();

        if (pin_irq_hdr_tab[irqindex].pin == -1)
        {
            rt_hw_interrupt_enable(level);
            return RT_ENOSYS;
        }

        irqmap = &port_irq_map[irqindex];
        /* Configure GPIO_InitStructure */
        GPIO_PinRemapConfig(PIN_STPORT(pin), PIN_STPIN(pin), GPIO_Remap_1);
        switch (pin_irq_hdr_tab[irqindex].mode)
        {
        case PIN_IRQ_MODE_RISING:
            EXTI_LineConfig(irqmap->line, PIN_STPIN(pin), EXTI_Trigger_Rising);
            break;
        case PIN_IRQ_MODE_FALLING:
            EXTI_LineConfig(irqmap->line, PIN_STPIN(pin), EXTI_Trigger_Falling);
            break;
        case PIN_IRQ_MODE_RISING_FALLING:
            EXTI_LineConfig(irqmap->line, PIN_STPIN(pin), EXTI_Trigger_Rising_Falling);
            break;
        }

        NVIC_InitStructure.NVIC_IRQChannel = irqmap->irqno;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        rt_hw_interrupt_enable(level);
    }
    else if (enabled == PIN_IRQ_DISABLE)
    {
        irqmap = get_port_irq_map(pin);
        if (irqmap == RT_NULL)
        {
            return RT_ENOSYS;
        }

        level = rt_hw_interrupt_disable();

        GPIO_InitStruct.GPIO_Pin = PIN_STPIN(pin);
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_InitStruct.GPIO_Remap = GPIO_Remap_1;

        GPIO_Init(PIN_STPORT(pin), &GPIO_InitStruct);

        NVIC_InitStructure.NVIC_IRQChannel = irqmap->irqno;
        NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
        NVIC_Init(&NVIC_InitStructure);

        rt_hw_interrupt_enable(level);
    }
    else
    {
        return -RT_ENOSYS;
    }

    return RT_EOK;
}
const static struct rt_pin_ops _mh_pin_ops =
{
        mh_pin_mode,
        mh_pin_write,
        mh_pin_read,
        mh_pin_attach_irq,
        mh_pin_dettach_irq,
        mh_pin_irq_enable,
        mh_pin_get,
};

rt_inline void pin_irq_hdr(int irqno)
{
    if (pin_irq_hdr_tab[irqno].hdr)
    {
        pin_irq_hdr_tab[irqno].hdr(pin_irq_hdr_tab[irqno].args);
    }
}

void EXTI0_IRQHandler(void)
{
    rt_uint32_t exti_status = 0;

    rt_interrupt_enter();
    exti_status = EXTI_GetITLineStatus(EXTI_Line0);
    if (exti_status)
    {
        pin_irq_hdr(EXTI_Line0);
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
    rt_interrupt_leave();
}

void EXTI1_IRQHandler(void)
{
    rt_uint32_t exti_status = 0;

    rt_interrupt_enter();
    exti_status = EXTI_GetITLineStatus(EXTI_Line1);
    if (exti_status)
    {
        pin_irq_hdr(EXTI_Line1);
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
    rt_interrupt_leave();
}

void EXTI2_IRQHandler(void)
{
    rt_uint32_t exti_status = 0;

    rt_interrupt_enter();
    exti_status = EXTI_GetITLineStatus(EXTI_Line2);
    if (exti_status)
    {
        pin_irq_hdr(EXTI_Line2);
        EXTI_ClearITPendingBit(EXTI_Line2);
    }
    rt_interrupt_leave();
}

void EXTI3_IRQHandler(void)
{
    rt_uint32_t exti_status = 0;

    rt_interrupt_enter();
    exti_status = EXTI_GetITLineStatus(EXTI_Line3);
    if (exti_status)
    {
        pin_irq_hdr(EXTI_Line3);
        EXTI_ClearITPendingBit(EXTI_Line3);
    }
    rt_interrupt_leave();
}

void EXTI4_IRQHandler(void)
{
    rt_uint32_t exti_status = 0;

    rt_interrupt_enter();
    exti_status = EXTI_GetITLineStatus(EXTI_Line4);
    if (exti_status)
    {
        pin_irq_hdr(EXTI_Line4);
        EXTI_ClearITPendingBit(EXTI_Line4);
    }
    rt_interrupt_leave();
}

void EXTI5_IRQHandler(void)
{
    rt_uint32_t exti_status = 0;

    rt_interrupt_enter();
    exti_status = EXTI_GetITLineStatus(EXTI_Line5);
    if (exti_status)
    {
        pin_irq_hdr(EXTI_Line5);
        EXTI_ClearITPendingBit(EXTI_Line5);
    }
    rt_interrupt_leave();
}

int rt_hw_pin_init(void)
{
    SYSCTRL_APBPeriphClockCmd(SYSCTRL_APBPeriph_GPIO, ENABLE);

    return rt_device_pin_register("pin", &_mh_pin_ops, RT_NULL);
}

#endif /* RT_USING_PIN */
