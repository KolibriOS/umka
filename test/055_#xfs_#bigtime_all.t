umka_init shell
disk_add ../img/xfs_bigtime.qcow2 hd0 -c 0
ls70 /hd0/1/
ls70 /hd0/1/dira
stat70 /hd0/1/dira -am
stat70 /hd0/1/dirb -am
stat70 /hd0/1/dirc -am
stat70 /hd0/1/dird -am
stat70 /hd0/1/dire -am
stat70 /hd0/1/dirf -am
disk_del hd0
