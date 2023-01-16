umka_init shell
disk_add ../img/xfs_nrext64.qcow2 hd0 -c 524288
stat70 /hd0/1/dir_sf
stat70 /hd0/1/dir_block
stat70 /hd0/1/dir_leaf
stat70 /hd0/1/dir_node
stat70 /hd0/1/dir_btree_l1
stat70 /hd0/1/dir_btree_l2
ls70 /hd0/1/dir_sf
ls70 /hd0/1/dir_block
ls70 /hd0/1/dir_leaf
ls70 /hd0/1/dir_node
ls70 /hd0/1/dir_btree_l1
ls70 /hd0/1/dir_btree_l2 -f 0 -c 10
ls70 /hd0/1/dir_btree_l2 -f 999995 -c 10
ls70 /hd0/1/dir_btree_l2 -f 1000001 -c 10
ls70 /hd0/1/dir_btree_l2 -f 1000002 -c 10
ls70 /hd0/1/dir_btree_l2 -f 1000003 -c 10
disk_del hd0
