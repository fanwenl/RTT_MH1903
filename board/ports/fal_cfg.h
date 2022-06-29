/**
 * Copyright (c) 2022, Wenlong Fan (fanwenl@foxmail.com). All rights reserved.
 *
 * Change logs:
 * Date          Author          Notes
 * 2022-06-17    fanwenl         The first version.
 *
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtthread.h>
//#include <board.h>

#define MH_FLASH_GRANULARITY   (4 * 1024)

#define FLASH_PARAM_OFFSET_ADRESS  0
#define FLASH_PARAM_SIZE  (1 * MH_FLASH_GRANULARITY)  // 4K

#define BOOT_OFFSET_ADRESS  (FLASH_PARAM_OFFSET_ADRESS + FLASH_PARAM_SIZE)
#define BOOT_SIZE  (22 * MH_FLASH_GRANULARITY)  // 88K

#define PARAMETER_OFFSET_ADRESS  (BOOT_OFFSET_ADRESS + BOOT_SIZE)
#define PARAMETER_SIZE  (1 * MH_FLASH_GRANULARITY)

#define APP_OFFSET_ADRESS  (PARAMETER_OFFSET_ADRESS + PARAMETER_SIZE)
#define APP_SIZE  (360 * MH_FLASH_GRANULARITY)

#define FONT_OFFSET_ADRESS  (APP_OFFSET_ADRESS + APP_SIZE)
#define FONT_SIZE  (128 * MH_FLASH_GRANULARITY)

#define OTA_OFFSET_ADRESS  (FONT_OFFSET_ADRESS + FONT_SIZE)
#define OTA_SIZE  (360 * MH_FLASH_GRANULARITY)

#define FS_OFFSET_ADRESS  (OTA_OFFSET_ADRESS + OTA_SIZE)
#define FS_SIZE (152 * MH_FLASH_GRANULARITY)

extern const struct fal_flash_dev mh_onchip_flash;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &mh_onchip_flash,                                                 \
}
/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG

/* partition table */
#define FAL_PART_TABLE                                                                 \
{                                                                                      \
    {FAL_PART_MAGIC_WROD, "header",         "onchip_flash", FLASH_PARAM_OFFSET_ADRESS, FLASH_PARAM_SIZE , 0}, \
    {FAL_PART_MAGIC_WROD, "boot",           "onchip_flash", BOOT_OFFSET_ADRESS, BOOT_SIZE , 0}, \
    {FAL_PART_MAGIC_WROD, "param",          "onchip_flash", PARAMETER_OFFSET_ADRESS, PARAMETER_SIZE , 0}, \
    {FAL_PART_MAGIC_WROD, "app",            "onchip_flash", APP_OFFSET_ADRESS, APP_SIZE , 0}, \
    {FAL_PART_MAGIC_WROD, "font",           "onchip_flash", FONT_OFFSET_ADRESS, FONT_SIZE , 0}, \
    {FAL_PART_MAGIC_WROD, "download",       "onchip_flash", OTA_OFFSET_ADRESS, OTA_SIZE, 0}, \
    {FAL_PART_MAGIC_WROD, "fs",             "onchip_flash", FS_OFFSET_ADRESS, FS_SIZE, 0}, \
}

#endif /* FAL_PART_HAS_TABLE_CFG */
#endif /* _FAL_CFG_H_ */
