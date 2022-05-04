umka_init
pci_set_path machines/011/pci
acpi_set_usage 2

acpi_preload_table machines/011/acpi/dsdt.dat
acpi_preload_table machines/011/acpi/ssdt1.dat
acpi_preload_table machines/011/acpi/ssdt2.dat

acpi_enable

acpi_get_node_alloc_cnt
acpi_get_node_free_cnt
acpi_get_node_cnt
