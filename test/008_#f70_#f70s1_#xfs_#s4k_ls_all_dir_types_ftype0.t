umka_init shell
disk_add ../img/xfs_v4_ftype0_s4k_b4k_n8k.qcow2 hd0 -c 524288
ls70 /hd0/1/sf
ls70 /hd0/1/block
ls70 /hd0/1/leaf
ls70 /hd0/1/node
ls70 /hd0/1/btree_leaf
ls70 /hd0/1/btree_leaf_free
disk_del hd0
