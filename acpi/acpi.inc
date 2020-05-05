uglobal
        sci_int dd ?
        sci_cmd dw ?
        acpi_enable db ?
        acpi_disable db ?
        PM1a_CNT_BLK dw ?
        PM1b_CNT_BLK dw ?
        acpi_ssdt_base dd ?
        acpi_ssdt_size dd ?
        acpi_ssdt_cnt dd ?
endg

enable_acpi:
ret
