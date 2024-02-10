# toolchain
CC=x86_64-elf-gcc
AS=nasm
LD=x86_64-elf-ld
CC_FLAGS=-ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 
# debug flag
CC_FLAGS+=-g -O0
LD_FLAGS=-ffreestanding -nostdlib -lgcc -mcmodel=kernel -g
TARGET=x86_64


CRTI_OBJ=build/crti.asm.o
CRTBEGIN_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
CRTN_OBJ=build/crtn.asm.o


# KERNEL PARTS
KERNEL_CSRC=$(shell cd kernel; find . -name "*.c" -exec basename {} \;)
KERNEL_CSRC+=$(shell cd libc; find . -name "*.c" -exec basename {} \;)
KERNEL_ASMSRC=$(shell cd kernel; find . -name "*.asm" -exec basename {} \;)

KERNEL_OBJ=$(patsubst %.c, build/%.c.o, $(KERNEL_CSRC))
KERNEL_OBJ+=$(patsubst %.asm, build/%.asm.o, $(KERNEL_ASMSRC))



KERNEL_INCLUDES=\
-I./kernel/include/ \
-I./kernel/arch/$(TARGET)/ \


# SCRIPTS
GRUB_CHECK_SCRIPT=kernel/grub_file_check.sh


test:
	echo $(KERNEL_ASMSRC)
	echo $(KERNEL_CSRC)
	echo $(KERNEL_OBJ)
	echo $(KERNEL)


build/%.c.o : kernel/arch/x86_64/%.c
	$(CC) $(CC_FLAGS) $(KERNEL_INCLUDES) $< -c -o $@ 

build/%.c.o : libc/%.c
	$(CC) $(CC_FLAGS) $(KERNEL_INCLUDES) $< -c -o $@ 


# -g is add debug symbols
build/%.asm.o : kernel/%.asm
	$(AS) -g -f elf64 $< -o $@ 



build/%.c.o: kernel/%.c
	$(CC) $(CC_FLAGS) $(KERNEL_INCLUDES) $< -c -o $@ 


myos.bin: $(KERNEL_OBJ)
	$(CC) -T kernel/linker.ld $(KERNEL_OBJ) -o myos.bin $(LD_FLAGS)
	$(GRUB_CHECK_SCRIPT)

myos.iso : myos.bin
	mkdir -p kernel/isodir/boot/grub
	cp myos.bin kernel/isodir/boot/myos.bin
	cp kernel/grub.cfg kernel/isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso kernel/isodir


qemu: clean myos.iso
	qemu-system-x86_64 -serial file:out.log -cdrom myos.iso

debug: clean myos.iso
	qemu-system-x86_64 -s -S -cdrom myos.iso

clean:
	# beginning line with a hyphen tells make to ignore errors
	-rm build/* \
	myos.iso \
	myos.bin \
	kernel/isodir/boot/myos.bin \
	out.log \