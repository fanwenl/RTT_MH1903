#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* Automatically generated file; DO NOT EDIT. */
/* RT-Thread Configuration */

/* RT-Thread Kernel */

#define RT_NAME_MAX 8
#define RT_ALIGN_SIZE 4
#define RT_THREAD_PRIORITY_8
#define RT_THREAD_PRIORITY_MAX 8
#define RT_TICK_PER_SECOND 1000
#define RT_USING_OVERFLOW_CHECK
// #define RT_USING_HOOK
// #define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 1024

/* kservice optimization */

#define RT_KSERVICE_USING_STDLIB
#define RT_KSERVICE_USING_TINY_SIZE
#define RT_DEBUG
// #define RT_DEBUG_COLOR

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
// #define RT_USING_EVENT
// #define RT_USING_MAILBOX
// #define RT_USING_MESSAGEQUEUE
// #define RT_USING_SIGNALS

/* Memory Management */

#define RT_USING_MEMPOOL
#define RT_USING_MEMHEAP
#define RT_MEMHEAP_FAST_MODE
#define RT_USING_MEMHEAP_AS_HEAP
#define RT_USING_MEMHEAP_AUTO_BINDING
#define RT_USING_HEAP

/* Kernel Device Object */

#define RT_USING_DEVICE
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 256
#define RT_CONSOLE_DEVICE_NAME "uart0"
#define RT_VER_NUM 0x40100

/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 2048
#define RT_MAIN_THREAD_PRIORITY 4

/* Command shell */

// #define RT_USING_FINSH
// #define RT_USING_MSH
// #define FINSH_USING_MSH
// #define FINSH_THREAD_NAME "tshell"
// #define FINSH_THREAD_PRIORITY 6
// #define FINSH_THREAD_STACK_SIZE 4096
// #define FINSH_USING_HISTORY
// #define FINSH_HISTORY_LINES 5
// #define FINSH_USING_SYMTAB
// #define FINSH_CMD_SIZE 80
// #define MSH_USING_BUILT_IN_COMMANDS
// #define FINSH_USING_DESCRIPTION
// #define FINSH_ARG_MAX 10

/* Device virtual file system */

/* Device Drivers */

#define RT_USING_DEVICE_IPC
// #define RT_USING_SYSTEM_WORKQUEUE
// #define RT_SYSTEM_WORKQUEUE_STACKSIZE 2048
// #define RT_SYSTEM_WORKQUEUE_PRIORITY 23
#define RT_USING_SERIAL
#define RT_USING_SERIAL_V1
#define RT_SERIAL_RB_BUFSZ 2048
// #define RT_USING_PIN
// #define RT_USING_MTD_NOR
//#define RT_USING_RTC
#define RT_USING_WDT

/* Using USB */


/* POSIX layer and C standard library */

// #define RT_USING_LIBC
//#define RT_LIBC_USING_TIME
//#define RT_LIBC_USING_FILEIO
//#define RT_LIBC_DEFAULT_TIMEZONE 8

/* POSIX (Portable Operating System Interface) layer */


/* Interprocess Communication (IPC) */


/* Socket is in the 'Network' category */

/* Network */

/* Socket abstraction layer */


/* protocol stack implement */


/* Network interface device */


/* light weight TCP/IP stack */


/* AT commands */


/* VBUS(Virtual Software BUS) */


/* Utilities */

// #define RT_USING_RYM
// #define YMODEM_USING_FILE_TRANSFER
// #define RT_USING_ULOG
// #define ULOG_OUTPUT_LVL_D
// #define ULOG_OUTPUT_LVL 7
// #define ULOG_USING_ISR_LOG
// #define ULOG_ASSERT_ENABLE
// #define ULOG_LINE_BUF_SIZE 128

/* log format */

// #define ULOG_USING_COLOR
// #define ULOG_OUTPUT_TIME
// #define ULOG_OUTPUT_LEVEL
// #define ULOG_OUTPUT_TAG
// #define ULOG_BACKEND_USING_CONSOLE
// #define ULOG_USING_FILTER

/* RT-Thread online packages */

/* IoT - internet of things */

/* Wi-Fi */

/* Marvell WiFi */


/* Wiced WiFi */

/* IoT Cloud */

//#define PKG_USING_OTA_DOWNLOADER
//#define PKG_USING_HTTP_OTA
//#define PKG_HTTP_OTA_URL "http://xxx/xxx/rtthread.rbl"
//#define PKG_USING_YMODEM_OTA
//#define PKG_USING_OTA_DOWNLOADER_V100

/* security packages */

/* Select Root Certificate */

#define PKG_USING_TINYCRYPT
#define PKG_USING_TINYCRYPT_V100
#define TINY_CRYPT_AES
#define TINY_CRYPT_AES_ROM_TABLES

/* language packages */


/* multimedia packages */

/* LVGL: powerful and easy-to-use embedded GUI library */


/* u8g2: a monochrome graphic library */


/* PainterEngine: A cross-platform graphics application framework written in C language */


/* tools packages */

/* system packages */

/* enhanced kernel services */


/* POSIX extension functions */


/* acceleration: Assembly language or algorithmic acceleration packages */


/* CMSIS: ARM Cortex-M Microcontroller Software Interface Standard */


/* Micrium: Micrium software products porting for RT-Thread */

#define PKG_USING_FAL
#define FAL_DEBUG_CONFIG
#define FAL_DEBUG 0
#define FAL_PART_HAS_TABLE_CFG
#define PKG_USING_FAL_V00500
#define PKG_FAL_VER_NUM 0x00500

/* peripheral libraries and drivers */


/* AI packages */


/* miscellaneous packages */

/* samples: kernel and components samples */


/* entertainment: terminal games and other interesting software packages */

#define PKG_USING_FASTLZ
#define PKG_USING_FASTLZ_V101
#define PKG_USING_QUICKLZ
#define QLZ_COMPRESSION_LEVEL 3
#define PKG_USING_QUICKLZ_V101

/* Hardware Drivers Config */

/* On-chip Peripheral Drivers */

#define BSP_USING_ON_CHIP_FLASH
#define BSP_USING_GPIO
#define BSP_USING_UART
#define BSP_USING_UART0
//#define BSP_USING_UART1
//#define BSP_USING_UART2
//#define BSP_USING_UART3
//#define BSP_USING_ONCHIP_RTC
//#define BSP_RTC_USING_LSE

/* Onboard Peripheral Drivers */

/* Board extended module Drivers */

#define SOC_MH190X

/* FOTA debug  */
#define RT_FOTA_DEBUG

/* RT-FOTA module define */
#define RT_FOTA_SW_VERSION "1.0.0"

/* Enable Ymodem OTA */
// #define PKG_USING_YMODEM_OTA

/* FOTA application partition name */
#ifndef RT_FOTA_APP_PART_NAME
#define RT_FOTA_APP_PART_NAME   "app"
#endif

/* FOTA download partition name */
#ifndef RT_FOTA_FM_PART_NAME
#define RT_FOTA_FM_PART_NAME    "download"
#endif

/* FOTA default partition name */
#ifndef RT_FOTA_DF_PART_NAME
#define RT_FOTA_DF_PART_NAME    "df_area"
#endif

/* AES256 encryption algorithm option */
#define RT_FOTA_ALGO_AES_IV  	"0123456789ABCDEF"
#define RT_FOTA_ALGO_AES_KEY 	"0123456789ABCDEF0123456789ABCDEF"

#endif
