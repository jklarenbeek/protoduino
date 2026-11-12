/* file: ./ld/module.ld
 *
 * Layout of a loadable protoduino module (see ./src/sys/module.h).
 *
 * The descriptor comes first, then code and constants, as one flash segment
 * that starts at the slot the module is linked for:
 *
 *   -Wl,--defsym=__module_base=0x6000        the slot
 *   -Wl,--defsym=__module_kernel_id=0x....   the kernel build
 *
 * A module owns no static RAM: its state lives in the process struct that
 * the kernel hands to its thread, so any .data or .bss is a build error.
 */
SECTIONS
{
  .text __module_base :
  {
    KEEP(*(.module))
    . = ALIGN(2);
    *(.text*)
    *(.progmem*)
    *(.rodata*)
  }
  .ram 0x800100 (NOLOAD) :
  {
    *(.data*)
    *(.bss*)
    *(COMMON)
  }
  ASSERT(SIZEOF(.ram) == 0, "module has static variables: keep state in the process struct")
}
