#!/bin/bash

RANDDIR=../tools/randdir
MKDOUBLEDIRS=../tools/mkdoubledirs
MKFILEPATTERN=../tools/mkfilepattern
DIRTOTEST=../tools/dirtotest.py
MKSAMEHASH=../tools/mksamehash
MKDIRRANGE=../tools/mkdirrange

MKFS_XFS="sudo mkfs.xfs"
XFS_MIN_DISK_SIZE="302MiB"
XFS_MKFS_OPTS="-q -i maxpct=0"
XFS_MOUNT_OPTS="-t xfs -o lazytime"

MKFS_EXT2="sudo mkfs.ext2"
MKFS_EXT4="sudo mkfs.ext4"
EXT_HASH_SEED="01234567-abcd-abcd-abcd-001122334455"
EXT_MKFS_OPTS="-q"
EXT_MOUNT_OPTS="-t ext4 -o lazytime"

MKFS_FAT="sudo mkfs.fat"
FAT_MOUNT_OPTS="-t vfat -o lazytime,umask=111,dmask=000"

MKFS_EXFAT="sudo mkfs.exfat"

QCOW2_OPTS="compat=v3,compression_type=zlib,encryption=off,extended_l2=off,preallocation=off"
QEMU_IMG_CONVERT_OPTS="-m 4 -f raw -O qcow2 -o $QCOW2_OPTS"
NBD_DEV=/dev/nbd0   # FIXME
SGDISK="sgdisk --align-end --disk-guid=abcdefff-0123-4554-3210-ffeeddccbbaa"

gpt_large.qcow2 () {
    local img=$FUNCNAME
    qemu-img create -f qcow2 -o $QCOW2_OPTS,cluster_size=2M $img 2E > /dev/null
    sudo qemu-nbd -c $NBD_DEV $img
    sleep 1
    # This is weird but the sleep above prevents the following error of the
    # sgdisk below. A better understanding and fix is needed.
    # Problem reading disk in BasicMBRData::ReadMBRData()!
    # Warning! Read error 22; strange behavior now likely!
    # Caution! Secondary header was placed beyond the disk's limits! Moving the
    # header, but other problems may occur!
    $SGDISK --clear --new=0:0:+1023MiB --new=0:0:+1023GiB --new=0:0:+1023TiB \
        --new=0:0:+1023PiB $NBD_DEV > /dev/null

    sudo qemu-nbd -d $NBD_DEV > /dev/null
}

gpt_partitions_s05k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 1GiB $img_raw

    $SGDISK --clear --new=0:0:+1MiB --new=0:0:+1MiB --new=0:4MiB:+1MiB \
        --new=0:3MiB:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB \
        --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB \
        --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB \
        --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB \
        --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB \
        --new=0:0:+1MiB $img_raw > /dev/null

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

gpt_partitions_s4k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 1GiB $img_raw
    sudo losetup -b 4096 $LOOP_DEV $img_raw

    $SGDISK --clear --new=0:0:+1MiB --new=0:0:+1MiB --new=0:4MiB:+1MiB \
        --new=0:3MiB:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB \
        --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB \
        --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB \
        --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB \
        --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB --new=0:0:+1MiB \
        --new=0:0:+1MiB $LOOP_DEV > /dev/null

    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
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
    mcopy -moi $img ../apps/network.obj ::LIB/NETWORK.OBJ
    mcopy -moi $img ../apps/console.obj ::LIB/CONSOLE.OBJ
}

jfs.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 16MiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    mkfs.jfs -q $p1  > /dev/null

    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_lookup_v4.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 3GiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -b size=1k -m crc=0 $p1
    sudo mount $XFS_MOUNT_OPTS $p1  $TEMP_DIR
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
    $MKDOUBLEDIRS $TEMP_DIR/dir_btree_l1b d 200000
#
    mkdir $TEMP_DIR/dir_btree_l2
    $MKDOUBLEDIRS $TEMP_DIR/dir_btree_l2 d 1000000
#
    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_lookup_v5.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 5GiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -b size=1k -m crc=1 $p1
    sudo mount $XFS_MOUNT_OPTS $p1  $TEMP_DIR
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
    $MKDOUBLEDIRS $TEMP_DIR/dir_btree_l1b d 200000
#
    mkdir $TEMP_DIR/dir_btree_l2
    $MKDOUBLEDIRS $TEMP_DIR/dir_btree_l2 d 1000000
#
    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_nrext64.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 3GiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -i nrext64=1 $p1
    sudo mount $XFS_MOUNT_OPTS $p1  $TEMP_DIR
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
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_bigtime.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -m bigtime=1 $p1
    sudo mount $XFS_MOUNT_OPTS $p1  $TEMP_DIR
    sudo chown $USER $TEMP_DIR -R
#
    mkdir $TEMP_DIR/dira
    mkdir $TEMP_DIR/dirb
    mkdir $TEMP_DIR/dirc
    mkdir $TEMP_DIR/dird
    mkdir $TEMP_DIR/dire
    mkdir $TEMP_DIR/dirf
    TZ=UTC touch -a -t 200504031122.33 $TEMP_DIR/dira
    TZ=UTC touch -m -t 200504031122.44 $TEMP_DIR/dira
    TZ=UTC touch -a -t 199504031122.33 $TEMP_DIR/dirb
    TZ=UTC touch -m -t 203504031122.44 $TEMP_DIR/dirb
    TZ=UTC touch -a -t 197504031122.33 $TEMP_DIR/dirc
    TZ=UTC touch -m -t 207504031122.44 $TEMP_DIR/dirc
    TZ=UTC touch -a -t 192504031122.33 $TEMP_DIR/dird
    TZ=UTC touch -m -t 210504031122.44 $TEMP_DIR/dird
    TZ=UTC touch -a -t 190004031122.33 $TEMP_DIR/dire
    TZ=UTC touch -m -t 220504031122.44 $TEMP_DIR/dire
    TZ=UTC touch -a -t 180004031122.33 $TEMP_DIR/dirf
    TZ=UTC touch -m -t 220504031122.44 $TEMP_DIR/dirf
#
    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img

    rm $img_raw
}

xfs_borg_bit.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -n version=ci $p1

    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_short_dir_i8.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

#    echo -en "\x00" > $img_raw
    fallocate -l 1MiB $img_raw
    fallocate -i -o 0 -l 42TiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 \
        -d sectsize=512 -i size=256 -n size=8k,ftype=0 $p1
#
    sudo mount $XFS_MOUNT_OPTS $p1  $TEMP_DIR
    sudo chown $USER $TEMP_DIR -R
#
    mkdir $TEMP_DIR/sf
    $MKDIRRANGE $TEMP_DIR/sf 0 5  0 244
#
    $MKDIRRANGE $TEMP_DIR/sf/d0000000002_xx 7 10  0 244
#
    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_ftype0_s05k_b2k_n8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 \
        -d sectsize=512 -n size=8k,ftype=0 $p1
    sudo mount $XFS_MOUNT_OPTS $p1  $TEMP_DIR
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
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_ftype1_s05k_b2k_n8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 \
        -d sectsize=512 -n size=8k,ftype=1 $p1
    sudo mount $XFS_MOUNT_OPTS $p1  $TEMP_DIR
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
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_xattr.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -m crc=0,finobt=0,rmapbt=0,reflink=0 \
        -d sectsize=512 -n ftype=0 $p1
    sudo mount $XFS_MOUNT_OPTS $p1 $TEMP_DIR
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
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_btrees_l2.qcow2 () {
    echo "[*] This may take about ten minutes"
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -b size=1k -m crc=0,finobt=0,rmapbt=0,reflink=0 \
        -d sectsize=512 -n size=4k,ftype=1 $p1
    sudo mount $XFS_MOUNT_OPTS $p1  $TEMP_DIR
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
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_files_s05k_b4k_n8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -b size=4k -m crc=0,finobt=0,rmapbt=0,reflink=0 \
        -d sectsize=512 -n size=8k,ftype=0 $p1
    sudo mount $XFS_MOUNT_OPTS $p1  $TEMP_DIR
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
#   fallocate -l 5GiB $TEMP_DIR/4GiB_plus
    $MKFILEPATTERN $TEMP_DIR/4GiB_plus 0x120008000 0x1000
    $MKFILEPATTERN $TEMP_DIR/4GiB_plus 0x120000000 0x4000
    $MKFILEPATTERN $TEMP_DIR/4GiB_plus 0xffffe000 0x4000
    $MKFILEPATTERN $TEMP_DIR/4GiB_plus 0x4000 0x4000
#
    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_ftype0_s4k_b4k_n8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    sudo losetup -b 4096 $LOOP_DEV $img_raw
    $SGDISK --clear --new=0:0:0 $LOOP_DEV > /dev/null
    sudo losetup -d $LOOP_DEV
    sudo losetup -b 4096 -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -b size=4k -m crc=0,finobt=0,rmapbt=0,reflink=0 \
        -d sectsize=4k -n size=8k,ftype=0 $p1
    sudo mount $XFS_MOUNT_OPTS $p1  $TEMP_DIR
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
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_ftype0_s05k_b2k_n8k_xattr.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 \
        -d sectsize=512 -n size=8k,ftype=0 $p1
    sudo mount $XFS_MOUNT_OPTS $p1  $TEMP_DIR
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
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_v4_unicode.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -m crc=0,finobt=0,rmapbt=0,reflink=0 $p1
    sudo mount $XFS_MOUNT_OPTS $p1 $TEMP_DIR
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
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_v5_ftype1_s05k_b2k_n8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -b size=2k -m crc=1,finobt=0,rmapbt=0,reflink=0 \
        -d sectsize=512 -n size=8k,ftype=1 $p1
    sudo mount $XFS_MOUNT_OPTS $p1 $TEMP_DIR
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
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_v5_files_s05k_b4k_n8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -b size=4k -m crc=1,finobt=0,rmapbt=0,reflink=0 \
        -d sectsize=512 -n size=8k,ftype=1 $p1
    sudo mount $XFS_MOUNT_OPTS $p1 $TEMP_DIR
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
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

fat32_test0.raw () {
    local img=$FUNCNAME

    fallocate -l 64MiB $img
    $SGDISK --clear --new=0:0:0 $img > /dev/null
    sudo losetup -P $LOOP_DEV $img
    local p1="$LOOP_DEV"p1

    $MKFS_FAT -n KOLIBRIOS -F 32 $p1 > /dev/null
    sudo mount -o codepage=866,iocharset=utf8,umask=111,dmask=000 $p1 $TEMP_DIR

    $RANDDIR $TEMP_DIR 1000 8 255 65536
    $DIRTOTEST $TEMP_DIR $img hd0 > "../test/t045/run.us"

#    tree $TEMP_DIR
#    du -sh $TEMP_DIR

    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV
}

exfat_s05k_c16k_b16k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_EXFAT -L KOLIBRIOS -c 16k -b 16k $p1 > /dev/null
    sudo mount -o umask=111,dmask=000 $p1 $TEMP_DIR

    mkdir $TEMP_DIR/dir_0
    mkdir $TEMP_DIR/dir_1
    touch $TEMP_DIR/dir_1/file000
    mkdir $TEMP_DIR/dir_1000
    $MKDIRRANGE $TEMP_DIR/dir_1000 0 1000  201 43
    mkdir $TEMP_DIR/dir_10000
    $MKDIRRANGE $TEMP_DIR/dir_10000 0 10000  201 43
#   mkdir $TEMP_DIR/dir_100000
#   $MKDIRRANGE $TEMP_DIR/dir_100000 0 100000  201 43

    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

exfat_s05k_c8k_b8k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l $XFS_MIN_DISK_SIZE $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_EXFAT -L KOLIBRIOS -c 8k -b 8k $p1 > /dev/null
    sudo mount -o umask=111,dmask=000 $p1 $TEMP_DIR
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
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

xfs_samehash_s05k.raw () {
    echo "[*] This may take about one hour"
    local img=$FUNCNAME
#    local img_raw=$(basename $img .qcow2).raw
    local img_raw=$img

    fallocate -l 1GiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_XFS $XFS_MKFS_OPTS -b size=1k -m crc=1 $p1
    sudo mount $p1 $TEMP_DIR
    sudo chown $USER $TEMP_DIR -R
#
    mkdir $TEMP_DIR/dir_sf
    $MKSAMEHASH $TEMP_DIR/dir_sf 2 -q
#
    mkdir $TEMP_DIR/dir_block
    $MKSAMEHASH $TEMP_DIR/dir_block 20 -q
#
    mkdir $TEMP_DIR/dir_leaf
    $MKSAMEHASH $TEMP_DIR/dir_leaf 100 -q
#
    mkdir $TEMP_DIR/dir_node
    $MKSAMEHASH $TEMP_DIR/dir_node 1000 -q
#
    mkdir $TEMP_DIR/dir_btree_l1
    $MKSAMEHASH $TEMP_DIR/dir_btree_l1 10000 -q
#
    mkdir $TEMP_DIR/dir_btree_l2
    $MKSAMEHASH $TEMP_DIR/dir_btree_l2 500000 -q
#
    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV

#    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
}

ext2_s05k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 5GiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_EXT2 $EXT_MKFS_OPTS -N 1200000 $p1
    sudo debugfs -w -R "set_super_value hash_seed $EXT_HASH_SEED" $p1
    sudo mount $p1 $TEMP_DIR
    sudo chown $USER $TEMP_DIR -R
#
    mkdir $TEMP_DIR/dir_a
    $MKDIRRANGE $TEMP_DIR/dir_a 0 3  0 1
#
    mkdir $TEMP_DIR/dir_b
    $MKDIRRANGE $TEMP_DIR/dir_b 0 10  0 1
#
    mkdir $TEMP_DIR/dir_c
    $MKDIRRANGE $TEMP_DIR/dir_c 0 100  0 1
#
    mkdir $TEMP_DIR/dir_d
    $MKDIRRANGE $TEMP_DIR/dir_d 0 1000  0 1
#
    mkdir $TEMP_DIR/dir_e
    $MKDIRRANGE $TEMP_DIR/dir_e 0 10000  0 1
#
    mkdir $TEMP_DIR/dir_f
    $MKDIRRANGE $TEMP_DIR/dir_f 0 64998  0 1
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
    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

ext2_symlinks.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 15MiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_EXT2 -b 1024 $EXT_MKFS_OPTS $p1

    sudo debugfs -w -R "set_super_value hash_seed $EXT_HASH_SEED" $p1

    sudo mount $p1 $TEMP_DIR
    sudo chown $USER $TEMP_DIR -R

    local SLOW_PAD=$(printf './%.0s' {1..40})
    local HUGE_PAD=$(printf 'x%.0s' {1..1000})

# --- TEST 69: BASIC VALID LINKS AND LOOPS ---
    mkdir -p $TEMP_DIR/targets/t69/subdir/deep
    mkdir $TEMP_DIR/t69
    echo "target69" > $TEMP_DIR/targets/t69/target_fast.txt
    echo "target69" > $TEMP_DIR/targets/t69/target_slow.txt
    echo "insidedir69" > $TEMP_DIR/targets/t69/subdir/file.txt
    echo "deepfile69" > $TEMP_DIR/targets/t69/subdir/deep/file.txt

    # File symlink chains (fast/slow x abs/rel)
    ln -s ../targets/t69/target_fast.txt $TEMP_DIR/t69/f_rel_target
    ln -s f_rel_target $TEMP_DIR/t69/f_rel_entry
    ln -s /targets/t69/target_fast.txt $TEMP_DIR/t69/f_abs_target
    ln -s /t69/f_abs_target $TEMP_DIR/t69/f_abs_entry
    ln -s ../targets/t69/${SLOW_PAD}target_slow.txt $TEMP_DIR/t69/s_rel_target
    ln -s ${SLOW_PAD}s_rel_target $TEMP_DIR/t69/s_rel_entry
    ln -s /targets/t69/${SLOW_PAD}target_slow.txt $TEMP_DIR/t69/s_abs_target
    ln -s /t69/${SLOW_PAD}s_abs_target $TEMP_DIR/t69/s_abs_entry

    # Mixed file chain (fast->slow->abs->rel)
    ln -s ../targets/t69/target_fast.txt $TEMP_DIR/t69/mix_f_rel
    ln -s /t69/mix_f_rel $TEMP_DIR/t69/mix_f_abs
    ln -s ${SLOW_PAD}mix_f_abs $TEMP_DIR/t69/mix_s_rel
    ln -s /t69/${SLOW_PAD}mix_s_rel $TEMP_DIR/t69/mix_s_abs_entry

    # Dir symlink chains (fast/slow x abs/rel)
    ln -s ../targets/t69/subdir $TEMP_DIR/t69/d_frel_target
    ln -s d_frel_target $TEMP_DIR/t69/d_frel_entry
    ln -s /targets/t69/subdir $TEMP_DIR/t69/d_fabs_target
    ln -s /t69/d_fabs_target $TEMP_DIR/t69/d_fabs_entry
    ln -s ../targets/t69/${SLOW_PAD}subdir $TEMP_DIR/t69/d_srel_target
    ln -s ${SLOW_PAD}d_srel_target $TEMP_DIR/t69/d_srel_entry
    ln -s /targets/t69/${SLOW_PAD}subdir $TEMP_DIR/t69/d_sabs_target
    ln -s /t69/${SLOW_PAD}d_sabs_target $TEMP_DIR/t69/d_sabs_entry

    # Mixed dir chain
    ln -s ../targets/t69/subdir $TEMP_DIR/t69/d_mix_frel
    ln -s /t69/d_mix_frel $TEMP_DIR/t69/d_mix_fabs
    ln -s ${SLOW_PAD}d_mix_fabs $TEMP_DIR/t69/d_mix_srel
    ln -s /t69/${SLOW_PAD}d_mix_srel $TEMP_DIR/t69/d_mix_sabs_entry

# --- TEST 70: NESTED NETWORKS ---
    mkdir -p $TEMP_DIR/targets/t70/subdir
    mkdir $TEMP_DIR/t70
    echo "target70" > $TEMP_DIR/targets/t70/target.txt
    echo "target70dir" > $TEMP_DIR/targets/t70/subdir/file.txt

    # Pure nested file chains (40 deep)
    ln -s ../targets/t70/target.txt $TEMP_DIR/t70/pure_frel_1
    for i in {2..40}; do ln -s pure_frel_$((i-1)) $TEMP_DIR/t70/pure_frel_$i; done
    ln -s ../targets/t70/${SLOW_PAD}target.txt $TEMP_DIR/t70/pure_srel_1
    for i in {2..40}; do ln -s ${SLOW_PAD}pure_srel_$((i-1)) $TEMP_DIR/t70/pure_srel_$i; done
    ln -s /targets/t70/target.txt $TEMP_DIR/t70/pure_fabs_1
    for i in {2..40}; do ln -s /t70/pure_fabs_$((i-1)) $TEMP_DIR/t70/pure_fabs_$i; done
    ln -s /targets/t70/${SLOW_PAD}target.txt $TEMP_DIR/t70/pure_sabs_1
    for i in {2..40}; do ln -s /t70/${SLOW_PAD}pure_sabs_$((i-1)) $TEMP_DIR/t70/pure_sabs_$i; done

    # Mixed nested file chain (42 deep)
    ln -s ../targets/t70/target.txt $TEMP_DIR/t70/hop1_frel
    prev_name="hop1_frel"
    for i in {2..42}; do
        type=$((i % 4))
        if [ $type -eq 0 ]; then name="hop${i}_frel"; ln -s ${prev_name} $TEMP_DIR/t70/${name}
        elif [ $type -eq 1 ]; then name="hop${i}_srel"; ln -s ${SLOW_PAD}${prev_name} $TEMP_DIR/t70/${name}
        elif [ $type -eq 2 ]; then name="hop${i}_fabs"; ln -s /t70/${prev_name} $TEMP_DIR/t70/${name}
        else name="hop${i}_sabs"; ln -s /t70/${SLOW_PAD}${prev_name} $TEMP_DIR/t70/${name}
        fi
        prev_name=$name
    done

    # File symlink loops
    ln -s rel_loop_b $TEMP_DIR/t70/rel_loop_a
    ln -s rel_loop_a $TEMP_DIR/t70/rel_loop_b
    ln -s /t70/abs_loop_b $TEMP_DIR/t70/abs_loop_a
    ln -s /t70/abs_loop_a $TEMP_DIR/t70/abs_loop_b

    # Pure nested dir chains (40 deep)
    ln -s ../targets/t70/subdir $TEMP_DIR/t70/d_pure_frel_1
    for i in {2..40}; do ln -s d_pure_frel_$((i-1)) $TEMP_DIR/t70/d_pure_frel_$i; done
    ln -s ../targets/t70/${SLOW_PAD}subdir $TEMP_DIR/t70/d_pure_srel_1
    for i in {2..40}; do ln -s ${SLOW_PAD}d_pure_srel_$((i-1)) $TEMP_DIR/t70/d_pure_srel_$i; done
    ln -s /targets/t70/subdir $TEMP_DIR/t70/d_pure_fabs_1
    for i in {2..40}; do ln -s /t70/d_pure_fabs_$((i-1)) $TEMP_DIR/t70/d_pure_fabs_$i; done
    ln -s /targets/t70/${SLOW_PAD}subdir $TEMP_DIR/t70/d_pure_sabs_1
    for i in {2..40}; do ln -s /t70/${SLOW_PAD}d_pure_sabs_$((i-1)) $TEMP_DIR/t70/d_pure_sabs_$i; done

    # Mixed nested dir chain (42 deep)
    ln -s ../targets/t70/subdir $TEMP_DIR/t70/d_hop1_frel
    prev_name="d_hop1_frel"
    for i in {2..42}; do
        type=$((i % 4))
        if [ $type -eq 0 ]; then name="d_hop${i}_frel"; ln -s ${prev_name} $TEMP_DIR/t70/${name}
        elif [ $type -eq 1 ]; then name="d_hop${i}_srel"; ln -s ${SLOW_PAD}${prev_name} $TEMP_DIR/t70/${name}
        elif [ $type -eq 2 ]; then name="d_hop${i}_fabs"; ln -s /t70/${prev_name} $TEMP_DIR/t70/${name}
        else name="d_hop${i}_sabs"; ln -s /t70/${SLOW_PAD}${prev_name} $TEMP_DIR/t70/${name}
        fi
        prev_name=$name
    done

    # Dir symlink loops
    ln -s d_rel_loop_b $TEMP_DIR/t70/d_rel_loop_a
    ln -s d_rel_loop_a $TEMP_DIR/t70/d_rel_loop_b
    ln -s /t70/d_abs_loop_b $TEMP_DIR/t70/d_abs_loop_a
    ln -s /t70/d_abs_loop_a $TEMP_DIR/t70/d_abs_loop_b

# --- TEST 71: INVALID LINKS ---
    mkdir $TEMP_DIR/t71

    # Invalid file symlinks
    ln -s does_not_exist $TEMP_DIR/t71/f_rel_inv
    ln -s /does_not_exist $TEMP_DIR/t71/f_abs_inv
    ln -s ${SLOW_PAD}does_not_exist $TEMP_DIR/t71/s_rel_inv
    ln -s /${SLOW_PAD}does_not_exist $TEMP_DIR/t71/s_abs_inv
    ln -s f_rel_inv $TEMP_DIR/t71/nested_inv
    ln -s ${HUGE_PAD}missing $TEMP_DIR/t71/huge_inv

    # Invalid dir symlinks
    ln -s no_such_dir $TEMP_DIR/t71/d_frel_inv
    ln -s /no_such_dir $TEMP_DIR/t71/d_fabs_inv
    ln -s ${SLOW_PAD}no_such_dir $TEMP_DIR/t71/d_srel_inv
    ln -s /${SLOW_PAD}no_such_dir $TEMP_DIR/t71/d_sabs_inv
    ln -s d_frel_inv $TEMP_DIR/t71/d_nested_inv
    ln -s ${HUGE_PAD}no_such_dir $TEMP_DIR/t71/d_huge_inv
#
    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

ext4_s05k.qcow2 () {
    echo "[*] This may take about fifteen minutes"
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 5GiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_EXT4 $EXT_MKFS_OPTS -N 1200000 $p1
    sudo debugfs -w -R "set_super_value hash_seed $EXT_HASH_SEED" $p1
    sudo mount $p1 $TEMP_DIR
    sudo chown $USER $TEMP_DIR -R
#
    mkdir $TEMP_DIR/dir_a
    $MKDIRRANGE $TEMP_DIR/dir_a 0 3  0 1
#
    mkdir $TEMP_DIR/dir_b
    $MKDIRRANGE $TEMP_DIR/dir_b 0 10  0 1
#
    mkdir $TEMP_DIR/dir_c
    $MKDIRRANGE $TEMP_DIR/dir_c 0 100  0 1
#
    mkdir $TEMP_DIR/dir_d
    $MKDIRRANGE $TEMP_DIR/dir_d 0 1000  0 1
#
    mkdir $TEMP_DIR/dir_e
    $MKDIRRANGE $TEMP_DIR/dir_e 0 10000  0 1
#
    mkdir $TEMP_DIR/dir_f
    $MKDIRRANGE $TEMP_DIR/dir_f 0 100000  0 1
#
    mkdir $TEMP_DIR/dir_g
    $MKDIRRANGE $TEMP_DIR/dir_g 0 1000000  0 1
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
    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

fat12_s05k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 256MiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_FAT -F 12 $p1 > /dev/null
    sudo mount $FAT_MOUNT_OPTS $p1 $TEMP_DIR
#
    mkdir $TEMP_DIR/dir_a
    $MKDIRRANGE $TEMP_DIR/dir_a 0 3  0 1
#
    mkdir $TEMP_DIR/dir_b
    $MKDIRRANGE $TEMP_DIR/dir_b 0 10  0 1
#
    mkdir $TEMP_DIR/dir_c
    $MKDIRRANGE $TEMP_DIR/dir_c 0 100  0 1
#
    mkdir $TEMP_DIR/dir_d
    $MKDIRRANGE $TEMP_DIR/dir_d 0 1000  0 1
#
    mkdir $TEMP_DIR/dir_e
    $MKDIRRANGE $TEMP_DIR/dir_e 0 2000  0 1
#
    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

fat16_s05k.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 4GiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_FAT -F 16 $p1 > /dev/null
    sudo mount $FAT_MOUNT_OPTS $p1 $TEMP_DIR
#
    mkdir $TEMP_DIR/dir_a
    $MKDIRRANGE $TEMP_DIR/dir_a 0 3  0 1
#
    mkdir $TEMP_DIR/dir_b
    $MKDIRRANGE $TEMP_DIR/dir_b 0 10  0 1
#
    mkdir $TEMP_DIR/dir_c
    $MKDIRRANGE $TEMP_DIR/dir_c 0 100  0 1
#
    mkdir $TEMP_DIR/dir_d
    $MKDIRRANGE $TEMP_DIR/dir_d 0 1000  0 1
#
    mkdir $TEMP_DIR/dir_e
    $MKDIRRANGE $TEMP_DIR/dir_e 0 10000  0 1
#
    mkdir $TEMP_DIR/dir_f
    $MKDIRRANGE $TEMP_DIR/dir_f 0 30000  0 1
#
    sudo umount $TEMP_DIR
    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

iso9660_s2k_dir_all.qcow2 () {
    echo "[*] This may take about thirty minutes"
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    sudo chown $USER $TEMP_DIR -R

    mkdir $TEMP_DIR/dir_ls_a

    mkdir $TEMP_DIR/dir_ls_b
    $MKDIRRANGE $TEMP_DIR/dir_ls_b 0 3  0 2

    mkdir $TEMP_DIR/dir_ls_c
    $MKDIRRANGE $TEMP_DIR/dir_ls_c 0 5  201 43

    mkdir $TEMP_DIR/dir_ls_d
    $MKDIRRANGE $TEMP_DIR/dir_ls_d 0 50  201 43

    mkdir $TEMP_DIR/dir_ls_e
    $MKDIRRANGE $TEMP_DIR/dir_ls_e 0 1000  201 43

    mkdir $TEMP_DIR/dir_ls_f
    $MKDIRRANGE $TEMP_DIR/dir_ls_f 0 5000  231 13

    mkdir $TEMP_DIR/dir_ls_g
    $MKDIRRANGE $TEMP_DIR/dir_ls_g 0 46656  231 13

    mkdir $TEMP_DIR/dir_stat_a

    mkdir $TEMP_DIR/dir_stat_b
    $MKDOUBLEDIRS $TEMP_DIR/dir_stat_b d 3

    mkdir $TEMP_DIR/dir_stat_c
    $MKDOUBLEDIRS $TEMP_DIR/dir_stat_c d 100

    mkdir $TEMP_DIR/dir_stat_d
    $MKDOUBLEDIRS $TEMP_DIR/dir_stat_d d 5000

    mkdir $TEMP_DIR/dir_stat_e
    $MKDOUBLEDIRS $TEMP_DIR/dir_stat_e d 3861

    mkisofs -J -R -T -V 'KolibriOS' -input-charset 'UTF-8' -quiet $TEMP_DIR > $img_raw
    rm $TEMP_DIR/* -rf
    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

ext2_extra_isize.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 15MiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_EXT2 -b 1024 -I 256 -O extra_isize $EXT_MKFS_OPTS $p1

    sudo debugfs -w -R "set_super_value hash_seed $EXT_HASH_SEED" $p1

    sudo mount $p1 $TEMP_DIR
    sudo chown $USER $TEMP_DIR -R

    mkdir $TEMP_DIR/dira
    touch $TEMP_DIR/file_1985
    TZ=UTC touch -a -t 198512312359.59 $TEMP_DIR/file_1985
    TZ=UTC touch -m -t 198512312359.59 $TEMP_DIR/file_1985

    touch $TEMP_DIR/file_2015
    TZ=UTC touch -a -t 201512312359.59 $TEMP_DIR/file_2015
    TZ=UTC touch -m -t 201512312359.59 $TEMP_DIR/file_2015

    touch $TEMP_DIR/file_2045
    TZ=UTC touch -a -t 204512312359.59 $TEMP_DIR/file_2045
    TZ=UTC touch -m -t 204512312359.59 $TEMP_DIR/file_2045

    touch $TEMP_DIR/file_2085
    TZ=UTC touch -a -t 208512312359.59 $TEMP_DIR/file_2085
    TZ=UTC touch -m -t 208512312359.59 $TEMP_DIR/file_2085

    touch $TEMP_DIR/file_2110
    TZ=UTC touch -a -t 211012312359.59 $TEMP_DIR/file_2110
    TZ=UTC touch -m -t 211012312359.59 $TEMP_DIR/file_2110

    touch $TEMP_DIR/file_2200
    TZ=UTC touch -a -t 220012312359.59 $TEMP_DIR/file_2200
    TZ=UTC touch -m -t 220012312359.59 $TEMP_DIR/file_2200

    touch $TEMP_DIR/file_ctime_crtime
    TZ=UTC touch -a -t 202401011200.00 $TEMP_DIR/file_ctime_crtime
    TZ=UTC touch -m -t 202401011200.00 $TEMP_DIR/file_ctime_crtime

    sudo umount $TEMP_DIR

    sudo debugfs -w -R "set_inode_field file_ctime_crtime ctime 20200101000000" $p1
    sudo debugfs -w -R "set_inode_field file_ctime_crtime crtime 20100101000000" $p1

    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
    rm $img_raw
}

ext2_rev0.qcow2 () {
    local img=$FUNCNAME
    local img_raw=$(basename $img .qcow2).raw

    fallocate -l 18MiB $img_raw
    $SGDISK --clear --new=0:0:0 $img_raw > /dev/null
    sudo losetup -P $LOOP_DEV $img_raw
    local p1="$LOOP_DEV"p1

    $MKFS_EXT2 -E revision=0 $EXT_MKFS_OPTS $p1

    sudo debugfs -w -R "set_super_value feature_compat 0" $p1
    sudo debugfs -w -R "set_super_value feature_incompat 0" $p1
    sudo debugfs -w -R "set_super_value feature_ro_compat 0" $p1
    sudo debugfs -w -R "set_super_value first_ino 0" $p1
    sudo debugfs -w -R "set_super_value inode_size 0" $p1

    # Zero out all modern extensions after the UUID (offset 120 to 1024)
    sudo dd if=/dev/zero of=$p1 bs=1 seek=$((1024 + 120)) count=$((1024 - 120)) conv=notrunc

    sudo losetup -d $LOOP_DEV

    qemu-img convert $QEMU_IMG_CONVERT_OPTS $img_raw $img
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
        exfat_s05k_c16k_b16k.qcow2 exfat_s05k_c8k_b8k.qcow2
        xfs_samehash_s05k.raw ext2_s05k.qcow2 ext4_s05k.qcow2 fat12_s05k.qcow2
        fat16_s05k.qcow2 iso9660_s2k_dir_all.qcow2 ext2_extra_isize.qcow2
        ext2_symlinks.qcow2 ext2_rev0.qcow2)

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
