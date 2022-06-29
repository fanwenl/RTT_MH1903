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

#ifndef __DRV_FLASH_H__
#define __DRV_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

int mh_flash_read(rt_uint32_t addr, rt_uint8_t *buf, size_t size);
int mh_flash_write(rt_uint32_t addr, const rt_uint8_t *buf, size_t size);
int mh_flash_erase(rt_uint32_t addr, size_t size);

#ifdef __cplusplus
}
#endif

#endif  /* __DRV_FLASH_H__ */
