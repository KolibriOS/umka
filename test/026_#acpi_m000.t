pci_set_path machines/000/pci
acpi_set_usage 2

acpi_preload_table machines/000/acpi/DSDT
acpi_preload_table machines/000/acpi/SSDT1
acpi_preload_table machines/000/acpi/SSDT2
acpi_preload_table machines/000/acpi/SSDT3
acpi_preload_table machines/000/acpi/SSDT4
acpi_preload_table machines/000/acpi/SSDT5
acpi_preload_table machines/000/acpi/SSDT6

acpi_enable
