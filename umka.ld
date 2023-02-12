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
    *(.data.64)
  }

}
INSERT AFTER .data;

SECTIONS
{
  .bss.aligned BLOCK(0x100000) :
  {
    *(.bss.1M)
  }
}
INSERT AFTER .bss;

SECTIONS
{
  .bss.memory BLOCK(0x1000) :
  {
    *(.bss.mem)
  }
}
INSERT AFTER .bss;
