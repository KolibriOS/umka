/*
MEMORY
{
  pew : ORIGIN = 0, LENGTH = 64K
}

SECTIONS
{
  .aou 0:
  {
    *(.app)
  } : NONE

}
INSERT BEFORE .interp;
*/

SECTIONS
{
  .data.boot BLOCK(0x1000) :
  {
    *(.data.bt)
  }

}
INSERT BEFORE .data;

SECTIONS
{
  .data.aligned BLOCK(0x10000) :
  {
    *(SORT_BY_NAME(.data.align*))
  }

}
INSERT AFTER .data;

SECTIONS
{
  .bss.aligned BLOCK(0x10000) :
  {
    *(SORT_BY_NAME(.bss.align*))
  }
}
INSERT AFTER .bss;
