umka_boot
disk_add ../img/fat12_s05k.qcow2 hd0 -c 524288
ls70 /hd0/1/dir_a
ls70 /hd0/1/dir_b
ls70 /hd0/1/dir_c
ls70 /hd0/1/dir_d
ls70 /hd0/1/dir_e
disk_del hd0
