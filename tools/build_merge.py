# -*- coding:utf-8 -*-
#
# Copyright (c) 2022, Wenlong Fan (fanwenl@foxmail.com). All rights reserved.
# All rights reserved.
#
# Change logs:
# Date          Author          Notes
# 2022-06-17    fanwenl         The first version.
import os
import struct
import sys

KEIL_PATH = "C:/keil_v5/UV4/UV4.exe"
BOOT_PRO="/bootloader/mdk/project.uvprojx"
APP_PRO="/project.uvprojx"

BOOT_BIN="/build/bootloader.bin"
APP_BIN="/build/app.bin"
FONT_BIN="/tools/gb2312_font.bin"

IMG_BIN ="/build/img.bin"

# 下面这些偏移地址，必须和固件分区里面的地址保持一致，去除 Flash 参数区。
APP_OFFSET = (0 * 4096)
FONT_OFFSET = (360 * 4096)
IMG_SIZE = (360 + 128) * 4096

sdk_path = os.getcwd()

def merge_bin():
    print('------------------------------------------')
    print('start merge bin file')
    img_file = sdk_path + IMG_BIN
    try:
        f_img = open(img_file, "wb+")
    except IOError:
        print("create packfile error")
        raise
    else:
        f_img.write(struct.pack('<B', 0xFF) * IMG_SIZE)
        f_img.flush()

        # 写入 app 文件
        f_img.seek(APP_OFFSET)
        try:
            bin_file = sdk_path + APP_BIN
            f_bin = open(bin_file, 'rb')
        except IOError:
            print("open {} file error".format(bin_file))
            f_img.close()
            return
        else:
            f_bin.seek(0)
            f_img.write(f_bin.read())
            f_bin.flush()
            f_bin.close()

        # 写入字库文件
        f_img.seek(FONT_OFFSET)
        try:
            bin_file = sdk_path + FONT_BIN
            f_bin = open(bin_file, 'rb')
        except IOError:
            f_bin.close()
            print("open {} file error".format(bin_file))
            f_img.close()
            return
        else:
            f_bin.seek(0)
            f_img.write(f_bin.read())
            f_bin.flush()
            f_bin.close()

        f_img.flush()
        f_img.close()

    boot_size = int(os.path.getsize(sdk_path + BOOT_BIN) / 1024)
    app_size = int(os.path.getsize(sdk_path + APP_BIN) / 1024)
    font_size = int(os.path.getsize(sdk_path+ FONT_BIN) /1024)
    img_size = int(os.path.getsize(sdk_path+ IMG_BIN) /1024)

    print('boot\tapp\tfont\tapp+font')
    print('{} KB\t{} KB\t{} KB\t{} KB'.format(boot_size, app_size, font_size, img_size))


def build_target(module, option):
    print('------------------------------------------')
    if module == 'boot':
        pro_name = BOOT_PRO
        print('start {} bootloader'.format('build' if option == 'b' else 'rebuild'))
    else:
        pro_name = APP_PRO
        print('start {} application'.format('build' if option == 'b' else 'rebuild'))
    # print(sdk_path)
    log_file = sdk_path + "/build/build.log"
    fp = open(log_file,'w+')
    fp.close()

    if option == 'r':
        pro_path = sdk_path + pro_name
        build_cmd = KEIL_PATH + ' -j0 -c ' + pro_path + ' -o ' + log_file
        status = os.system(build_cmd)
        # try:
        #     with open(log_file, "r+") as f:
        #         for line in f:
        #             print(line, end='')
        #         f.flush()
        #         f.close()
        # except:
        #     print("open log file error!")
        if status == 0 or status == 1:
            print('clearn {} success'.format('bootloader' if module == 'boot' else 'application'))
        else:
            print('clearn {} fail more info see {}'.format('bootloader' if module == 'boot' else 'application', log_file))
            sys.exit(0)

    pro_path = sdk_path + pro_name
    build_cmd = KEIL_PATH + ' -j0 -b ' + pro_path + ' -o ' + log_file
    status = os.system(build_cmd)
    # time.sleep(10)
    # try:
    #     with open(log_file, "r+") as f:
    #         for line in f:
    #             print(line, end='')
    #         f.flush()
    #         f.close()
    # except:
    #     print("open log file error!")

    if status == 0 or status == 1:
        if module == 'boot':
            print('{} bootloader success'.format('build' if option == 'b' else 'rebuild'))
        else:
            print('{} application success'.format('build' if option == 'b' else 'rebuild'))
    else:
        if module == 'boot':
            print('{} bootloader fail more info see {}'.format('build' if option == 'b' else 'rebuild', log_file))
        else:
            print('{} application fail more info see {}'.format('build' if option == 'b' else 'rebuild', log_file))
        sys.exit(0)

