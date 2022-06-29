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

#ifdef BSP_USING_ON_CHIP_FLASH
#include "drv_flash.h"
#include "mhscpu_conf.h"
#include "mhscpu_qspi.h"
#include "mhscpu_cache.h"

#if defined(PKG_USING_FAL)
#include "fal.h"
#endif

// #define DRV_DEBUG
#define LOG_TAG                "drv.flash"
#include <drv_log.h>

/**
 * Read data from flash.
 * @note This operation's units is word.
 *
 * @param addr flash address
 * @param buf buffer to store read data
 * @param size read bytes size
 *
 * @return result
 */
int mh_flash_read(rt_uint32_t addr, rt_uint8_t *buf, size_t size)
{
    size_t i;

    if ((addr + size) > MH_FLASH_END_ADDRESS)
    {
        LOG_E("read outrange flash size! addr is (0x%p)", (void*)(addr + size));
        return -1;
    }

    for (i = 0; i < size; i++, buf++, addr++)
    {
        *buf = *(rt_uint8_t *) addr;
    }

    return size;
}

static rt_int8_t data_check(rt_uint8_t *src, rt_uint8_t *dst, rt_uint32_t size)
{
    if (memcmp(src, dst, size))
    {
        return -1;
    }
    return 0;
}

static int erase_check(rt_uint32_t addr, rt_uint32_t pagNum)
{
    rt_uint32_t i = 0;
    rt_uint8_t erase_Buf[X25Q_PAGE_SIZE];

    memset(erase_Buf, 0xFF, X25Q_PAGE_SIZE);
    CACHE_CleanAll(CACHE);
    for (i = 0; i < pagNum; i++)
    {
        if (0 != data_check(erase_Buf, (uint8_t *)(addr + i * X25Q_PAGE_SIZE), sizeof(erase_Buf)))
        {
            return -1;
        }
    }

    return 0;
}

/**
 * Write data to flash.
 * @note This operation's units is word.
 * @note This operation must after erase. @see flash_erase.
 *
 * @param addr flash address
 * @param buf the write data buffer
 * @param size write bytes size
 *
 * @return result
 */
int mh_flash_write(rt_uint32_t addr, const rt_uint8_t *buf, size_t size)
{
    rt_err_t result      = RT_EOK;
    rt_uint32_t end_addr = addr + size;
    rt_uint32_t written_size = 0;

    rt_uint32_t num_pages = 0;
    rt_uint32_t num_rest = 0;
    rt_uint32_t addr_mod = 0;
    rt_uint32_t first_pages = 0;
    rt_uint8_t write_buf[X25Q_PAGE_SIZE];

    if (addr % 4 != 0)
    {
        LOG_E("write addr must be 4-byte alignment");
        return -RT_EINVAL;
    }

    if ((end_addr) > MH_FLASH_END_ADDRESS)
    {
        LOG_E("write outrange flash size! addr is (0x%p)", (void*)(addr + size));
        return -RT_EINVAL;
    }

    if (size < 1)
    {
        return -RT_EINVAL;
    }

    /* 按照 page 对齐写入 */
    while (written_size < size)
    {
        addr_mod = (addr + written_size) % X25Q_PAGE_SIZE;
        num_pages = (size - written_size) / X25Q_PAGE_SIZE;
        num_rest = (size - written_size) % X25Q_PAGE_SIZE;

        LOG_D("DEBUG: flash program page addr (0x%p) addr_mod(%u) pages (%u) rest (%u)", (void*)(addr + written_size),
        addr_mod, num_pages, num_rest);

        if(addr_mod == 0)
        {
            if(num_pages > 0)
            {
                memcpy(write_buf, (rt_uint8_t *)(buf + written_size), X25Q_PAGE_SIZE);
                if(FLASH_ProgramPage(addr + written_size, X25Q_PAGE_SIZE, (rt_uint8_t *)(write_buf)) == QSPI_STATUS_OK)
                {
                    CACHE_CleanAll(CACHE);
                    LOG_D("DEBUG: flash program page addr is (0x%p) size is(%u) buf size is (0x%p)", (void*)(addr + written_size),
                        X25Q_PAGE_SIZE, (void*)(buf + written_size));

                    if (data_check((rt_uint8_t *)(write_buf), (rt_uint8_t *)(addr + written_size), X25Q_PAGE_SIZE) != 0)
                    {
                        LOG_E("DEBUG: flash program read data check error addr (0x%p)", (void *)(addr + written_size));
                        result = -RT_ERROR;
                        break;
                    }
                }
                else
                {
                    LOG_E("DEBUG: flash program page error addr is (0x%p) size is(0x%u) buf size is (0x%p)", (void*)(addr + written_size),
                          X25Q_PAGE_SIZE, (void*)(buf + written_size));
                    result = -RT_ERROR;
                    break;
                }
                written_size += X25Q_PAGE_SIZE;
            }
            else
            {
                memcpy(write_buf, (rt_uint8_t *)(buf + written_size), num_rest);
                if(FLASH_ProgramPage(addr + written_size, num_rest, (rt_uint8_t *)(write_buf)) == QSPI_STATUS_OK)
                {
                    CACHE_CleanAll(CACHE);
                    if (data_check((rt_uint8_t *)(write_buf), (rt_uint8_t *)(addr + written_size), num_rest) != 0)
                    {
                        LOG_E("DEBUG: flash program read data check error addr (0x%p)", (void *)(addr + written_size));
                        result = -RT_ERROR;
                        break;
                    }
                }
                else
                {
                    LOG_E("DEBUG1: flash program page error addr is (0x%p) size is(0x%u) buf size is (0x%p)", (void*)(addr + written_size),
                    X25Q_PAGE_SIZE, (void*)(buf + written_size));
                    result = -RT_ERROR;
                    break;
                }
                written_size += num_rest;
            }
        }
        else
        {
            /* 只有首次写入的时候会走这儿 */
            first_pages = X25Q_PAGE_SIZE - addr_mod;
            memcpy(write_buf, (rt_uint8_t *)(buf + written_size), first_pages);
            if(FLASH_ProgramPage(addr + written_size, first_pages, (rt_uint8_t *)(write_buf)) == QSPI_STATUS_OK)
            {
                CACHE_CleanAll(CACHE);
                if (data_check((rt_uint8_t *)(write_buf), (rt_uint8_t *)(addr + written_size), first_pages) != 0)
                {
                    LOG_E("DEBUG: flash program read data check error addr (0x%p)", (void *)(addr + written_size));
                    result = -RT_ERROR;
                    break;
                }
            }
            else
            {
                LOG_E("DEBUG: flash program page error addr is (0x%p) size is(0x%u) buf size is (0x%p)", (void*)(addr + written_size),
                    X25Q_PAGE_SIZE, (void*)(buf + written_size));

                result = -RT_ERROR;
                break;
            }
            written_size += first_pages;
        }
    }

    if (result != RT_EOK)
    {
        return result;
    }

    return size;
}

/**
 * Erase data on flash.
 * @note This operation is irreversible.
 * @note This operation's units is different which on many chips.
 *
 * @param addr flash address
 * @param size erase bytes size
 *
 * @return result
 */
int mh_flash_erase(rt_uint32_t addr, size_t size)
{
    rt_err_t result = RT_EOK;
    size_t i = 0;
    rt_uint32_t sector = 0;
    rt_uint32_t sector_num = 0;

    if ((addr + size) > MH_FLASH_END_ADDRESS)
    {
        LOG_E("ERROR: erase outrange flash size! addr is (0x%p)", (void*)(addr + size));
        return -RT_EINVAL;
    }

    if (size < 1)
    {
        return -RT_EINVAL;
    }

    sector = RT_ALIGN_DOWN(addr, MH_FLASH_GRANULARITY);
    sector_num = RT_ALIGN(size, MH_FLASH_GRANULARITY) / (MH_FLASH_GRANULARITY);
    for (i = 0; i < sector_num; i++)
    {
        LOG_D("DEBUG: erase flash addr is (0x%x) sector is (0x%x)", addr, sector);
        if (FLASH_EraseSector(sector) != QSPI_STATUS_OK)
        {
            LOG_E("DEBUG: erase flash addr is (0x%x) sector is (0x%x) faild.", addr, sector);
            result = -RT_ERROR;
            goto __exit;
        }
        else
        {
            if(erase_check(sector, 16) != 0)
            {
                LOG_E("DEBUG: erase flash addr is (0x%x) sector is (0x%x) check faild.", addr, sector);
                result = -RT_ERROR;
                goto __exit;
            }
        }
        sector += MH_FLASH_GRANULARITY;
    }

__exit:

    if (result != RT_EOK)
    {
        return result;
    }

    LOG_D("erase done: addr (0x%p), size %d", (void*)addr, size);
    return size;
}

#if defined(PKG_USING_FAL)

static int fal_flash_read(long offset, rt_uint8_t *buf, size_t size);
static int fal_flash_write(long offset, const rt_uint8_t *buf, size_t size);
static int fal_flash_erase(long offset, size_t size);

const struct fal_flash_dev mh_onchip_flash = {
    "onchip_flash",
    MH_FLASH_START_ADRESS,
    MH_FLASH_SIZE,
    MH_FLASH_GRANULARITY,
    {NULL, fal_flash_read, fal_flash_write, fal_flash_erase}
};

static int fal_flash_read(long offset, rt_uint8_t *buf, size_t size)
{
    return mh_flash_read(mh_onchip_flash.addr + offset, buf, size);
}

static int fal_flash_write(long offset, const rt_uint8_t *buf, size_t size)
{
    return mh_flash_write(mh_onchip_flash.addr + offset, buf, size);
}

static int fal_flash_erase(long offset, size_t size)
{
    return mh_flash_erase(mh_onchip_flash.addr + offset, size);
}

#endif
#endif /* BSP_USING_ON_CHIP_FLASH */
