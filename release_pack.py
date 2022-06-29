# -*- coding:utf-8 -*-
#
# Copyright (c) 2022, Wenlong Fan (fanwenl@foxmail.com). All rights reserved.
# All rights reserved.
#
# Change logs:
# Date          Author          Notes
# 2022-06-17    fanwenl         The first version.
#

import sys
import os
from tools.build_lib import *
from tools.build_merge import *

PACK_TOOLS = "/tools/ota_tool_cli.exe"
FILE_PATH = "build/"
sdk_path = os.getcwd()

# bootloader 版本，修改boot或者驱动后需要修改。
BOOT_VERSION = "1.0.0"

产品类型 = "MH1903S"
版本号 = "0.0.1"

打包日期 = getTime()

fg_boot_version = 'bootloader/src/rtconfig.h'


def modify_file(is_upgrade, is_print):
    boot_suffix = ''
    if is_upgrade:
        UPFolders = "FG_"+产品类型+"_V10"+版本号
        version = '99'+版本号
    else:

        UPFolders = "FG_"+产品类型+"_V"+版本号
        version = 版本号

        # boot 文件名
        boot_suffix = "FG_"+产品类型+"_BOOT_V"+ BOOT_VERSION +"_"+打包日期
        alter(fg_boot_version, "RT_FOTA_SW_VERSION ", '"' + BOOT_VERSION + '"')

    PackFolders = UPFolders + "_"+ 打包日期

    return (version, PackFolders, UPFolders, boot_suffix)


def copy_release_file(is_upgrade, folder, up_folder, file_suffix=None):

    bin_path = "build/"

    if not is_upgrade:
        mkdir("./版本发布/"+folder+"/固件")
        firm_path = "版本发布/"+folder+"/固件/"+ file_suffix
        copy(bin_path + "bootloader.bin", firm_path +".bin")
        copy(bin_path + "bootloader.map", firm_path +".map")
        copy(bin_path + "bootloader.axf", firm_path +".axf")
        file_suffix = folder
        firm_path = "版本发布/"+folder+"/固件/"+ file_suffix
        copy(bin_path + "img.bin", firm_path+".bin")
        copy(bin_path + "app.map", firm_path +".map")
        copy(bin_path + "app.axf", firm_path +".axf")

    mkdir("./版本发布/"+folder+"/升级包/"+ up_folder)
    firm_path = "版本发布/"+folder+"/升级包/"+ up_folder +"/"+ file_suffix
    copy(bin_path + "app.rbl", firm_path +".img")


def upgrade_package(version):
    """
    rt_ota_packaging_tool_cli -f BIN -v VERSION -p PARTNAME [-o OUTFILE] [-c CMPRS_TYPE] [-s CRYPT_TYPE] [-i IV] [-k KEY] [-h]
            -f bin file.
            -v firmware's version.
            -p firmware's target part name.
            -o output rbl file path.(optional)
            -c compress type allow [quicklz|gzip|none](optional)
            -s crypt type allow [aes|none](optional)
            -i iv for aes-256-cbc
            -k key for aes-256-cbc
            -h show this help information
    """
    print('------------------------------------------')
    tools_path = os.getcwd() + PACK_TOOLS
    # build_cmd = tools_path + ' -f ' + FILE_PATH + "app.bin" + ' -v ' + version + ' -p app -c quicklz'
    build_cmd = tools_path + ' -f ' + FILE_PATH + "app.bin" + ' -v ' + version + ' -p app'
    status = os.system(build_cmd)
    if status == 0 or status == 1:
        print('upgrade package success')
    else:
        print('upgrade package success')


def delete_binfile():
    for root, dirs, files in os.walk(sdk_path + "/build", topdown=False):
        for name in files:
            os.remove(os.path.join(root, name))

        for name in dirs:
            os.rmdir(os.path.join(root, name))

def default_package():
    print("\r\n")
    print('build release package.')
    delete_binfile()
    version, f_PackFolders, UPFolders, boot_suffix = modify_file(0, 1)
    build_target('boot', 'r')
    build_target('app', 'r')
    merge_bin()
    upgrade_package(version)
    copy_release_file(0,f_PackFolders, UPFolders, boot_suffix)
    delete_binfile()
    version, u_PackFolders, UPFolders, boot_suffix  = modify_file(1, 1)
    build_target('app', 'r')
    upgrade_package(version)
    copy_release_file(1, f_PackFolders, UPFolders, u_PackFolders)
    modify_file(0, 0)
    print("\r\n")

def help():
    print('\r\n')
    print('Usage: release_pack.py [OPTION] [TRAGET] \r\n')
    print('release_pack Options:')
    print('     -b       :build')
    print('     -r       :rebuild(clean and build)')
    print('     -h       :printf help message\r\n')

    print('release_pack Target:')
    print('     -boot    :build bootloader only')
    print('     -app     :build app only')
    print('     -all     :build boot and app')
    print('     -release :build release package(this target ignore [OPTION] cmd)\r\n')


def main(argv):
    argc = len(argv)
    if argc == 1:
        default_package()
    elif argc == 2:
        if argv[1] == '-h':
            help()
            return
        elif argv[1] == '-release':
            default_package()
        else:
            help()
            return
    elif argc == 3:
        if argv[1] == '-b' and argv[2] == '-boot':
            build_target('boot', 'b')
        elif argv[1] == '-b' and argv[2] == '-app':
            build_target('app', 'b')
        elif argv[1] == '-b' and argv[2] == '-all':
            delete_binfile()
            build_target('boot', 'b')
            build_target('app', 'b')
            merge_bin()
        elif argv[1] == '-r' and argv[2] == '-boot':
            build_target('boot', 'r')
        elif argv[1] == '-r' and argv[2] == '-app':
            build_target('app', 'r')
        elif argv[1] == '-r' and argv[2] == '-all':
            delete_binfile()
            build_target('boot', 'r')
            build_target('app', 'r')
            merge_bin()
        else:
            help()
            return
    else:
        help()
        return

if __name__ == '__main__':
    main(sys.argv)