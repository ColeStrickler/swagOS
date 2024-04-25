# toolchain
CC=x86_64-elf-gcc
AS=nasm
LD=x86_64-elf-ld
CC_FLAGS=-ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 
# debug flag
CC_FLAGS+=-g -O0
LD_FLAGS=-ffreestanding -nostdlib -lgcc -mcmodel=kernel -g
TARGET=x86_64
SCRIPT_FOLDER=scripts/



CRTI_OBJ=build/crti.asm.o
CRTBEGIN_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
CRTN_OBJ=build/crtn.asm.o


# KERNEL PARTS
#KERNEL_CSRC=$(shell cd kernel; find . -name "*.c" -exec basename {} \;)
#KERNEL_CSRC+=$(shell cd libc; find . -name "*.c" -exec basename {} \;)
#KERNEL_ASMSRC=$(shell cd kernel; find . -name "*.asm" -exec basename {} \;)
KERNEL_CSRC=$(shell cd src; find . -type f -name "*.c")
KERNEL_ASMSRC=$(shell cd src; find . -type f -name "*.asm")




KERNEL_OBJ=$(patsubst ./%.c, build/%.c.o, $(KERNEL_CSRC))
KERNEL_OBJ+=$(patsubst ./%.asm, build/%.asm.o, $(KERNEL_ASMSRC))



KERNEL_INCLUDES=\
-I./src/kernel/include/ \
-I./src/kernel/include/arch/$(TARGET)/ \
-I./src/kernel/include/sys/ \
-I./src/kernel/include/kernel/ \
-I./src/kernel/include/boot/ \
-I./src/kernel/include/mem/ \
-I./src/kernel/include/data_structures/ \
-I./src/kernel/include/drivers/ \
-I./src/kernel/include/kernel/ \
-I./src/kernel/include/fs/ \



# SCRIPTS
GRUB_CHECK_SCRIPT=$(SCRIPT_FOLDER)/grub_file_check.sh


test:
	echo $(KERNEL_ASMSRC)
	echo $(KERNEL_CSRC)
	echo $(KERNEL_OBJ)
	echo $(KERNEL)


#build/%.c.o : kernel/arch/$(TARGET)/%.c
#	$(CC) $(CC_FLAGS) $(KERNEL_INCLUDES) $< -c -o $@ 

build/%.c.o: src/%.c
	mkdir -p "$(@D)"	# this nifty command will create the directory we need in the build folder
	$(CC) $(CC_FLAGS) $(KERNEL_INCLUDES) $< -c -o $@ 



# -g is add debug symbols
build/%.asm.o : src/%.asm
	mkdir -p "$(@D)"	# this nifty command will create the directory we need in the build folder
	$(AS) -g -f elf64 $< -o $@ 






myos.bin: $(KERNEL_OBJ)
	$(CC) -T src/kernel/linker.ld $(KERNEL_OBJ) -o myos.bin $(LD_FLAGS)
	$(GRUB_CHECK_SCRIPT)

myos.iso: myos.bin
	mkdir -p kernel/isodir/boot/grub
	cp myos.bin src/kernel/isodir/boot/myos.bin
	cp src/kernel/grub.cfg src/kernel/isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso src/kernel/isodir


build-disk : clean myos.iso
	./scripts/mkdisk.sh


qemu: build-disk
	sudo qemu-system-x86_64 -enable-kvm -cpu host -serial file:out.log -m 4G -smp 1 -drive format=raw,file=./build/disk.img

debug: build-disk
	qemu-system-x86_64 -enable-kvm -cpu host -s -S -serial file:out.log -m 4G -smp 8 -drive format=raw,file=./build/disk.img

clean:
	# beginning line with a hyphen tells make to ignore errors
	-rm -rf build/* \
	myos.iso \
	myos.bin \
	kernel/isodir/boot/myos.bin \
	out.log \