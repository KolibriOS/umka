umka_init shell
disk_add ../img/xfs_v5_files_s05k_b4k_n8k.qcow2 hd0 -c 524288
# zero length
read70 /hd0/1/no_hole 0 0 -b
read70 /hd0/1/no_hole 1 0 -b
read70 /hd0/1/no_hole 0xffe 0 -b
read70 /hd0/1/no_hole 0xfff 0 -b
read70 /hd0/1/no_hole 0x1000 0 -b
read70 /hd0/1/no_hole 0x1001 0 -b
read70 /hd0/1/no_hole 0x1ffe 0 -b
read70 /hd0/1/no_hole 0x1fff 0 -b
read70 /hd0/1/no_hole 0x2000 0 -b
read70 /hd0/1/no_hole 0xfffe 0 -b
read70 /hd0/1/no_hole 0xffff 0 -b
read70 /hd0/1/no_hole 0x10000 0 -b
read70 /hd0/1/no_hole 0x10001 0 -b
read70 /hd0/1/no_hole 0x1ffff 0 -b
read70 /hd0/1/no_hole 0x10000000 0 -b
read70 /hd0/1/no_hole 0x1000ffff 0 -b
read70 /hd0/1/no_hole 0xffff0000 0 -b
read70 /hd0/1/no_hole 0xffff0001 0 -b
read70 /hd0/1/no_hole 0xffffffff 0 -b
read70 /hd0/1/no_hole 0x100000000 0 -b
read70 /hd0/1/no_hole 0x100000001 0 -b
read70 /hd0/1/no_hole 0x1ffffffff 0 -b
read70 /hd0/1/no_hole 0xffffffff00000000 0 -b
read70 /hd0/1/no_hole 0xffffffffffffffff 0 -b
# one-byte length
read70 /hd0/1/no_hole 0 1 -b
read70 /hd0/1/no_hole 1 1 -b
read70 /hd0/1/no_hole 0xffe 1 -b
read70 /hd0/1/no_hole 0xfff 1 -b
read70 /hd0/1/no_hole 0x1000 1 -b
read70 /hd0/1/no_hole 0x1001 1 -b
read70 /hd0/1/no_hole 0x1ffe 1 -b
read70 /hd0/1/no_hole 0x1fff 1 -b
read70 /hd0/1/no_hole 0x2000 1 -b
read70 /hd0/1/no_hole 0xfffe 1 -b
read70 /hd0/1/no_hole 0xffff 1 -b
read70 /hd0/1/no_hole 0x10000 1 -b
read70 /hd0/1/no_hole 0x10001 1 -b
read70 /hd0/1/no_hole 0x1ffff 1 -b
read70 /hd0/1/no_hole 0x10000000 1 -b
read70 /hd0/1/no_hole 0x1000ffff 1 -b
read70 /hd0/1/no_hole 0xffff0000 1 -b
read70 /hd0/1/no_hole 0xffff0001 1 -b
read70 /hd0/1/no_hole 0xffffffff 1 -b
read70 /hd0/1/no_hole 0x100000000 1 -b
read70 /hd0/1/no_hole 0x100000001 1 -b
read70 /hd0/1/no_hole 0x1ffffffff 1 -b
read70 /hd0/1/no_hole 0xffffffff00000000 1 -b
read70 /hd0/1/no_hole 0xffffffffffffffff 1 -b
# fixed-size block, different begin/end positions
read70 /hd0/1/no_hole 0 11 -b
read70 /hd0/1/no_hole 1 11 -b
read70 /hd0/1/no_hole 0xfff4 11 -b
read70 /hd0/1/no_hole 0xfff5 11 -b
read70 /hd0/1/no_hole 0xfff6 11 -b
read70 /hd0/1/no_hole 0xfffe 11 -b
read70 /hd0/1/no_hole 0xffff 11 -b
read70 /hd0/1/no_hole 0x10000 11 -b
read70 /hd0/1/no_hole 0x10001 11 -b
read70 /hd0/1/no_hole 0x10000000 11 -b
read70 /hd0/1/no_hole 0x10000001 11 -b
read70 /hd0/1/no_hole 0x1000ffff 11 -b
read70 /hd0/1/no_hole 0xffff0000 11 -b
read70 /hd0/1/no_hole 0xffffffff 11 -b
read70 /hd0/1/no_hole 0x100000000 11 -b
read70 /hd0/1/no_hole 0x100000001 11 -b
read70 /hd0/1/no_hole 0x1ffffffff 11 -b
read70 /hd0/1/no_hole 0xffffffff00000000 11 -b
read70 /hd0/1/no_hole 0xffffffffffffffff 11 -b

# btree
read70 /hd0/1/btree_l1_no_hole 0x80000 11 -b
