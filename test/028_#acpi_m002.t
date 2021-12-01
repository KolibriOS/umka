umka_init
pci_set_path machines/002/pci

# some _ADR's are methods with memory access
# don't know how to trick here
acpi_set_usage 1

acpi_preload_table machines/002/acpi/DSDT
acpi_preload_table machines/002/acpi/SSDT1
acpi_preload_table machines/002/acpi/SSDT2
acpi_preload_table machines/002/acpi/SSDT3
acpi_preload_table machines/002/acpi/SSDT4
acpi_preload_table machines/002/acpi/SSDT5
acpi_preload_table machines/002/acpi/SSDT6
acpi_preload_table machines/002/acpi/SSDT7
acpi_preload_table machines/002/acpi/SSDT8
acpi_preload_table machines/002/acpi/SSDT9
acpi_preload_table machines/002/acpi/SSDT10
acpi_preload_table machines/002/acpi/SSDT11
acpi_preload_table machines/002/acpi/SSDT12

acpi_enable

acpi_get_node_alloc_cnt
acpi_get_node_free_cnt
acpi_get_node_cnt