umka_init
acpi_set_usage 1

acpi_preload_table machines/unit/acpi/debug_object.aml

acpi_enable

acpi_get_node_alloc_cnt
acpi_get_node_free_cnt
acpi_get_node_cnt

acpi_call \MAIN

acpi_get_node_alloc_cnt
acpi_get_node_free_cnt
acpi_get_node_cnt