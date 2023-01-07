#!/bin/bash

RANDDIR=../tools/randdir
MKDOUBLEDIRS=../tools/mkdoubledirs
MKFILEPATTERN=../tools/mkfilepattern
DIRTOTEST=../tools/dirtotest.py
MKSAMEHASH=../tools/mksamehash
MKDIRRANGE=../tools/mkdirrange
XFS_MIN_PART_SIZE="300MiB"
XFS_MKFS_OPTS="-q -i maxpct=0"
XFS_MOUNT_OPTS="-t xfs"
FAT_MOUNT_OPTS="umask=111,dmask=000"
QCOW2_OPTS="compat=v3,compression_type=zlib,encryption=off,extended_l2=off,preallocation=off"
NBD_DEV=/dev/nbd0   # FIXME

gpt_large.qcow2 () {
    local img=$FUNCNAME
    qemu-img create -f qcow2 -o $QCOW2_OPTS,cluster_size=2097152 $img 2E > /dev/null
    sudo qemu-nbd -c $NBD_DEV $img
    sudo parted --script --align optimal $NBD_DEV mktable gpt \
        mkpart part0 1MiB 1GiB \
        mkpart part1 1GiB 1TiB \
        mkpart part2 1TiB 1024TiB \
        mkpart part3 1024TiB 1048576TiB
    sudo qemu-nbd -d $NBD_DEV > /dev/null
}

gpt_partitions_s05k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw
    fallocate -l 1GiB $img_raw
    parted --script --align optimal $img_raw mktable gpt \
        mkpart part0 1MiB 2MiB \
        mkpart part1 2MiB 3MiB \
        mkpart part2 4MiB 5MiB \
        mkpart part3 3MiB 4MiB \
        mkpart part4 5MiB 6MiB \
        mkpart part5 6MiB 7MiB \
        mkpart part6 7MiB 8MiB \
        mkpart part7 8MiB 9MiB \
        mkpart part8 9MiB 10MiB \
        mkpart part9 10MiB 11MiB \
        mkpart part10 11MiB 12MiB \
        mkpart part11 12MiB 13MiB \
        mkpart part12 13MiB 14MiB \
        mkpart part13 14MiB 15MiB \
        mkpart part14 15MiB 16MiB \
        mkpart part15 16MiB 17MiB \
        mkpart part16 17MiB 18MiB \
        mkpart part17 18MiB 19MiB \
        mkpart part18 19MiB 20MiB \
        mkpart part19 20MiB 21MiB \
        mkpart part20 21MiB 22MiB \
        mkpart part21 22MiB 23MiB \
        mkpart part22 23MiB 24MiB \
        mkpart part23 24MiB 25MiB

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

gpt_partitions_s4k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw
    fallocate -l 1GiB $img_raw
    sudo losetup -b 4096 $LOOP_DEV $img_raw
    sudo parted --script --align optimal $LOOP_DEV mktable gpt \
        mkpart part0 1MiB 2MiB \
        mkpart part1 2MiB 3MiB \
        mkpart part2 4MiB 5MiB \
        mkpart part3 3MiB 4MiB \
        mkpart part4 5MiB 6MiB \
        mkpart part5 6MiB 7MiB \
        mkpart part6 7MiB 8MiB \
        mkpart part7 8MiB 9MiB \
        mkpart part8 9MiB 10MiB \
        mkpart part9 10MiB 11MiB \
        mkpart part10 11MiB 12MiB \
        mkpart part11 12MiB 13MiB \
        mkpart part12 13MiB 14MiB \
        mkpart part13 14MiB 15MiB \
        mkpart part14 15MiB 16MiB \
        mkpart part15 16MiB 17MiB \
        mkpart part16 17MiB 18MiB \
        mkpart part17 18MiB 19MiB \
        mkpart part18 19MiB 20MiB \
        mkpart part19 20MiB 21MiB \
        mkpart part20 21MiB 22MiB \
        mkpart part21 22MiB 23MiB \
        mkpart part22 23MiB 24MiB \
        mkpart part23 24MiB 25MiB
    sudo losetup -d $LOOP_DEV

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

kolibri.raw () {
    local img=$FUNCNAME
    touch $img
    fallocate -z -o 0 -l 1440KiB $img
    mkfs.fat -n KOLIBRIOS -F 12 $img > /dev/null
    mcopy -moi $img ../default.skn ::DEFAULT.SKN
    mcopy -moi $img ../fill.cur ::FILL.CUR
    mcopy -moi $img ../spray.cur ::SPRAY.CUR
    mcopy -moi $img ../apps/board_cycle ::LOADER
    mmd -i $img ::LIB
    mcopy -moi $img ../network.obj ::LIB/NETWORK.OBJ
}

jfs.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw
    fallocate -l 16MiB $img_raw
    mkfs.jfs -q $img_raw > /dev/null
    fallocate -i -o 0 -l 1MiB $img_raw
    parted --script --align optimal $img_raw mktable msdos
    parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_lookup_v4.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 10GiB $img_raw
    mkfs.xfs $XFS_MKFS_OPTS -m crc=0 $img_raw
    sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
    sudo chown $USER $TEMP_DIR -R
#
    mkdir $TEMP_DIR/dir_sf
    $MKDOUBLEDIRS $TEMP_DIR/dir_sf d 5
#
    mkdir $TEMP_DIR/dir_block
    $MKDOUBLEDIRS $TEMP_DIR/dir_block d 50
#
    mkdir $TEMP_DIR/dir_leaf
    $MKDOUBLEDIRS $TEMP_DIR/dir_leaf d 500
#
    mkdir $TEMP_DIR/dir_node
    $MKDOUBLEDIRS $TEMP_DIR/dir_node d 2000
#
    mkdir $TEMP_DIR/dir_btree_l1a
    $MKDOUBLEDIRS $TEMP_DIR/dir_btree_l1a d 5000
#
    mkdir $TEMP_DIR/dir_btree_l1b
    $MKDOUBLEDIRS $TEMP_DIR/dir_btree_l1b d 50000
#
    mkdir $TEMP_DIR/dir_btree_l1c
    $MKDOUBLEDIRS $TEMP_DIR/dir_btree_l1c d 500000
#
    mkdir $TEMP_DIR/dir_btree_l2
    $MKDOUBLEDIRS $TEMP_DIR/dir_btree_l2 d 2000000
#
    sudo umount $TEMP_DIR
    fallocate -i -o 0 -l 1MiB $img_raw
    parted --script --align optimal $img_raw mktable msdos
    parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_lookup_v5.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l 10GiB $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -m crc=1 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	mkdir $TEMP_DIR/dir_sf
	$MKDOUBLEDIRS $TEMP_DIR/dir_sf d 5
#
	mkdir $TEMP_DIR/dir_block
	$MKDOUBLEDIRS $TEMP_DIR/dir_block d 50
#
	mkdir $TEMP_DIR/dir_leaf
	$MKDOUBLEDIRS $TEMP_DIR/dir_leaf d 500
#
	mkdir $TEMP_DIR/dir_node
	$MKDOUBLEDIRS $TEMP_DIR/dir_node d 2000
#
	mkdir $TEMP_DIR/dir_btree_l1a
	$MKDOUBLEDIRS $TEMP_DIR/dir_btree_l1a d 5000
#
	mkdir $TEMP_DIR/dir_btree_l1b
	$MKDOUBLEDIRS $TEMP_DIR/dir_btree_l1b d 50000
#
	mkdir $TEMP_DIR/dir_btree_l1c
	$MKDOUBLEDIRS $TEMP_DIR/dir_btree_l1c d 500000
#
	mkdir $TEMP_DIR/dir_btree_l2
	$MKDOUBLEDIRS $TEMP_DIR/dir_btree_l2 d 2000000
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_nrext64.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l 3000MiB $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -i nrext64=1 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	mkdir $TEMP_DIR/dir_sf
	$MKDIRRANGE $TEMP_DIR/dir_sf 0 3  0 2
#
	mkdir $TEMP_DIR/dir_block
	$MKDIRRANGE $TEMP_DIR/dir_block 0 5  201 43
#
	mkdir $TEMP_DIR/dir_leaf
	$MKDIRRANGE $TEMP_DIR/dir_leaf 0 50  201 43
#
	mkdir $TEMP_DIR/dir_node
	$MKDIRRANGE $TEMP_DIR/dir_node 0 1000  201 43
#
	mkdir $TEMP_DIR/dir_btree_l1
	$MKDIRRANGE $TEMP_DIR/dir_btree_l1 0 5000  231 13
#
	mkdir $TEMP_DIR/dir_btree_l2
	$MKDIRRANGE $TEMP_DIR/dir_btree_l2 0 1000000  231 13
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_bigtime.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -m bigtime=1 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	mkdir $TEMP_DIR/dira
	mkdir $TEMP_DIR/dirb
	mkdir $TEMP_DIR/dirc
	mkdir $TEMP_DIR/dird
	mkdir $TEMP_DIR/dire
	mkdir $TEMP_DIR/dirf
	touch -a -t 200504031122.33 $TEMP_DIR/dira
	touch -m -t 200504031122.44 $TEMP_DIR/dira
	touch -a -t 199504031122.33 $TEMP_DIR/dirb
	touch -m -t 203504031122.44 $TEMP_DIR/dirb
	touch -a -t 197504031122.33 $TEMP_DIR/dirc
	touch -m -t 207504031122.44 $TEMP_DIR/dirc
	touch -a -t 192504031122.33 $TEMP_DIR/dird
	touch -m -t 210504031122.44 $TEMP_DIR/dird
	touch -a -t 190004031122.33 $TEMP_DIR/dire
	touch -m -t 220504031122.44 $TEMP_DIR/dire
	touch -a -t 180004031122.33 $TEMP_DIR/dirf
	touch -m -t 220504031122.44 $TEMP_DIR/dirf
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_borg_bit.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -n version=ci $img_raw
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_short_dir_i8.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	echo -en "\x00" > $img_raw
	fallocate -i -o 0 -l 42TiB $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i size=256 -n size=8k,ftype=0 $img_raw
#
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	mkdir $TEMP_DIR/sf
	$MKDIRRANGE $TEMP_DIR/sf 0 5  0 244
#
	$MKDIRRANGE $TEMP_DIR/sf/d0000000002_xx 7 10  0 244
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable gpt
	parted --script --align optimal $img_raw mkpart part0 1MiB 99%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_ftype0_s05k_b2k_n8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n size=8k,ftype=0 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	mkdir $TEMP_DIR/sf_empty
#
	mkdir $TEMP_DIR/sf
	$MKDIRRANGE $TEMP_DIR/sf 0 3  0 244
#
	mkdir $TEMP_DIR/block
	$MKDIRRANGE $TEMP_DIR/block 0 5  234 10
#
	mkdir $TEMP_DIR/leaf
	$MKDIRRANGE $TEMP_DIR/leaf 0 40  214 30
#
	mkdir $TEMP_DIR/node
	$MKDIRRANGE $TEMP_DIR/node 0 1100  0 23
#
	mkdir $TEMP_DIR/btree_leaf
	$MKDIRRANGE $TEMP_DIR/btree_leaf 0 1000  201 43
#
	mkdir $TEMP_DIR/btree_leaf_free
	$MKDIRRANGE $TEMP_DIR/btree_leaf_free 0 1200  201 43
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_ftype1_s05k_b2k_n8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n size=8k,ftype=1 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	mkdir $TEMP_DIR/sf_empty
#
	mkdir $TEMP_DIR/sf
	$MKDIRRANGE $TEMP_DIR/sf 0 3  0 244
#
	mkdir $TEMP_DIR/block
	$MKDIRRANGE $TEMP_DIR/block 0 5  234 10
#
	mkdir $TEMP_DIR/leaf
	$MKDIRRANGE $TEMP_DIR/leaf 0 40  214 30
#
	mkdir $TEMP_DIR/node
	$MKDIRRANGE $TEMP_DIR/node 0 1100  0 23
#
	mkdir $TEMP_DIR/btree_leaf
	$MKDIRRANGE $TEMP_DIR/btree_leaf 0 1000  201 43
#
	mkdir $TEMP_DIR/btree_leaf_free
	$MKDIRRANGE $TEMP_DIR/btree_leaf_free 0 1200  201 43
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_xattr.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n ftype=0 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR -o attr2
	sudo chown $USER $TEMP_DIR -R
#
	mkdir $TEMP_DIR/sf
	$MKDIRRANGE $TEMP_DIR/sf 0 900  0 244
#
	mkdir $TEMP_DIR/leaf
	$MKDIRRANGE $TEMP_DIR/leaf 0 600  0 244
#
	mkdir $TEMP_DIR/node
	$MKDIRRANGE $TEMP_DIR/node 0 600  0 244
#
	mkdir $TEMP_DIR/btree
	$MKDIRRANGE $TEMP_DIR/btree 0 600  0 244
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_btrees_l2.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -b size=1k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n size=4k,ftype=1 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	mkdir $TEMP_DIR/dir_btree_l2
	$MKDIRRANGE $TEMP_DIR/dir_btree_l2 0 193181  214 30
#
	fallocate -l 4KiB $TEMP_DIR/file_btree_l2
	for n in $(seq 1 4000); do
		fallocate -i -l 4KiB -o 0KiB $TEMP_DIR/file_btree_l2
		fallocate -z -l 4KiB -o 0KiB $TEMP_DIR/file_btree_l2
	done
	$MKFILEPATTERN $TEMP_DIR/file_btree_l2 0 16388096
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_files_s05k_b4k_n8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -b size=4k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n size=8k,ftype=0 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	$MKFILEPATTERN $TEMP_DIR/no_hole 0 65536
#
	$MKFILEPATTERN $TEMP_DIR/hole_begin 0 65536
	fallocate -p -o 0 -l 16KiB $TEMP_DIR/hole_begin
#
	$MKFILEPATTERN $TEMP_DIR/hole_middle 0 65536
	fallocate -p -o 32KiB -l 16KiB $TEMP_DIR/hole_middle
#
	$MKFILEPATTERN $TEMP_DIR/hole_end 0 65536
	fallocate -p -o 48KiB -l 16KiB $TEMP_DIR/hole_end
#
	fallocate -l 4KiB $TEMP_DIR/btree_l1_no_hole
	for n in $(seq 1 2000); do
		fallocate -i -l 4KiB -o 0KiB $TEMP_DIR/btree_l1_no_hole
		fallocate -z -l 4KiB -o 0KiB $TEMP_DIR/btree_l1_no_hole
	done
	$MKFILEPATTERN $TEMP_DIR/btree_l1_no_hole 0 8196096
#
#	fallocate -l 5GiB $TEMP_DIR/4GiB_plus
	$MKFILEPATTERN $TEMP_DIR/4GiB_plus 0x120008000 0x1000
	$MKFILEPATTERN $TEMP_DIR/4GiB_plus 0x120000000 0x4000
	$MKFILEPATTERN $TEMP_DIR/4GiB_plus 0xffffe000 0x4000
	$MKFILEPATTERN $TEMP_DIR/4GiB_plus 0x4000 0x4000
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_ftype0_s4k_b4k_n8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -b size=4k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=4k -n size=8k,ftype=0 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	mkdir $TEMP_DIR/sf_empty
#
	mkdir $TEMP_DIR/sf
	$MKDIRRANGE $TEMP_DIR/sf 0 3  0 244
#
	mkdir $TEMP_DIR/block
	$MKDIRRANGE $TEMP_DIR/block 0 5  234 10
#
	mkdir $TEMP_DIR/leaf
	$MKDIRRANGE $TEMP_DIR/leaf 0 40  214 30
#
	mkdir $TEMP_DIR/node
	$MKDIRRANGE $TEMP_DIR/node 0 1100  0 23
#
	mkdir $TEMP_DIR/btree_leaf
	$MKDIRRANGE $TEMP_DIR/btree_leaf 0 1000  201 43
#
	mkdir $TEMP_DIR/btree_leaf_free
	$MKDIRRANGE $TEMP_DIR/btree_leaf_free 0 1200  201 43
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	sudo losetup -b 4096 $LOOP_DEV $img_raw
	sudo parted --script --align optimal $LOOP_DEV mktable msdos \
		mkpart primary 1MiB 100%
	sudo losetup -d $LOOP_DEV

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_ftype0_s05k_b2k_n8k_xattr.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n size=8k,ftype=0 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	mkdir $TEMP_DIR/sf_empty
	setfattr -n user.pew_attr_pew -v pew_value_pew $TEMP_DIR/sf_empty
#
	mkdir $TEMP_DIR/sf
	setfattr -n user.pew_attr_pew -v pew_value_pew $TEMP_DIR/sf
	$MKDIRRANGE $TEMP_DIR/sf 0 3  0 244
#
	mkdir $TEMP_DIR/block
	setfattr -n user.pew_attr_pew -v pew_value_pew $TEMP_DIR/block
	$MKDIRRANGE $TEMP_DIR/block 0 5  234 10
#
	mkdir $TEMP_DIR/leaf
	setfattr -n user.pew_attr_pew -v pew_value_pew $TEMP_DIR/leaf
	$MKDIRRANGE $TEMP_DIR/leaf 0 40  214 30
#
	mkdir $TEMP_DIR/node
	setfattr -n user.pew_attr_pew -v pew_value_pew $TEMP_DIR/node
	$MKDIRRANGE $TEMP_DIR/node 0 1020  0 23
#
	mkdir $TEMP_DIR/btree_leaf
	setfattr -n user.pew_attr_pew -v pew_value_pew $TEMP_DIR/btree_leaf
	$MKDIRRANGE $TEMP_DIR/btree_leaf 0 1000  201 43
#
	mkdir $TEMP_DIR/btree_leaf_free
	setfattr -n user.pew_attr_pew -v pew_value_pew $TEMP_DIR/btree_leaf_free
	$MKDIRRANGE $TEMP_DIR/btree_leaf_free 0 1200  201 43
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_unicode.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -m crc=0,finobt=0,rmapbt=0,reflink=0 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	mkdir -p $TEMP_DIR/dir0
	mkdir -p $TEMP_DIR/дир❦/дир11
	mkdir -p $TEMP_DIR/❦❦❦/д❦р22
	mkdir -p $TEMP_DIR/❦👩❦/
	mkdir -p $TEMP_DIR/❦👩❦/👩❦❦/
	mkdir -p $TEMP_DIR/❦👩❦/❦👩❦/
	mkdir -p $TEMP_DIR/❦👩❦/❦❦👩/
	mkdir $TEMP_DIR/дир3/
#
	echo hello_world > $TEMP_DIR/dir0/file00
	echo привет❦мир > $TEMP_DIR/❦❦❦/д❦р22/❦❦
	echo привет💗мир > $TEMP_DIR/❦❦❦/д❦р22/💗💗
	echo привет❦💗мир > $TEMP_DIR/дир3/файл33
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_v5_ftype1_s05k_b2k_n8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -b size=2k -m crc=1,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n size=8k,ftype=1 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	mkdir $TEMP_DIR/sf_empty
#
	mkdir $TEMP_DIR/sf
	$MKDIRRANGE $TEMP_DIR/sf 0 3  0 244
#
	mkdir $TEMP_DIR/block
	$MKDIRRANGE $TEMP_DIR/block 0 5  234 10
#
	mkdir $TEMP_DIR/leaf
	$MKDIRRANGE $TEMP_DIR/leaf 0 40  214 30
#
	mkdir $TEMP_DIR/node
	$MKDIRRANGE $TEMP_DIR/node 0 1100  0 23
#
	mkdir $TEMP_DIR/btree_leaf
	$MKDIRRANGE $TEMP_DIR/btree_leaf 0 1000  201 43
#
	mkdir $TEMP_DIR/btree_leaf_free
	$MKDIRRANGE $TEMP_DIR/btree_leaf_free 0 1200  201 43
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

xfs_v5_files_s05k_b4k_n8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.xfs $XFS_MKFS_OPTS -b size=4k -m crc=1,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n size=8k,ftype=1 $img_raw
	sudo mount $XFS_MOUNT_OPTS $img_raw $TEMP_DIR
	sudo chown $USER $TEMP_DIR -R
#
	$MKFILEPATTERN $TEMP_DIR/no_hole 0 65536
#
	$MKFILEPATTERN $TEMP_DIR/hole_begin 0 65536
	fallocate -p -o 0 -l 16KiB $TEMP_DIR/hole_begin
#
	$MKFILEPATTERN $TEMP_DIR/hole_middle 0 65536
	fallocate -p -o 32KiB -l 16KiB $TEMP_DIR/hole_middle
#
	$MKFILEPATTERN $TEMP_DIR/hole_end 0 65536
	fallocate -p -o 48KiB -l 16KiB $TEMP_DIR/hole_end
#
	fallocate -l 4KiB $TEMP_DIR/btree_l1_no_hole
	for n in $(seq 1 2000); do
		fallocate -i -l 4KiB -o 0KiB $TEMP_DIR/btree_l1_no_hole
		fallocate -z -l 4KiB -o 0KiB $TEMP_DIR/btree_l1_no_hole
	done
	$MKFILEPATTERN $TEMP_DIR/btree_l1_no_hole 0 8196096
#
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

fat32_test0.raw () {
    local img=$FUNCNAME
	fallocate -l 64MiB $img
	mkfs.fat -n KOLIBRIOS -F 32 $img > /dev/null
	sudo mount -o codepage=866,iocharset=utf8,umask=111,dmask=000 $img $TEMP_DIR
	$RANDDIR $TEMP_DIR 1000 8 255 65536
	$DIRTOTEST $TEMP_DIR $img hd0 > "../test/045_#f70_#fat32_test0.t"
	tree $TEMP_DIR
	du -sh $TEMP_DIR
	sudo umount $TEMP_DIR
}

exfat_s05k_c16k_b16k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.exfat -L KOLIBRIOS -c 16k -b 16k $img_raw > /dev/null
	sudo mount -o codepage=866,iocharset=utf8,umask=111,dmask=000 $img_raw $TEMP_DIR
	mkdir $TEMP_DIR/dir_0
	mkdir $TEMP_DIR/dir_1
	touch $TEMP_DIR/dir_1/file000
	mkdir $TEMP_DIR/dir_1000
	$MKDIRRANGE $TEMP_DIR/dir_1000 0 1000  201 43
	mkdir $TEMP_DIR/dir_10000
	$MKDIRRANGE $TEMP_DIR/dir_10000 0 10000  201 43
#	mkdir $TEMP_DIR/dir_100000
#	$MKDIRRANGE $TEMP_DIR/dir_100000 0 100000  201 43
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

exfat_s05k_c8k_b8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

	fallocate -l $XFS_MIN_PART_SIZE $img_raw
	mkfs.exfat -L KOLIBRIOS -c 8k -b 8k $img_raw > /dev/null
	sudo mount -o codepage=866,iocharset=utf8,umask=111,dmask=000 $img_raw $TEMP_DIR
	mkdir $TEMP_DIR/dir_000
	echo -n '' > $TEMP_DIR/dir_000/file_000
	mkdir $TEMP_DIR/dir_001
	echo -n 'x' > $TEMP_DIR/dir_001/file_001
	mkdir $TEMP_DIR/dir_002
	echo -n 'x' > $TEMP_DIR/dir_002/file_002
	mkdir $TEMP_DIR/dir_003
	echo -n 'x' > $TEMP_DIR/dir_003/file_003
	mkdir $TEMP_DIR/dir_004
	echo -n 'x' > $TEMP_DIR/dir_004/file_004
	mkdir $TEMP_DIR/dir_005
	echo -n 'x' > $TEMP_DIR/dir_005/file_005
	mkdir $TEMP_DIR/dir_006
	echo -n 'x' > $TEMP_DIR/dir_006/file_006
	mkdir $TEMP_DIR/dir_007
	echo -n 'x' > $TEMP_DIR/dir_007/file_007
	mkdir $TEMP_DIR/dir_008
	echo -n 'x' > $TEMP_DIR/dir_008/file_008
	mkdir $TEMP_DIR/dir_009
	echo -n 'x' > $TEMP_DIR/dir_009/file_009
	mkdir $TEMP_DIR/dir_010
	echo -n 'x' > $TEMP_DIR/dir_010/file_010
	mkdir $TEMP_DIR/dir_011
	echo -n 'x' > $TEMP_DIR/dir_011/file_011
	mkdir $TEMP_DIR/dir_012
	echo -n 'x' > $TEMP_DIR/dir_012/file_012
	mkdir $TEMP_DIR/dir_013
	echo -n 'x' > $TEMP_DIR/dir_013/file_013
	mkdir $TEMP_DIR/dir_014
	echo -n 'x' > $TEMP_DIR/dir_014/file_014
	mkdir $TEMP_DIR/dir_015
	echo -n 'x' > $TEMP_DIR/dir_015/file_015
	mkdir $TEMP_DIR/dir_016
	echo -n 'x' > $TEMP_DIR/dir_016/file_016
	mkdir $TEMP_DIR/dir_017
	echo -n 'x' > $TEMP_DIR/dir_017/file_017
	mkdir $TEMP_DIR/dir_018
	echo -n 'x' > $TEMP_DIR/dir_018/file_018
	mkdir $TEMP_DIR/dir_019
	echo -n 'x' > $TEMP_DIR/dir_019/file_019
	mkdir $TEMP_DIR/dir_020
	echo -n 'x' > $TEMP_DIR/dir_020/file_020
	mkdir $TEMP_DIR/dir_021
	echo -n 'x' > $TEMP_DIR/dir_021/file_021
	mkdir $TEMP_DIR/dir_022
	echo -n 'x' > $TEMP_DIR/dir_022/file_022
	mkdir $TEMP_DIR/dir_023
	echo -n 'x' > $TEMP_DIR/dir_023/file_023
	mkdir $TEMP_DIR/dir_024
	echo -n 'x' > $TEMP_DIR/dir_024/file_024
	mkdir $TEMP_DIR/dir_025
	echo -n 'x' > $TEMP_DIR/dir_025/file_025
	mkdir $TEMP_DIR/dir_026
	echo -n 'x' > $TEMP_DIR/dir_026/file_026
	mkdir $TEMP_DIR/dir_027
	echo -n 'x' > $TEMP_DIR/dir_027/file_027
	mkdir $TEMP_DIR/dir_028
	echo -n 'x' > $TEMP_DIR/dir_028/file_028
	mkdir $TEMP_DIR/dir_029
	echo -n 'x' > $TEMP_DIR/dir_029/file_029
	mkdir $TEMP_DIR/dir_030
	echo -n 'x' > $TEMP_DIR/dir_030/file_030
	mkdir $TEMP_DIR/dir_031
	echo -n 'x' > $TEMP_DIR/dir_031/file_031
	sudo umount $TEMP_DIR
	fallocate -i -o 0 -l 1MiB $img_raw
	parted --script --align optimal $img_raw mktable msdos
	parted --script --align optimal $img_raw mkpart primary 1MiB 100%

    qemu-img convert -m 2 -O qcow2 -o $QCOW2_OPTS $img_raw $img
    rm $img_raw
}

images=(gpt_large.qcow2 gpt_partitions_s05k.qcow2 gpt_partitions_s4k.qcow2
        kolibri.raw jfs.qcow2 xfs_lookup_v4.qcow2 xfs_lookup_v5.qcow2
        xfs_nrext64.qcow2 xfs_bigtime.qcow2 xfs_borg_bit.qcow2
        xfs_short_dir_i8.qcow2 xfs_v4_ftype0_s05k_b2k_n8k.qcow2
        xfs_v4_ftype1_s05k_b2k_n8k.qcow2 xfs_v4_xattr.qcow2
        xfs_v4_btrees_l2.qcow2 xfs_v4_files_s05k_b4k_n8k.qcow2
        xfs_v4_ftype0_s4k_b4k_n8k.qcow2 xfs_v4_ftype0_s05k_b2k_n8k_xattr.qcow2
        xfs_v4_unicode.qcow2 xfs_v5_ftype1_s05k_b2k_n8k.qcow2
        xfs_v5_files_s05k_b4k_n8k.qcow2 fat32_test0.raw
        exfat_s05k_c16k_b16k.qcow2 exfat_s05k_c8k_b8k.qcow2)

TEMP_DIR=$(mktemp -d)
LOOP_DEV=$(losetup --find)

for image in ${images[*]}; do
    if [ -f "$image" ]; then
        echo "skipping $image"
        continue
    else
        echo "generate $image"
        $image
    fi
done

sudo umount -q $TEMP_DIR
rmdir $TEMP_DIR
