umka_init
disk_add ../img/exfat_s05k_c16k_b16k.img hd0 -c 0

ls70 /hd0/1/dir_empty -f 0 -c 0
ls70 /hd0/1/dir_empty -f 0 -c 1
ls70 /hd0/1/dir_empty -f 0 -c 2
ls70 /hd0/1/dir_empty -f 0 -c 0xffffffff
ls70 /hd0/1/dir_empty -f 1 -c 0
ls70 /hd0/1/dir_empty -f 1 -c 1
ls70 /hd0/1/dir_empty -f 1 -c 2
ls70 /hd0/1/dir_empty -f 1 -c 0xffffffff
ls70 /hd0/1/dir_one -f 0 -c 0
ls70 /hd0/1/dir_one -f 0 -c 1
ls70 /hd0/1/dir_one -f 0 -c 2
ls70 /hd0/1/dir_one -f 0 -c 0xffffffff
ls70 /hd0/1/dir_one -f 1 -c 0
ls70 /hd0/1/dir_one -f 1 -c 1
ls70 /hd0/1/dir_one -f 1 -c 2
ls70 /hd0/1/dir_one -f 1 -c 0xffffffff
disk_del hd0
