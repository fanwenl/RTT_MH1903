/**
 * Copyright (c) 2022, Wenlong Fan (fanwenl@foxmail.com). All rights reserved.
 *
 * Change logs:
 * Date          Author          Notes
 * 2022-06-17    fanwenl         The first version.
 *
 */

#include "board.h"

#include <string.h>
#include <stdio.h>

#include "ports/fal_cfg.h"

#include "mhscpu.h"
#include "mhscpu_qspi.h"

#ifdef RT_USING_SERIAL
#include "drv_usart.h"
#endif /* RT_USING_SERIAL */

#define DBG_TAG    "drv_boadr"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

/** System Clock Configuration
*/
void SystemClock_Config(void)
{
    CACHE_InitTypeDef test_aes;

    NVIC_SetPriorityGrouping(NVIC_PriorityGroup_3);

    QSPI_Init(NULL);

    SYSCTRL_SYSCLKSourceSelect(SELECT_EXT12M);

	SYSCTRL_PLLConfig(SYSCTRL_PLL_192MHz);
	SYSCTRL_PLLDivConfig(SYSCTRL_PLL_Div_None);
	SYSCTRL_HCLKConfig(SYSCTRL_HCLK_Div2);
	SYSCTRL_PCLKConfig(SYSCTRL_PCLK_Div2);

    QSPI_SetLatency(0);

	SYSCTRL_AHBPeriphClockCmd(SYSCTRL_AHBPeriph_DMA, ENABLE);
	SYSCTRL_AHBPeriphResetCmd(SYSCTRL_AHBPeriph_DMA, ENABLE);

    /* 下面这些代码没啥用 */
    memset(&test_aes, 0x00, sizeof(test_aes));
    CACHE_CleanAll(CACHE);
    test_aes.aes_enable = DISABLE;
    CACHE_Init(CACHE, &test_aes);
}

#ifdef RT_USING_FINSH
#include <finsh.h>
static void reboot(uint8_t argc, char **argv)
{
    rt_hw_cpu_reset();
}
MSH_CMD_EXPORT(reboot, Reboot System);
#endif /* RT_USING_FINSH */

/* 中断向量重映射，在启动文件中已经实现了 */
#ifndef RT_FOTA_SW_VERSION
// static int ota_app_vtor_reconfig(void)
// {
//     NVIC_SetVectorTable(NVIC_VectTab_FLASH, APP_OFFSET_ADRESS);

//     return 0;
// }

// INIT_BOARD_EXPORT(ota_app_vtor_reconfig);
#endif

/* SysTick configuration */
void rt_hw_systick_init(void)
{
    SYSCTRL_ClocksTypeDef clock;

    SYSCTRL_GetClocksFreq(&clock);
	SysTick_Config(clock.CPU_Frequency / RT_TICK_PER_SECOND);

    NVIC_SetPriority(SysTick_IRQn, 0xFF);
}

/**
 * This is the timer interrupt service routine.
 *
 */
void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
        //HAL_IncTick();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

/**
 * This function will delay for some us.
 *
 * @param us the delay time of us
 */
void rt_hw_us_delay(rt_uint32_t us)
{
    rt_uint32_t ticks;
    rt_uint32_t told, tnow, tcnt = 0;
    rt_uint32_t reload = SysTick->LOAD;

    ticks = us * reload / (1000000 / RT_TICK_PER_SECOND);
    told = SysTick->VAL;
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }
}

/**
 * This function will initial STM32 board.
 */
RT_WEAK void rt_hw_board_init(void)
{
#ifdef BSP_SCB_ENABLE_I_CACHE
    /* Enable I-Cache---------------------------------------------------------*/
    SCB_EnableICache();
#endif

#ifdef BSP_SCB_ENABLE_D_CACHE
    /* Enable D-Cache---------------------------------------------------------*/
    SCB_EnableDCache();
#endif
    /* System clock initialization */
    SystemClock_Config();

    /* systick init */
    rt_hw_systick_init();

    /* Heap initialization */
#if defined(RT_USING_HEAP)
    rt_system_heap_init((void *)HEAP_BEGIN, (void *)HEAP_END);
#endif

    /* Pin driver initialization is open by default */
#ifdef RT_USING_PIN
    rt_hw_pin_init();
#endif

    /* USART driver initialization is open by default */
#ifdef RT_USING_SERIAL
    rt_hw_usart_init();
#endif

    /* Set the shell console output device */
#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

    /* Board underlying hardware initialization */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}
