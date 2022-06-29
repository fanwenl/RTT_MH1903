# -*- coding:utf-8 -*-
#
# Copyright (c) 2022, Wenlong Fan (fanwenl@foxmail.com). All rights reserved.
# All rights reserved.
#
# Change logs:
# Date          Author          Notes
# 2022-06-17    fanwenl         The first version.

import os
import subprocess
import shutil
import re
import time
import datetime
from pytz import timezone

tz_shanghai = timezone('Asia/Shanghai')

#替换文件
def alter(file, target_str, new_str):
    re_str = target_str + "(.+?)$"
    with open(file, "r", encoding="utf-8") as f1, open("%s.bak" % file, "w", encoding="utf-8") as f2:
        for line in f1:
            f2.write(re.sub(r'' + re_str, target_str+new_str, line))
    os.remove(file)
    os.rename("%s.bak" % file, file)


def mkdir(path):
    # 引入模块
    import os

    # 去除首位空格
    path = path.strip()
    # 去除尾部 \ 符号
    path = path.rstrip("\\")

    # 判断路径是否存在
    # 存在     True
    # 不存在   False
    isExists = os.path.exists(path)

    # 判断结果
    if not isExists:
        # 如果不存在则创建目录
        # 创建目录操作函数
        os.makedirs(path)

        print(path+' 创建成功')
        return True
    else:
        # 如果目录存在则不创建，并提示目录已存在
        print(path+' 目录已存在')
        return False


def getTime():
    build_time =  datetime.datetime.now(tz=tz_shanghai).strftime('%Y%m%d_%H%M')
    return build_time

def getFormatTime():
    build_time = datetime.datetime.now(tz=tz_shanghai).strftime('%Y')+"年"+\
              datetime.datetime.now(tz=tz_shanghai).strftime('%m')+"月"+\
              datetime.datetime.now(tz=tz_shanghai).strftime('%d')+"日 "+\
              datetime.datetime.now(tz=tz_shanghai).strftime('%H')+":"+\
              datetime.datetime.now(tz=tz_shanghai).strftime('%M')+":"+\
              datetime.datetime.now(tz=tz_shanghai).strftime('%S')
    return build_time

def copy(src_name, dest_name):
    shutil.copy(src_name, dest_name)

def copytree(src, dst, symlinks=False, ignore=None):
    mkdir(dst)
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            shutil.copytree(s, d, symlinks, ignore)
        else:
            shutil.copy2(s, d)



def find_folder(file_tree, file_head):
    folder_list = []
    max_time = 0
    for item in os.listdir(file_tree):
        s = os.path.join(file_tree, item)
        #d = os.path.join(dst, item)
        if os.path.isdir(s) and s.find(file_head) >= 0:
            # print(s)
            file_time = os.path.getctime(s)
            if file_time > max_time:
                max_time = file_time
                lens = len(folder_list)

            folder_list.append([file_time, s])
            # print(file_time)
    return folder_list[lens][1]



def excuteCommand(com):
    print("cmd in:", com)
    start = time.time()
    ex = subprocess.Popen(com, stdout=subprocess.PIPE, shell=True)
    stdlog = ex.stdout.read().decode('gb2312', 'ignore')
    ex.communicate()

    if ex.returncode != 0:
        # f= open("errlog.log","w+")
        # f.write(stdlog)
        # f.close
        print("存在错误，检查log文件")

    end = time.time()
    print('运行时间'+str( datetime.timedelta(seconds=end-start)))

    # print("cmd out: ", out.decode())
    return ex.returncode, stdlog

if __name__ == "__main__":
    pass
