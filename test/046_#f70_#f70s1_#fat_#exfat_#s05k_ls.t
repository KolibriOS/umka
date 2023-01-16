umka_init shell
disk_add ../img/exfat_s05k_c16k_b16k.qcow2 hd0 -c 524288

ls70 /hd0/1/dir_0 -f 0 -c 0
ls70 /hd0/1/dir_0 -f 0 -c 1
ls70 /hd0/1/dir_0 -f 0 -c 2
ls70 /hd0/1/dir_0 -f 0 -c 0xffffffff
ls70 /hd0/1/dir_0 -f 1 -c 0
ls70 /hd0/1/dir_0 -f 1 -c 1
ls70 /hd0/1/dir_0 -f 1 -c 2
ls70 /hd0/1/dir_0 -f 1 -c 0xffffffff

ls70 /hd0/1/dir_1 -f 0 -c 0
ls70 /hd0/1/dir_1 -f 0 -c 1
ls70 /hd0/1/dir_1 -f 0 -c 2
ls70 /hd0/1/dir_1 -f 0 -c 0xffffffff
ls70 /hd0/1/dir_1 -f 1 -c 0
ls70 /hd0/1/dir_1 -f 1 -c 1
ls70 /hd0/1/dir_1 -f 1 -c 2
ls70 /hd0/1/dir_1 -f 1 -c 0xffffffff

ls70 /hd0/1/dir_1000
ls70 /hd0/1/dir_10000
ls70 /hd0/1/dir_100000

disk_del hd0
