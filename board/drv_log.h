/**
 * Copyright (c) 2022, Wenlong Fan (fanwenl@foxmail.com). All rights reserved.
 *
 * Change logs:
 * Date          Author          Notes
 * 2022-06-17    fanwenl         The first version.
 *
 */

#ifndef LOG_TAG
#define DBG_TAG               "drv"
#else
#define DBG_TAG               LOG_TAG
#endif /* LOG_TAG */

#ifdef DRV_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif /* DRV_DEBUG */

#include <rtdbg.h>
