umka_boot
pci_set_path machines/014/pci
acpi_set_usage 1

acpi_preload_table machines/014/acpi/dsdt.dat
acpi_preload_table machines/014/acpi/ssdt.dat
acpi_preload_table machines/014/acpi/ssdt1.dat
acpi_preload_table machines/014/acpi/ssdt2.dat
acpi_preload_table machines/014/acpi/ssdt3.dat
acpi_preload_table machines/014/acpi/ssdt4.dat
acpi_preload_table machines/014/acpi/ssdt5.dat
acpi_preload_table machines/014/acpi/ssdt6.dat
acpi_preload_table machines/014/acpi/ssdt7.dat
acpi_preload_table machines/014/acpi/ssdt8.dat

acpi_enable
board_get -f

acpi_get_node_alloc_cnt
acpi_get_node_free_cnt
acpi_get_node_cnt
