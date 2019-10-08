MKDIRRANGE=../tools/mkdirrange
MKFILEPATTERN=../tools/mkfilepattern
LOOP_DEV=/dev/loop8
TEMP_DIR:=$(shell mktemp -d)

all: s512_xfs_v4_ftype0.img s512_xfs_v4_ftype0_b4k_n2b.img s512_xfs_v4_ftype1.img s512_xfs_v4_ftype0_xattr.img s512_xfs_v4_files.img s512_xfs_v4_files_b4k_n2b.img s512_xfs_v4_ftype1_btree_dirs.img

s512_xfs_v4_ftype0.img:
#	dd if=/dev/zero of=$@ bs=1M count=64
	fallocate -l 64MiB $@
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

s512_xfs_v4_ftype0_b4k_n2b.img:
#	dd if=/dev/zero of=$@ bs=1M count=64
	fallocate -l 64MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%
	sudo losetup $(LOOP_DEV) --partscan $@
	sudo mkfs.xfs -b size=4k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n size=2b,ftype=0 $(LOOP_DEV)p1
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
#	dd if=/dev/zero of=$@ bs=1M count=64
	fallocate -l 64MiB $@
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

s512_xfs_v4_ftype0_xattr.img:
#	dd if=/dev/zero of=$@ bs=1M count=64
	fallocate -l 64MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%
	sudo losetup $(LOOP_DEV) --partscan $@
	sudo mkfs.xfs -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -i maxpct=100 -n ftype=0 $(LOOP_DEV)p1
	sudo mount $(LOOP_DEV)p1 $(TEMP_DIR) -o attr2
	sudo chown $(shell whoami) $(TEMP_DIR) -R
#
	mkdir $(TEMP_DIR)/sf
	$(MKDIRRANGE) $(TEMP_DIR)/sf 900
#
	mkdir $(TEMP_DIR)/leaf
	$(MKDIRRANGE) $(TEMP_DIR)/leaf 600
#
	mkdir $(TEMP_DIR)/node
	$(MKDIRRANGE) $(TEMP_DIR)/node 600
#
	mkdir $(TEMP_DIR)/btree
	$(MKDIRRANGE) $(TEMP_DIR)/btree 600

	sudo umount $(TEMP_DIR)
	sudo losetup -d $(LOOP_DEV)

s512_xfs_v4_ftype1_btree_dirs.img:
#	dd if=/dev/zero of=$@ bs=1M count=128
	fallocate -l 128MiB $@
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

s512_xfs_v4_files.img:
#	dd if=/dev/zero of=$@ bs=1M count=128000
	fallocate -l 128MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%
	sudo losetup $(LOOP_DEV) --partscan $@
	sudo mkfs.xfs -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n ftype=0 $(LOOP_DEV)p1
	sudo mount $(LOOP_DEV)p1 $(TEMP_DIR)
	sudo chown $(shell whoami) $(TEMP_DIR) -R
#
	$(MKFILEPATTERN) $(TEMP_DIR)/no_hole 65536
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_begin 65536
	fallocate -p -o 0 -l 16384 $(TEMP_DIR)/hole_begin
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_middle 65536
	fallocate -p -o 32768 -l 16384 $(TEMP_DIR)/hole_middle
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_end 65536
	fallocate -p -o 49152 -l 16384 $(TEMP_DIR)/hole_end
#
#	fallocate -l 120GiB $(TEMP_DIR)/btree_l1_no_hole
	fallocate -l 4KiB $(TEMP_DIR)/btree_l1_no_hole
#	$(MKFILEPATTERN) $(TEMP_DIR)/btree_l1_no_hole 2097152
#		fallocate -c -l 64KiB -o  $$((64*$$n))KiB $(TEMP_DIR)/btree_l1_no_hole ; \
#	dd if=/dev/zero of=$(TEMP_DIR)/btree_l1_no_hole bs=1M count=128
	for n in $$(seq 1 2000); do \
		fallocate -i -l 4KiB -o 0KiB $(TEMP_DIR)/btree_l1_no_hole ; \
		fallocate -z -l 4KiB -o 0KiB $(TEMP_DIR)/btree_l1_no_hole ; \
	done
#	$(MKFILEPATTERN) $(TEMP_DIR)/btree_l1_no_hole 134217728
#
	sudo umount $(TEMP_DIR)
	sudo losetup -d $(LOOP_DEV)

s512_xfs_v4_files_b4k_n2b.img:
#	dd if=/dev/zero of=$@ bs=1M count=128000
	fallocate -l 128MiB $@
	parted --script $@ mktable gpt
	parted --script --align optimal $@ mkpart primary 1MiB 100%
	sudo losetup $(LOOP_DEV) --partscan $@
	sudo mkfs.xfs -b size=4k -m crc=0,finobt=0,rmapbt=0,reflink=0 -d sectsize=512 -n size=2b,ftype=0 $(LOOP_DEV)p1
	sudo mount $(LOOP_DEV)p1 $(TEMP_DIR)
	sudo chown $(shell whoami) $(TEMP_DIR) -R
#
	$(MKFILEPATTERN) $(TEMP_DIR)/no_hole 65536
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_begin 65536
	fallocate -p -o 0 -l 16384 $(TEMP_DIR)/hole_begin
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_middle 65536
	fallocate -p -o 32768 -l 16384 $(TEMP_DIR)/hole_middle
#
	$(MKFILEPATTERN) $(TEMP_DIR)/hole_end 65536
	fallocate -p -o 49152 -l 16384 $(TEMP_DIR)/hole_end
#
#	fallocate -l 120GiB $(TEMP_DIR)/btree_l1_no_hole
	fallocate -l 4KiB $(TEMP_DIR)/btree_l1_no_hole
#	$(MKFILEPATTERN) $(TEMP_DIR)/btree_l1_no_hole 2097152
#		fallocate -c -l 64KiB -o  $$((64*$$n))KiB $(TEMP_DIR)/btree_l1_no_hole ; \
#	dd if=/dev/zero of=$(TEMP_DIR)/btree_l1_no_hole bs=1M count=128
	for n in $$(seq 1 2000); do \
		fallocate -i -l 4KiB -o 0KiB $(TEMP_DIR)/btree_l1_no_hole ; \
		fallocate -z -l 4KiB -o 0KiB $(TEMP_DIR)/btree_l1_no_hole ; \
	done
#	$(MKFILEPATTERN) $(TEMP_DIR)/btree_l1_no_hole 134217728
#
	sudo umount $(TEMP_DIR)
	sudo losetup -d $(LOOP_DEV)

