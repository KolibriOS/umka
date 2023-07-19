umka_boot
acpi_set_usage 1

acpi_preload_table machines/unit/acpi/scope_empty.aml
acpi_preload_table machines/unit/acpi/scope_spec1.aml
acpi_preload_table machines/unit/acpi/scope.aml

acpi_enable
board_get -f

acpi_get_node_alloc_cnt
acpi_get_node_free_cnt
acpi_get_node_cnt
