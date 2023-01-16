umka_init shell
disk_add ../img/gpt_partitions_s05k.qcow2 hd0 -c 524288
disk_del hd0
disk_add ../img/gpt_partitions_s4k.qcow2 hd0 -c 524288
disk_del hd0
disk_add ../img/gpt_large.qcow2 hd0 -c 524288
disk_del hd0
