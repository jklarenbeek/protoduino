/* pd-module.ld - layout of a loadable protoduino module.
 * The descriptor comes first, then code and constants, as one flash segment.
 * A module owns no static RAM: its state lives in the process struct that the
 * kernel hands to its thread, so any .data/.bss is a build error. */
SECTIONS
{
  .text 0x6000 : { KEEP(*(.pd.module)) . = ALIGN(2); *(.text*) *(.progmem*) *(.rodata*) }
  .ram 0x800100 (NOLOAD) : { *(.data*) *(.bss*) *(COMMON) }
  ASSERT(SIZEOF(.ram) == 0, "module has static variables: keep state in the process struct")
}
