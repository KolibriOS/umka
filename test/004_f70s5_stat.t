disk_add ../img/xfs_v4_ftype0_s05k_b2k_n8k.img hd0
stat /hd0/1/sf/.
stat /hd0/1/sf/..
stat /hd0/1/sf///
stat /hd0/1/sf///.
stat /hd0/1/sf///..
stat /hd0/1/sf/../sf
stat /hd0/1/sf/../sf/
#stat /hd0/1/sf///..//sf
#stat /hd0/1/sf///..//sf/

stat /hd0/1/sf/d0000000000_
stat /hd0/1/sf/d0000000001_x
stat /hd0/1/sf/d0000000002_xx
stat /hd0/1/sf/d0000000003_xxx

cd /hd0/1/sf
stat d0000000001_x
stat d0000000003_xxx
