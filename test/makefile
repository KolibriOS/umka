KOFU=../kofu

all:    t_000_ls_all_dir_types_ftype0 t_001_ls_all_dir_types_ftype1
.PHONY: t_000_ls_all_dir_types_ftype0 t_001_ls_all_dir_types_ftype1

t_000_ls_all_dir_types_ftype0:
	$(KOFU) ../img/s512_xfs_v4_ftype0.img < $@ > $@.out
	cmp $@.ref $@.out

t_001_ls_all_dir_types_ftype1:
	$(KOFU) ../img/s512_xfs_v4_ftype1.img < $@ > $@.out
	cmp $@.ref $@.out
