umka_boot
pci_set_path machines/007/pci
acpi_set_usage 2

acpi_preload_table machines/007/acpi/DSDT
acpi_preload_table machines/007/acpi/SSDT

acpi_enable
board_get -f

acpi_get_node_alloc_cnt
acpi_get_node_free_cnt
acpi_get_node_cnt
