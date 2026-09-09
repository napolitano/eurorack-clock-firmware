/*
 * STM32F401CC clock firmware linker layout.
 *
 * Sector 0: firmware vectors / optional low-flash sections (16 KiB)
 * Sector 1: persistence slot A (16 KiB, never linked)
 * Sector 2: persistence slot B (16 KiB, never linked)
 * Sectors 3..5: main firmware image (208 KiB)
 *
 * Total firmware-owned Flash: 224 KiB. Persistent data is isolated in two
 * independently erasable sectors so one committed generation always survives.
 */

ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x400;

MEMORY
{
  RAM         (xrw) : ORIGIN = 0x20000000, LENGTH = LD_MAX_DATA_SIZE
  FLASH_BOOT  (rx)  : ORIGIN = 0x08000000, LENGTH = 16K
  PERSIST_A   (r)   : ORIGIN = 0x08004000, LENGTH = 16K
  PERSIST_B   (r)   : ORIGIN = 0x08008000, LENGTH = 16K
  FLASH_APP   (rx)  : ORIGIN = 0x0800C000, LENGTH = 208K
}

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH_BOOT

  /* Explicit opt-in area for code/assets that may use the remaining sector-0
   * headroom if the main 208-KiB region ever approaches capacity. */
  .flash_boot :
  {
    . = ALIGN(4);
    *(.flash_boot)
    *(.flash_boot.*)
    . = ALIGN(4);
  } > FLASH_BOOT

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
    _etext = .;
  } > FLASH_APP

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH_APP

  .ARM.extab (READONLY) :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } > FLASH_APP

  .ARM (READONLY) :
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } > FLASH_APP

  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
    . = ALIGN(4);
  } > FLASH_APP

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
    . = ALIGN(4);
  } > FLASH_APP

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
    . = ALIGN(4);
  } > FLASH_APP

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH_APP

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } > RAM

  .noinit (NOLOAD) :
  {
    . = ALIGN(4);
    _snoinit = .;
    *(.noinit)
    *(.noinit*)
    . = ALIGN(4);
    _enoinit = .;
  } > RAM

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM

  /DISCARD/ :
  {
    libc.a(*)
    libm.a(*)
    libgcc.a(*)
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}

ASSERT(SIZEOF(.isr_vector) + SIZEOF(.flash_boot) <= LENGTH(FLASH_BOOT),
       "sector 0 overflow: move fewer sections to .flash_boot")
ASSERT(ADDR(.text) >= ORIGIN(FLASH_APP), "firmware overlaps persistence sectors")
