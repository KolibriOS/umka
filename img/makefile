MKDIRRANGE=../tools/mkdirrange
LOOP_DEV=/dev/loop8
TEMP_DIR:=$(shell mktemp -d)

all: s512_xfs_v4_ftype0.img s512_xfs_v4_ftype1.img s512_xfs_v4_ftype1_btree_dirs.img

s512_xfs_v4_ftype0.img:
	dd if=/dev/zero of=$@ bs=1M count=64
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%
	sudo losetup $(LOOP_DEV) --partscan $@
	sudo mkfs.xfs -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n ftype=0 $(LOOP_DEV)p1
	sudo mount $(LOOP_DEV)p1 $(TEMP_DIR)
	sudo chown $(shell whoami) $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/sf
	$(MKDIRRANGE) $(TEMP_DIR)/sf 3
#
	mkdir $(TEMP_DIR)/block
	$(MKDIRRANGE) $(TEMP_DIR)/block 20
#
	mkdir $(TEMP_DIR)/leaf
	$(MKDIRRANGE) $(TEMP_DIR)/leaf 70
#
	mkdir $(TEMP_DIR)/node
	$(MKDIRRANGE) $(TEMP_DIR)/node 520
#
	mkdir $(TEMP_DIR)/btree
	$(MKDIRRANGE) $(TEMP_DIR)/btree 600

	sudo umount $(TEMP_DIR)
	sudo losetup -d $(LOOP_DEV)

s512_xfs_v4_ftype1.img:
	dd if=/dev/zero of=$@ bs=1M count=64
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%
	sudo losetup $(LOOP_DEV) --partscan $@
	sudo mkfs.xfs -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n ftype=1 $(LOOP_DEV)p1
	sudo mount $(LOOP_DEV)p1 $(TEMP_DIR)
	sudo chown $(shell whoami) $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/sf
	$(MKDIRRANGE) $(TEMP_DIR)/sf 3
#
	mkdir $(TEMP_DIR)/block
	$(MKDIRRANGE) $(TEMP_DIR)/block 20
#
	mkdir $(TEMP_DIR)/leaf
	$(MKDIRRANGE) $(TEMP_DIR)/leaf 70
#
	mkdir $(TEMP_DIR)/node
	$(MKDIRRANGE) $(TEMP_DIR)/node 520
#
	mkdir $(TEMP_DIR)/btree
	$(MKDIRRANGE) $(TEMP_DIR)/btree 600

	sudo umount $(TEMP_DIR)
	sudo losetup -d $(LOOP_DEV)

s512_xfs_v4_ftype1_btree_dirs.img:
	dd if=/dev/zero of=$@ bs=1M count=128
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%
	sudo losetup $(LOOP_DEV) --partscan $@
	sudo mkfs.xfs -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n ftype=1 $(LOOP_DEV)p1
	sudo mount $(LOOP_DEV)p1 $(TEMP_DIR)
	sudo chown $(shell whoami) $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/btree
	$(MKDIRRANGE) $(TEMP_DIR)/btree 293181

	sudo umount $(TEMP_DIR)
	sudo losetup -d $(LOOP_DEV)
