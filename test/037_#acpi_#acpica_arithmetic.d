acpi_set_usage 1

acpi_preload_table /data/work/mirror/acpica/tests/aslts/tmp/aml/20201217/nopt/64/arithmetic.aml

acpi_enable

acpi_get_node_alloc_cnt
acpi_get_node_free_cnt
acpi_get_node_cnt

acpi_call \MAIN

acpi_get_node_alloc_cnt
acpi_get_node_free_cnt
acpi_get_node_cnt
