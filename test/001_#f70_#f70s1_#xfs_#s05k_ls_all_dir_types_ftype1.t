umka_boot
disk_add ../img/xfs_v4_ftype1_s05k_b2k_n8k.qcow2 hd0 -c 0
disk_add ../img/xfs_short_dir_i8.qcow2 hd1 -c 0

ls70 /hd0/1/sf_empty
ls70 /hd0/1/sf
#ls70 /hd0/1/sf -c 0
ls70 /hd0/1/sf -f 1 -c 1
ls70 /hd0/1/sf -f 2 -c 1
ls70 /hd0/1/sf -f 3 -c 1
ls70 /hd0/1/sf -f 3 -c 100
ls70 /hd0/1/sf -f 200 -c 1

ls70 /hd1/1/sf
ls70 /hd0/1/sf -f 1 -c 1
ls70 /hd0/1/sf -f 2 -c 1
ls70 /hd0/1/sf -f 3 -c 1
ls70 /hd0/1/sf -f 3 -c 100
ls70 /hd0/1/sf -f 200 -c 1
ls70 /hd1/1/sf/d0000000002_xx

ls70 /hd0/1/block
ls70 /hd0/1/block -f 5 -c 1
ls70 /hd0/1/block -f 5 -c 1000
ls70 /hd0/1/block -f 500 -c 1

ls70 /hd0/1/leaf
ls70 /hd0/1/leaf -f 1 -c 1

ls70 /hd0/1/node
ls70 /hd0/1/node -f 2 -c 2

ls70 /hd0/1/btree_leaf
ls70 /hd0/1/btree_leaf -f 3 -c 3
ls70 /hd0/1/btree_leaf_free

disk_del hd0
