pci_set_path machines/012/pci
acpi_set_usage 2

acpi_preload_table machines/012/acpi/dsdt.dat
acpi_preload_table machines/012/acpi/ssdt1.dat
acpi_preload_table machines/012/acpi/ssdt2.dat
acpi_preload_table machines/012/acpi/ssdt3.dat
acpi_preload_table machines/012/acpi/ssdt4.dat
acpi_preload_table machines/012/acpi/ssdt5.dat
acpi_preload_table machines/012/acpi/ssdt6.dat

acpi_enable

acpi_get_node_alloc_cnt
acpi_get_node_free_cnt
acpi_get_node_cnt
