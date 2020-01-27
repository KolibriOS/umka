MKDIRRANGE=../tools/mkdirrange
MKFILEPATTERN=../tools/mkfilepattern
MOUNT_OPT=-t xfs
TEMP_DIR:=$(shell mktemp -d)

all: s05k s4k unicode v5
	rmdir $(TEMP_DIR)

s05k: xfs_v4_ftype0_s05k_b2k_n8k.img xfs_v4_ftype1_s05k_b2k_n8k.img xfs_v4_xattr.img xfs_v4_files_s05k_b4k_n8k.img xfs_v4_ftype0_s05k_b2k_n8k_xattr.img

s4k: xfs_v4_ftype0_s4k_b4k_n8k.img

unicode: xfs_v4_unicode.img

v5: xfs_v5_ftype1_s05k_b2k_n8k.img xfs_v5_files_s05k_b4k_n8k.img

xfs_v4_ftype0_s05k_b2k_n8k.img:
	fallocate -l 63MiB $@
	sudo mkfs.xfs -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n size=8k,ftype=0 $@
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
	sudo mkfs.xfs -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n size=8k,ftype=1 $@
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
	sudo mkfs.xfs -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n ftype=0 $@
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

xfs_v4_deep_btree_dirs.img:
	fallocate -l 127MiB $@
	sudo mkfs.xfs -b size=4k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n size=4k,ftype=1 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/btree
#	$(MKDIRRANGE) $(TEMP_DIR)/btree 0 293181  0 244
	$(MKDIRRANGE) $(TEMP_DIR)/btree 0 193181  0 244
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@ 
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_v4_files_s05k_b4k_n8k.img:
	fallocate -l 127MiB $@
	sudo mkfs.xfs -b size=4k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n size=8k,ftype=0 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	$(MKFILEPATTERN) $(TEMP_DIR)/no_hole 65536
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_begin 65536
	fallocate -p -o 0 -l 16KiB $(TEMP_DIR)/hole_begin
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_middle 65536
	fallocate -p -o 32KiB -l 16KiB $(TEMP_DIR)/hole_middle
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_end 65536
	fallocate -p -o 48KiB -l 16KiB $(TEMP_DIR)/hole_end
#
	fallocate -l 4KiB $(TEMP_DIR)/btree_l1_no_hole
	for n in $$(seq 1 2000); do \
		fallocate -i -l 4KiB -o 0KiB $(TEMP_DIR)/btree_l1_no_hole ; \
		fallocate -z -l 4KiB -o 0KiB $(TEMP_DIR)/btree_l1_no_hole ; \
	done
	$(MKFILEPATTERN) $(TEMP_DIR)/btree_l1_no_hole 8196096
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@ 
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%

xfs_v4_ftype0_s4k_b4k_n8k.img:
	fallocate -l 63MiB $@
	sudo mkfs.xfs -b size=4k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=4k -i maxpct=100 -n size=8k,ftype=0 $@
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
	sudo mkfs.xfs -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n size=8k,ftype=0 $@
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
	sudo mkfs.xfs -m crc=0,finobt=0,rmapbt=0,reflink=0 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	mkdir -p $(TEMP_DIR)/dir0
	mkdir -p $(TEMP_DIR)/дир❦/дир11
	mkdir -p $(TEMP_DIR)/❦❦❦/д❦р22
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
	sudo mkfs.xfs -b size=2k -m crc=1,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n size=8k,ftype=1 $@
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
	sudo mkfs.xfs -b size=4k -m crc=1,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n size=8k,ftype=1 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
#
	$(MKFILEPATTERN) $(TEMP_DIR)/no_hole 65536
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_begin 65536
	fallocate -p -o 0 -l 16KiB $(TEMP_DIR)/hole_begin
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_middle 65536
	fallocate -p -o 32KiB -l 16KiB $(TEMP_DIR)/hole_middle
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_end 65536
	fallocate -p -o 48KiB -l 16KiB $(TEMP_DIR)/hole_end
#
	fallocate -l 4KiB $(TEMP_DIR)/btree_l1_no_hole
	for n in $$(seq 1 2000); do \
		fallocate -i -l 4KiB -o 0KiB $(TEMP_DIR)/btree_l1_no_hole ; \
		fallocate -z -l 4KiB -o 0KiB $(TEMP_DIR)/btree_l1_no_hole ; \
	done
	$(MKFILEPATTERN) $(TEMP_DIR)/btree_l1_no_hole 8196096
#
	sudo umount $(TEMP_DIR)
	fallocate -i -o 0 -l 1MiB $@ 
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%


