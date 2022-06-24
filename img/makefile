RANDDIR=../tools/randdir
MKDIRRANGE=../tools/mkdirrange
MKFILEPATTERN=../tools/mkfilepattern
DIRTOTEST=python3 ../tools/dirtotest.py
MOUNT_OPT=-t xfs
TEMP_DIR:=$(shell mktemp -d)

all: s05k s4k unicode v5 kolibri.img fat32_test0.img coverage
	rmdir $(TEMP_DIR)

s05k: xfs_v4_ftype0_s05k_b2k_n8k.img xfs_v4_ftype1_s05k_b2k_n8k.img xfs_v4_xattr.img xfs_v4_files_s05k_b4k_n8k.img xfs_v4_ftype0_s05k_b2k_n8k_xattr.img xfs_v4_btrees_l2.img xfs_short_dir_i8.img exfat_s05k_c16k_b16k.img

s4k: xfs_v4_ftype0_s4k_b4k_n8k.img

unicode: xfs_v4_unicode.img

v5: xfs_v5_ftype1_s05k_b2k_n8k.img xfs_v5_files_s05k_b4k_n8k.img

coverage: jfs.img xfs_borg_bit.img xfs_short_dir_i8.img

kolibri.img: ../default.skn ../fill.cur
	touch $@
	fallocate -z -o 0 -l 1440KiB $@
	mkfs.vfat -n KOLIBRIOS -F 12 $@
	mcopy -vmoi $@ ../default.skn ::DEFAULT.SKN
	mcopy -vmoi $@ ../fill.cur ::FILL.CUR
	mcopy -vmoi $@ ../spray.cur ::SPRAY.CUR
	mcopy -vmoi $@ ../apps/board_cycle ::LOADER
	mmd -i $@ ::LIB
	mcopy -vmoi $@ ../network.obj ::LIB/NETWORK.OBJ

jfs.img:
	fallocate -l 16MiB $@
	mkfs.jfs -q $@
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_borg_bit.img:
	fallocate -l 16MiB $@
	mkfs.xfs -n version=ci $@
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_short_dir_i8.img:
	echo -en "\x00" > $@
	fallocate -i -o 0 -l 42TiB $@
	mkfs.xfs -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100,size=256 -n size=8k,ftype=0 $@
#
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/sf
	$(MKDIRRANGE) $(TEMP_DIR)/sf 0 5  0 244
#
	$(MKDIRRANGE) $(TEMP_DIR)/sf/d0000000002_xx 7 10  0 244
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_v4_ftype0_s05k_b2k_n8k.img:
	fallocate -l 63MiB $@
	mkfs.xfs -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n size=8k,ftype=0 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/sf_empty
#
	mkdir $(TEMP_DIR)/sf
	$(MKDIRRANGE) $(TEMP_DIR)/sf 0 3  0 244
#
	mkdir $(TEMP_DIR)/block
	$(MKDIRRANGE) $(TEMP_DIR)/block 0 5  234 10
#
	mkdir $(TEMP_DIR)/leaf
	$(MKDIRRANGE) $(TEMP_DIR)/leaf 0 40  214 30
#
	mkdir $(TEMP_DIR)/node
	$(MKDIRRANGE) $(TEMP_DIR)/node 0 1100  0 23
#
	mkdir $(TEMP_DIR)/btree_leaf
	$(MKDIRRANGE) $(TEMP_DIR)/btree_leaf 0 1000  201 43
#
	mkdir $(TEMP_DIR)/btree_leaf_free
	$(MKDIRRANGE) $(TEMP_DIR)/btree_leaf_free 0 1200  201 43
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_v4_ftype1_s05k_b2k_n8k.img:
	fallocate -l 63MiB $@
	mkfs.xfs -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n size=8k,ftype=1 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/sf_empty
#
	mkdir $(TEMP_DIR)/sf
	$(MKDIRRANGE) $(TEMP_DIR)/sf 0 3  0 244
#
	mkdir $(TEMP_DIR)/block
	$(MKDIRRANGE) $(TEMP_DIR)/block 0 5  234 10
#
	mkdir $(TEMP_DIR)/leaf
	$(MKDIRRANGE) $(TEMP_DIR)/leaf 0 40  214 30
#
	mkdir $(TEMP_DIR)/node
	$(MKDIRRANGE) $(TEMP_DIR)/node 0 1100  0 23
#
	mkdir $(TEMP_DIR)/btree_leaf
	$(MKDIRRANGE) $(TEMP_DIR)/btree_leaf 0 1000  201 43
#
	mkdir $(TEMP_DIR)/btree_leaf_free
	$(MKDIRRANGE) $(TEMP_DIR)/btree_leaf_free 0 1200  201 43
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_v4_xattr.img:
	fallocate -l 63MiB $@
	mkfs.xfs -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n ftype=0 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR) -o attr2
	sudo chown $$USER $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/sf
	$(MKDIRRANGE) $(TEMP_DIR)/sf 0 900  0 244
#
	mkdir $(TEMP_DIR)/leaf
	$(MKDIRRANGE) $(TEMP_DIR)/leaf 0 600  0 244
#
	mkdir $(TEMP_DIR)/node
	$(MKDIRRANGE) $(TEMP_DIR)/node 0 600  0 244
#
	mkdir $(TEMP_DIR)/btree
	$(MKDIRRANGE) $(TEMP_DIR)/btree 0 600  0 244
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_v4_btrees_l2.img:
	fallocate -l 127MiB $@
	mkfs.xfs -b size=1k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n size=4k,ftype=1 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/dir_btree_l2
	$(MKDIRRANGE) $(TEMP_DIR)/dir_btree_l2 0 193181  214 30
#
	fallocate -l 4KiB $(TEMP_DIR)/file_btree_l2
	for n in $$(seq 1 4000); do \
		fallocate -i -l 4KiB -o 0KiB $(TEMP_DIR)/file_btree_l2 ; \
		fallocate -z -l 4KiB -o 0KiB $(TEMP_DIR)/file_btree_l2 ; \
	done
	$(MKFILEPATTERN) $(TEMP_DIR)/file_btree_l2 0 16388096
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_v4_files_s05k_b4k_n8k.img:
	fallocate -l 127MiB $@
	mkfs.xfs -b size=4k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n size=8k,ftype=0 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	$(MKFILEPATTERN) $(TEMP_DIR)/no_hole 0 65536
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_begin 0 65536
	fallocate -p -o 0 -l 16KiB $(TEMP_DIR)/hole_begin
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_middle 0 65536
	fallocate -p -o 32KiB -l 16KiB $(TEMP_DIR)/hole_middle
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_end 0 65536
	fallocate -p -o 48KiB -l 16KiB $(TEMP_DIR)/hole_end
#
	fallocate -l 4KiB $(TEMP_DIR)/btree_l1_no_hole
	for n in $$(seq 1 2000); do \
		fallocate -i -l 4KiB -o 0KiB $(TEMP_DIR)/btree_l1_no_hole ; \
		fallocate -z -l 4KiB -o 0KiB $(TEMP_DIR)/btree_l1_no_hole ; \
	done
	$(MKFILEPATTERN) $(TEMP_DIR)/btree_l1_no_hole 0 8196096
#
#	fallocate -l 5GiB $(TEMP_DIR)/4GiB_plus
	$(MKFILEPATTERN) $(TEMP_DIR)/4GiB_plus 0x120008000 0x1000
	$(MKFILEPATTERN) $(TEMP_DIR)/4GiB_plus 0x120000000 0x4000
	$(MKFILEPATTERN) $(TEMP_DIR)/4GiB_plus 0xffffe000 0x4000
	$(MKFILEPATTERN) $(TEMP_DIR)/4GiB_plus 0x4000 0x4000
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_v4_ftype0_s4k_b4k_n8k.img:
	fallocate -l 63MiB $@
	mkfs.xfs -b size=4k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=4k -i maxpct=100 -n size=8k,ftype=0 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/sf_empty
#
	mkdir $(TEMP_DIR)/sf
	$(MKDIRRANGE) $(TEMP_DIR)/sf 0 3  0 244
#
	mkdir $(TEMP_DIR)/block
	$(MKDIRRANGE) $(TEMP_DIR)/block 0 5  234 10
#
	mkdir $(TEMP_DIR)/leaf
	$(MKDIRRANGE) $(TEMP_DIR)/leaf 0 40  214 30
#
	mkdir $(TEMP_DIR)/node
	$(MKDIRRANGE) $(TEMP_DIR)/node 0 1100  0 23
#
	mkdir $(TEMP_DIR)/btree_leaf
	$(MKDIRRANGE) $(TEMP_DIR)/btree_leaf 0 1000  201 43
#
	mkdir $(TEMP_DIR)/btree_leaf_free
	$(MKDIRRANGE) $(TEMP_DIR)/btree_leaf_free 0 1200  201 43
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	sudo losetup -b 4096 /dev/loop8 $@
	sudo parted --script /dev/loop8 mktable gpt
	sudo parted --script --align optimal /dev/loop8 mkpart primary 1MiB 100%
	sudo losetup -d /dev/loop8
#	parted --script $@ mktable gpt
#	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_v4_ftype0_s05k_b2k_n8k_xattr.img:
	fallocate -l 63MiB $@
	mkfs.xfs -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n size=8k,ftype=0 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/sf_empty
	setfattr -n user.pew_attr_pew -v pew_value_pew $(TEMP_DIR)/sf_empty
#
	mkdir $(TEMP_DIR)/sf
	setfattr -n user.pew_attr_pew -v pew_value_pew $(TEMP_DIR)/sf
	$(MKDIRRANGE) $(TEMP_DIR)/sf 0 3  0 244
#
	mkdir $(TEMP_DIR)/block
	setfattr -n user.pew_attr_pew -v pew_value_pew $(TEMP_DIR)/block
	$(MKDIRRANGE) $(TEMP_DIR)/block 0 5  234 10
#
	mkdir $(TEMP_DIR)/leaf
	setfattr -n user.pew_attr_pew -v pew_value_pew $(TEMP_DIR)/leaf
	$(MKDIRRANGE) $(TEMP_DIR)/leaf 0 40  214 30
#
	mkdir $(TEMP_DIR)/node
	setfattr -n user.pew_attr_pew -v pew_value_pew $(TEMP_DIR)/node
	$(MKDIRRANGE) $(TEMP_DIR)/node 0 1020  0 23
#
	mkdir $(TEMP_DIR)/btree_leaf
	setfattr -n user.pew_attr_pew -v pew_value_pew $(TEMP_DIR)/btree_leaf
	$(MKDIRRANGE) $(TEMP_DIR)/btree_leaf 0 1000  201 43
#
	mkdir $(TEMP_DIR)/btree_leaf_free
	setfattr -n user.pew_attr_pew -v pew_value_pew $(TEMP_DIR)/btree_leaf_free
	$(MKDIRRANGE) $(TEMP_DIR)/btree_leaf_free 0 1200  201 43
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_v4_unicode.img:
	fallocate -l 63MiB $@
	mkfs.xfs -m crc=0,finobt=0,rmapbt=0,reflink=0 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	mkdir -p $(TEMP_DIR)/dir0
	mkdir -p $(TEMP_DIR)/дир❦/дир11
	mkdir -p $(TEMP_DIR)/❦❦❦/д❦р22
	mkdir -p $(TEMP_DIR)/❦👩❦/
	mkdir -p $(TEMP_DIR)/❦👩❦/👩❦❦/
	mkdir -p $(TEMP_DIR)/❦👩❦/❦👩❦/
	mkdir -p $(TEMP_DIR)/❦👩❦/❦❦👩/
	mkdir $(TEMP_DIR)/дир3/
#
	echo hello_world > $(TEMP_DIR)/dir0/file00
	echo привет❦мир > $(TEMP_DIR)/❦❦❦/д❦р22/❦❦
	echo привет💗мир > $(TEMP_DIR)/❦❦❦/д❦р22/💗💗
	echo привет❦💗мир > $(TEMP_DIR)/дир3/файл33
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_v5_ftype1_s05k_b2k_n8k.img:
	fallocate -l 63MiB $@
	mkfs.xfs -b size=2k -m crc=1,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n size=8k,ftype=1 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/sf_empty
#
	mkdir $(TEMP_DIR)/sf
	$(MKDIRRANGE) $(TEMP_DIR)/sf 0 3  0 244
#
	mkdir $(TEMP_DIR)/block
	$(MKDIRRANGE) $(TEMP_DIR)/block 0 5  234 10
#
	mkdir $(TEMP_DIR)/leaf
	$(MKDIRRANGE) $(TEMP_DIR)/leaf 0 40  214 30
#
	mkdir $(TEMP_DIR)/node
	$(MKDIRRANGE) $(TEMP_DIR)/node 0 1100  0 23
#
	mkdir $(TEMP_DIR)/btree_leaf
	$(MKDIRRANGE) $(TEMP_DIR)/btree_leaf 0 1000  201 43
#
	mkdir $(TEMP_DIR)/btree_leaf_free
	$(MKDIRRANGE) $(TEMP_DIR)/btree_leaf_free 0 1200  201 43
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_v5_files_s05k_b4k_n8k.img:
	fallocate -l 127MiB $@
	mkfs.xfs -b size=4k -m crc=1,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n size=8k,ftype=1 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	$(MKFILEPATTERN) $(TEMP_DIR)/no_hole 0 65536
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_begin 0 65536
	fallocate -p -o 0 -l 16KiB $(TEMP_DIR)/hole_begin
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_middle 0 65536
	fallocate -p -o 32KiB -l 16KiB $(TEMP_DIR)/hole_middle
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_end 0 65536
	fallocate -p -o 48KiB -l 16KiB $(TEMP_DIR)/hole_end
#
	fallocate -l 4KiB $(TEMP_DIR)/btree_l1_no_hole
	for n in $$(seq 1 2000); do \
		fallocate -i -l 4KiB -o 0KiB $(TEMP_DIR)/btree_l1_no_hole ; \
		fallocate -z -l 4KiB -o 0KiB $(TEMP_DIR)/btree_l1_no_hole ; \
	done
	$(MKFILEPATTERN) $(TEMP_DIR)/btree_l1_no_hole 0 8196096
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

fat32_test0.img:
	fallocate -l 64MiB $@
	mkfs.fat -n KOLIBRIOS -F 32 $@
	sudo mount -o codepage=866,iocharset=utf8,umask=111,dmask=000 $@ $(TEMP_DIR)
	$(RANDDIR) $(TEMP_DIR) 1000 8 255 65536
	$(DIRTOTEST) $(TEMP_DIR) fat32_test0.img hd0 > ../test/045_#f70_#fat32_test0.t
	tree $(TEMP_DIR)
	du -sh $(TEMP_DIR)
	sudo umount $(TEMP_DIR)

exfat_s05k_c16k_b16k.img:
	fallocate -l 256MiB $@
	mkfs.exfat -L KOLIBRIOS -c 16k -b 16k $@
	sudo mount -o codepage=866,iocharset=utf8,umask=111,dmask=000 $@ $(TEMP_DIR)
	mkdir $(TEMP_DIR)/dir_0
	mkdir $(TEMP_DIR)/dir_1
	touch $(TEMP_DIR)/dir_1/file000
	mkdir $(TEMP_DIR)/dir_1000
	$(MKDIRRANGE) $(TEMP_DIR)/dir_1000 0 1000  201 43
	mkdir $(TEMP_DIR)/dir_10000
	$(MKDIRRANGE) $(TEMP_DIR)/dir_10000 0 10000  201 43
#	mkdir $(TEMP_DIR)/dir_100000
#	$(MKDIRRANGE) $(TEMP_DIR)/dir_100000 0 100000  201 43
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable msdos
	parted --script --align optimal $@ mkpart primary 1MiB 100%

exfat_s05k_c8k_b8k.img:
	fallocate -l 256MiB $@
	mkfs.exfat -L KOLIBRIOS -c 8k -b 8k $@
	sudo mount -o codepage=866,iocharset=utf8,umask=111,dmask=000 $@ $(TEMP_DIR)
	mkdir $(TEMP_DIR)/dir_000
	echo -n '' > $(TEMP_DIR)/dir_000/file_000
	mkdir $(TEMP_DIR)/dir_001
	echo -n 'x' > $(TEMP_DIR)/dir_001/file_001
	mkdir $(TEMP_DIR)/dir_002
	echo -n 'x' > $(TEMP_DIR)/dir_002/file_002
	mkdir $(TEMP_DIR)/dir_003
	echo -n 'x' > $(TEMP_DIR)/dir_003/file_003
	mkdir $(TEMP_DIR)/dir_004
	echo -n 'x' > $(TEMP_DIR)/dir_004/file_004
	mkdir $(TEMP_DIR)/dir_005
	echo -n 'x' > $(TEMP_DIR)/dir_005/file_005
	mkdir $(TEMP_DIR)/dir_006
	echo -n 'x' > $(TEMP_DIR)/dir_006/file_006
	mkdir $(TEMP_DIR)/dir_007
	echo -n 'x' > $(TEMP_DIR)/dir_007/file_007
	mkdir $(TEMP_DIR)/dir_008
	echo -n 'x' > $(TEMP_DIR)/dir_008/file_008
	mkdir $(TEMP_DIR)/dir_009
	echo -n 'x' > $(TEMP_DIR)/dir_009/file_009
	mkdir $(TEMP_DIR)/dir_010
	echo -n 'x' > $(TEMP_DIR)/dir_010/file_010
	mkdir $(TEMP_DIR)/dir_011
	echo -n 'x' > $(TEMP_DIR)/dir_011/file_011
	mkdir $(TEMP_DIR)/dir_012
	echo -n 'x' > $(TEMP_DIR)/dir_012/file_012
	mkdir $(TEMP_DIR)/dir_013
	echo -n 'x' > $(TEMP_DIR)/dir_013/file_013
	mkdir $(TEMP_DIR)/dir_014
	echo -n 'x' > $(TEMP_DIR)/dir_014/file_014
	mkdir $(TEMP_DIR)/dir_015
	echo -n 'x' > $(TEMP_DIR)/dir_015/file_015
	mkdir $(TEMP_DIR)/dir_016
	echo -n 'x' > $(TEMP_DIR)/dir_016/file_016
	mkdir $(TEMP_DIR)/dir_017
	echo -n 'x' > $(TEMP_DIR)/dir_017/file_017
	mkdir $(TEMP_DIR)/dir_018
	echo -n 'x' > $(TEMP_DIR)/dir_018/file_018
	mkdir $(TEMP_DIR)/dir_019
	echo -n 'x' > $(TEMP_DIR)/dir_019/file_019
	mkdir $(TEMP_DIR)/dir_020
	echo -n 'x' > $(TEMP_DIR)/dir_020/file_020
	mkdir $(TEMP_DIR)/dir_021
	echo -n 'x' > $(TEMP_DIR)/dir_021/file_021
	mkdir $(TEMP_DIR)/dir_022
	echo -n 'x' > $(TEMP_DIR)/dir_022/file_022
	mkdir $(TEMP_DIR)/dir_023
	echo -n 'x' > $(TEMP_DIR)/dir_023/file_023
	mkdir $(TEMP_DIR)/dir_024
	echo -n 'x' > $(TEMP_DIR)/dir_024/file_024
	mkdir $(TEMP_DIR)/dir_025
	echo -n 'x' > $(TEMP_DIR)/dir_025/file_025
	mkdir $(TEMP_DIR)/dir_026
	echo -n 'x' > $(TEMP_DIR)/dir_026/file_026
	mkdir $(TEMP_DIR)/dir_027
	echo -n 'x' > $(TEMP_DIR)/dir_027/file_027
	mkdir $(TEMP_DIR)/dir_028
	echo -n 'x' > $(TEMP_DIR)/dir_028/file_028
	mkdir $(TEMP_DIR)/dir_029
	echo -n 'x' > $(TEMP_DIR)/dir_029/file_029
	mkdir $(TEMP_DIR)/dir_030
	echo -n 'x' > $(TEMP_DIR)/dir_030/file_030
	mkdir $(TEMP_DIR)/dir_031
	echo -n 'x' > $(TEMP_DIR)/dir_031/file_031
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@
	parted --script $@ mktable msdos
	parted --script --align optimal $@ mkpart primary 1MiB 100%

