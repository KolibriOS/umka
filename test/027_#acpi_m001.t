pci_set_path machines/001/pci
acpi_set_usage 2

acpi_preload_table machines/001/acpi/DSDT
acpi_preload_table machines/001/acpi/SSDT1
acpi_preload_table machines/001/acpi/SSDT2

acpi_enable

acpi_get_node_alloc_cnt
acpi_get_node_free_cnt
acpi_get_node_cnt
