KOFU=../kofu

all:    t_000_ls_all_dir_types_ftype0 t_001_ls_all_dir_types_ftype1 t_002_ls_all_dir_types_ftype0_b4k_n2b
.PHONY: t_000_ls_all_dir_types_ftype0 t_001_ls_all_dir_types_ftype1 t_002_ls_all_dir_types_ftype0_b4k_n2b

t_000_ls_all_dir_types_ftype0:
	$(KOFU) ../img/s512_xfs_v4_ftype0.img < $@ > $@.out
	cmp $@.ref $@.out

t_001_ls_all_dir_types_ftype1:
	$(KOFU) ../img/s512_xfs_v4_ftype1.img < $@ > $@.out
	cmp $@.ref $@.out

t_002_ls_all_dir_types_ftype0_b4k_n2b:
	$(KOFU) ../img/s512_xfs_v4_ftype0_b4k_n2b.img < $@ > $@.out
	cmp $@.ref $@.out
