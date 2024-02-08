# toolchain
CC=x86_64-elf-gcc
AS=nasm
LD=x86_64-elf-ld
CC_FLAGS=-std=gnu99 -ffreestanding -O2 -Wall -Wextra
LD_FLAGS=-ffreestanding -O2 -nostdlib -lgcc -n


# KERNEL PARTS
KERNEL_CSRC=$(shell cd kernel; find -name "*.c")
KERNEL_ASMSRC=$(shell cd kernel; find -name "*.asm")

KERNEL_OBJ=$(patsubst %.c, build/%.c.o, $(KERNEL_CSRC))
KERNEL_OBJ+=$(patsubst %.asm, build/%.asm.o, $(KERNEL_ASMSRC))



# SCRIPTS
GRUB_CHECK_SCRIPT=kernel/grub_file_check.sh


test:
	echo $(KERNEL_ASMSRC)
	echo $(KERNEL_CSRC)
	echo $(KERNEL_OBJ)



build/%.asm.o : kernel/%.asm
	$(AS) -f elf64 $< -o $@


build/%.c.o: kernel/%.c
	$(CC) $(CC_FLAGS) $< -c -o $@ 


myos.bin: $(KERNEL_OBJ)
	$(CC) -T kernel/linker.ld $(KERNEL_OBJ) -o myos.bin $(LD_FLAGS)
	$(GRUB_CHECK_SCRIPT)

myos.iso : myos.bin
	mkdir -p kernel/isodir/boot/grub
	cp myos.bin kernel/isodir/boot/myos.bin
	cp kernel/grub.cfg kernel/isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso kernel/isodir


qemu: myos.iso
	qemu-system-x86_64 -cdrom myos.iso

clean:
	# beginning line with a hyphen tells make to ignore errors
	-rm build/* myos.iso myos.bin kernel/isodir/boot/myos.bin