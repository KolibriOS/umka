#   SPDX-License-Identifier: GPL-2.0-or-later
#
#   UMKa - User-Mode KolibriOS developer tools
#
#   Copyright (C) 2021  Magomed Kostoev <mkostoevr@yandex.ru>

import os
import sys
import shutil

def handle_file(path):
    path_without_root_folder_name = path.replace(f"{folder}/", "")
    virtual_path = f"{disk}/{path_without_root_folder_name}"
    print(f"read70 {virtual_path} 0 16388096 -h")

def handle_dir(path):
    if path == folder:
        print(f"ls80 {disk}/")
    else:
        path_without_root_folder_name = path.replace(f"{folder}/", "")
        virtual_path = f"{disk}/{path_without_root_folder_name}"
        print(f"ls80 {virtual_path}/")
    items = [f"{path}/{f}" for f in os.listdir(path)]
    items.sort()
    for item in items:
        if os.path.isdir(item):
            handle_dir(item)
        else:
            handle_file(item)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <folder_name> [<img_name> [<virtual_disk_name>]]")
        exit()

    if len(sys.argv) > 2:
        img_name = sys.argv[2]
    else:
        img_name = "fat32_test0.img"

    if len(sys.argv) > 3:
        virtual_disk_name = sys.argv[3]
    else:
        virtual_disk_name = "hd0"

    print("umka_init")
    print(f"disk_add ../img/{img_name} {virtual_disk_name} -c 0")
    disk = f"/{virtual_disk_name}/1"
    folder = sys.argv[1]
    handle_dir(folder)
    print(f"disk_del {virtual_disk_name}")

