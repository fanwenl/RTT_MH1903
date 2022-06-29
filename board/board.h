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

#ifndef __BOARD_H__
#define __BOARD_H__

#include <rtthread.h>
#include <rthw.h>
#ifdef RT_USING_DEVICE
#include <rtdevice.h>
#endif

#include "drv_gpio.h"
#include "mhscpu_cache.h"
#include "fal_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 注意：Flash 的第一个扇区为Flash 参数区 */
#define MH_FLASH_START_ADRESS     ((uint32_t)0x01000000)
#define MH_FLASH_SIZE             (4096 * 1024)
#define MH_FLASH_END_ADDRESS      ((uint32_t)(MH_FLASH_START_ADRESS + MH_FLASH_SIZE))

#define MH_SRAM_START          ((uint32_t)0x20000000)
#define MH_SRAM_SIZE           (1024 * 1024)
#define MH_SRAM_END            (MH_SRAM_START + MH_SRAM_SIZE)

#if defined(__CC_ARM) || defined(__CLANG_ARM)
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define HEAP_BEGIN      (&Image$$RW_IRAM1$$ZI$$Limit)
#elif __ICCARM__
#pragma section="CSTACK"
#define HEAP_BEGIN      (__segment_end("CSTACK"))
#else
extern int __bss_end;
#define HEAP_BEGIN      (&__bss_end)
#endif

#define HEAP_END        MH_SRAM_END

#ifdef __cplusplus
}
#endif

#endif
