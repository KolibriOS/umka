MKDIRRANGE=../tools/mkdirrange
MKFILEPATTERN=../tools/mkfilepattern
MOUNT_OPT=-t xfs
TEMP_DIR:=$(shell mktemp -d)

all: xfs_v4_ftype0_s05k_b2k_n8k.img xfs_v4_ftype1_s05k_b2k_n8k.img xfs_v4_xattr.img xfs_v4_files_s05k_b4k_n8k.img xfs_v4_deep_btree_dirs.img
	rmdir $(TEMP_DIR)

xfs_v4_ftype0_s05k_b2k_n8k.img:
	fallocate -l 63MiB $@
	sudo mkfs.xfs -b size=2k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n size=8k,ftype=0 $@
	sudo mount $(MOUNT_OPT) $@ $(TEMP_DIR)
	sudo chown $$USER $(TEMP_DIR) -R
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
	$(MKDIRRANGE) $(TEMP_DIR)/btree 0 293181  0 244
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

