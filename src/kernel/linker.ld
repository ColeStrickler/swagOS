/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(start)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	. = 1M;
	KERNEL_START = .;
    KERNEL_HH_VA_OFFSET = 0xffffffff80000000;
	
	
	/* First put the multiboot header, as it is required to be put very early
	   in the image or the bootloader won't recognize the file format.
	   Next we'll put the .text section. */

	.boot :
	{
		*(.multiboot)
	}

	. += KERNEL_HH_VA_OFFSET;
		.text ALIGN(4K) : AT (ADDR (.text) - KERNEL_HH_VA_OFFSET)
		{
			*(.text)
		}
	
		/* Read-only data. */
		.rodata ALIGN(4K) : AT (ADDR (.rodata) - KERNEL_HH_VA_OFFSET)
		{
			*(.rodata)
		}
	
		/* Read-write data (initialized) */
		.data ALIGN(4K) : AT (ADDR (.data) - KERNEL_HH_VA_OFFSET)
		{
			*(.data)
		}
	
		/* Read-write data (uninitialized) and stack */
		.bss ALIGN(4K) : AT (ADDR (.bss) - KERNEL_HH_VA_OFFSET)
		{
			*(COMMON)
			*(.bss)
		}
	
	/* The compiler may produce other sections, by default it will put them in
	   a segment with the same name. Simply add stuff here as needed. */

	KERNEL_END = . - KERNEL_HH_VA_OFFSET;
}