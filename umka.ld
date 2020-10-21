SECTIONS
{
  . = ALIGN(65536);
  .data.aligned :
  {
    *(SORT_BY_NAME(.data.align*))
  }
}
INSERT AFTER .data;
