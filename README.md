# MH1903X 芯片 BSP 说明

## 简介

本文档为兆讯 MH1903X 芯片 BSP (板级支持包) 说明。

主要内容如下：

- MH1903X 芯片介绍
- BSP 快速上手

通过阅读快速上手章节开发者可以快速地上手该 BSP，将 RT-Thread 运行在该芯片上。

## MH1903X 芯片介绍

MH1903S 芯片使用SC300 安全核处理器，最高204MHz 主频，充分利用其卓越的架构特性、高性能，在提供高性能的同时，还提供安全、节能的解决方案。

芯片内置硬件安全加密模块，支持多种加密安全算法，包括DES、TDES、AES、RSA、ECC、SHA 等主流加密算法。芯片硬件还支持多种攻击检测功能，符合金融安全设备标准。

芯片内部包含安BOOT 程序，支持下载、启动时对固件签名校验。芯片内建640KBSRAM ,同时片内还集成了丰富的外设资源：Smartcard、磁条卡解码、安全键盘、LCDI、DCMI等，所有外设驱动软件兼容目前主流安全芯片软件接口并符合ARM CMSIS 规范，用户可在现有方案基础上进行快速开发和移植。

## 外设支持


本 BSP 目前对外设的支持情况如下：

| **片上外设**      | **支持情况** | **备注**                              |
| :----------------- | :----------: | :------------------------------------- |
| GPIO              |     支持     |  |
| UART              |     支持     | UART0/1/2/3                             |
| RTC               |     支持     | 支持外部晶振和内部低速时钟 |
| WDT               |     支持     |                                       |
| PWM               |     支持     |                                       |
| FLASH | 支持 | 已适配 [FAL](https://github.com/RT-Thread-packages/fal) |


## 使用说明

本 BSP 为开发者提供 MDK5工程。下面以 MDK5 开发环境为例，介绍如何将系统运行起来。

## 依赖包说明

本项目依赖如下软件包:

- fal
- fastlz
- quicklz
- ota_downloader
- tinycrypt

### 编译下载

#### KEIL 编译

双击项目根目录下的 project.uvprojx 文件，打开 MDK5 工程，可编译 app 固件。

双击 bootloader\mdk 目录下的 project.uvprojx 文件，打开 MDK5 工程，可编译 bootLoader 固件。

#### 脚本编译

使用项目根目录下的 build_app.bat 和 rebuild_app.bat 脚本可编译 app 固件。

使用项目根目录下的 build_boot.bat 和 rebuild_boot.bat 脚本可编译 bootLoader 固件。

使用 release_pack.py 打包编译发布的版本。

### 运行结果

连接开发板对应串口到 PC , 在终端工具里打开相应的串口（115200-8-1-N），复位设备后，可以看到 RT-Thread 的输出信息:


```bash
 ____ _____      _____ ___  _____  _
|  _ \_   _|	|  ___/ _ \ _   _|/ \
| |_) || |_____ | |_  || ||  | | / _ \
|  _ < | |_____ |  _| ||_||  | |/ ___ \
|_| \_\|_|	|_|   \___/  |_/_/   \_\

2016 - 2019 Copyright by Radiation @ warfalcon
Version: 1.0.0 build Jun 16 2022

[I/FAL] RT-Thread Flash Abstraction Layer (V0.5.0) initialize success.
[D/fota] Partition[download] head CRC32 error!
[D/fota] Partition[download] verify failed!
[I/fota] Implement application now.

 \ | /
- RT -     Thread Operating System
 / | \     4.1.0 build Jun 16 2022 11:50:41
 2006 - 2021 Copyright by rt-thread team
[D/FAL] (fal_flash_init:61) Flash device |             onchip_flash | addr: 0x01000000 | len: 0x00400000 | blk_size: 0x00001000 |initialized finish.
[I/FAL] ==================== FAL partition table ====================
[I/FAL] | name     | flash_dev    |   offset   |    length  |
[I/FAL] -------------------------------------------------------------
[I/FAL] | header   | onchip_flash | 0x00000000 | 0x00001000 |
[I/FAL] | boot     | onchip_flash | 0x00001000 | 0x00016000 |
[I/FAL] | param    | onchip_flash | 0x00017000 | 0x00001000 |
[I/FAL] | app      | onchip_flash | 0x00018000 | 0x00168000 |
[I/FAL] | font     | onchip_flash | 0x00180000 | 0x00080000 |
[I/FAL] | download | onchip_flash | 0x00200000 | 0x00168000 |
[I/FAL] | fs       | onchip_flash | 0x00368000 | 0x00098000 |
[I/FAL] =============================================================
[I/FAL] RT-Thread Flash Abstraction Layer (V0.5.0) initialize success.
hello world!
msh />hello world!
hello world!
hello world!
hello world!
```

## 其他说明

- Bootloader 使用的是[rt-fota](https://gitee.com/spunky_973/rt-fota)

- HM1903 资料相对来说比较少，有问题请联系芯片厂家。

## 联系人信息

维护人: [fanwenl](https://github.com/fanwenl)