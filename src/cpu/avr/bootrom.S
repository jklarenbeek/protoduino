; file: ./src/cpu/avr/bootrom.S
;
; The far calls of ./src/sys/bootrom.h, for parts with more than 128 KB of
; flash. A function pointer has 16 bits there and reaches the first 128 KB;
; the boot section lies above. Each routine puts the word address of the
; service on the stack and returns to it, so the service returns to whoever
; called the routine.
;
; - EIND is not touched.
; - The address is the last argument, so the arguments of the service are
;   already in the registers the service expects them in.

#include <avr/io.h>

#if defined(__AVR_3_BYTE_PC__)

; ptstate_t bootrom_far_load_thread(struct elf_load *self, uint32_t word_address)
;   self: r24-r25, word_address: r20-r23
  .section .text.bootrom_far_load_thread,"ax",@progbits
  .global bootrom_far_load_thread
  .type bootrom_far_load_thread, @function
bootrom_far_load_thread:
  push r20
  push r21
  push r22
  ret
  .size bootrom_far_load_thread, .-bootrom_far_load_thread

; void bootrom_far_page_write(bootrom_addr_t base, const uint8_t *data, uint32_t word_address)
;   base: r22-r25, data: r20-r21, word_address: r16-r19
  .section .text.bootrom_far_page_write,"ax",@progbits
  .global bootrom_far_page_write
  .type bootrom_far_page_write, @function
bootrom_far_page_write:
  push r16
  push r17
  push r18
  ret
  .size bootrom_far_page_write, .-bootrom_far_page_write

#endif
