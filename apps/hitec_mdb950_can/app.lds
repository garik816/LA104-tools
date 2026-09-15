MEMORY
{
  rom (rx) : ORIGIN = 0x0801a000, LENGTH = 16K
  ram (rwx) : ORIGIN = 0x20004800, LENGTH = 16K
  null (rwx): ORIGIN = 0x00001000, LENGTH = 1K
}

SECTIONS
{
  .text : { *(.entry) *(.text*) *(.rodata*) } >rom
  .data : { *(.data*) } >ram AT >rom
  .bss : { *(.bss*) } >ram
  .rel.plt : { *(.rel.plt) } > rom
  .plt : { *(.plt) } > rom
  .got : { *(.got.plt) *(.got) } > ram
  .dynsym : { *(.dynsym) } > null
  .dynstr : { *(.dynstr) } > null
}
