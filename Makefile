# toolchain
CC=x86_64-elf-gcc
AS=nasm
LD=x86_64-elf-ld
CC_FLAGS=-ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 
USERCC_FLAGS=-ffreestanding -nostdlib -nostartfiles 
CC_FLAGS+=-g -O0
LD_FLAGS=-ffreestanding -nostdlib -lgcc -mcmodel=kernel -g
TARGET=x86_64
SCRIPT_FOLDER=scripts/




CRTI_OBJ=build/crti.asm.o
CRTBEGIN_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
CRTN_OBJ=build/crtn.asm.o


KERNEL_CSRC=$(shell cd src; find ./kernel/ -type f -name "*.c")


LIBC_CSRC=$(shell cd src; find ./libc/ -type f -name "*.c")
KERNEL_CSRC+=$(LIBC_CSRC)
KERNEL_ASMSRC=$(shell cd src; find ./kernel/ -type f -name "*.asm")
USER_CSRC=$(shell cd src; find ./user/lib/ -type f -name "*.c")
USER_PROGSRC=$(shell cd src; find ./user/prog/ -type f -name "*.c")


KERNEL_OBJ=$(patsubst ./%.c, build/%.c.o, $(KERNEL_CSRC))
KERNEL_OBJ+=$(patsubst ./%.asm, build/%.asm.o, $(KERNEL_ASMSRC))
LIBC_OBJ=$(patsubst ./%.c, build/%.c.o, $(LIBC_CSRC))
USER_OBJ=$(patsubst ./%.c, build/%.c.o, $(USER_CSRC))
USER_PROG=$(patsubst ./%.c, build/%, $(USER_PROGSRC))

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


USER_INCLUDES=\
-I./src/user/lib/ \



# SCRIPTS
GRUB_CHECK_SCRIPT=$(SCRIPT_FOLDER)/grub_file_check.sh


test:
	echo $(USER_CSRC)
	echo $(USER_OBJ)



#build/%.c.o : kernel/arch/$(TARGET)/%.c
#	$(CC) $(CC_FLAGS) $(KERNEL_INCLUDES) $< -c -o $@ 

build/kernel/%.c.o: src/kernel/%.c
	mkdir -p "$(@D)"	# this nifty command will create the directory we need in the build folder
	$(CC) $(CC_FLAGS) $(KERNEL_INCLUDES) $< -c -o $@ 


build/libc/%.c.o: src/libc/%.c
	mkdir -p "$(@D)"	# this nifty command will create the directory we need in the build folder
	$(CC) $(CC_FLAGS) $(KERNEL_INCLUDES) $< -c -o $@ 


build/user/lib/%.c.o: src/user/lib/%.c
	mkdir -p "$(@D)"	# this nifty command will create the directory we need in the build folder
	$(CC) $(USERCC_FLAGS) $(USER_INCLUDES) $< -c -o $@ 

build/user/prog/%: src/user/prog/%.c $(USER_OBJ) $(LIBC_OBJ)
	mkdir -p "$(@D)"
	$(CC) $(USERCC_FLAGS) $(USER_INCLUDES) $^ -o $@ -e main




# -g is add debug symbols
build/%.asm.o : src/%.asm
	mkdir -p "$(@D)"	# this nifty command will create the directory we need in the build folder
	$(AS) -g -f elf64 $< -o $@ 






myos.bin: $(KERNEL_OBJ) $(USER_OBJ)
	$(CC) -T src/kernel/linker.ld $(KERNEL_OBJ) -o myos.bin $(LD_FLAGS)
	$(GRUB_CHECK_SCRIPT)

myos.iso: myos.bin
	mkdir -p kernel/isodir/boot/grub
	cp myos.bin src/kernel/isodir/boot/myos.bin
	cp src/kernel/grub.cfg src/kernel/isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso src/kernel/isodir


build-disk : test clean myos.iso $(USER_PROG) 
	./scripts/mkdisk.sh
	
# 
qemu: build-disk
	sudo qemu-system-x86_64 -enable-kvm -cpu host -D out.txt -serial file:out.log -m 8G -smp 2 -drive format=raw,file=./build/disk.img
# -d int -no-reboot
debug: build-disk
	qemu-system-x86_64 -enable-kvm -cpu host -s -S -serial file:out.log -m 8G -smp 1 -drive format=raw,file=./build/disk.img | grep exception

clean:
	# beginning line with a hyphen tells make to ignore errors
	-rm -rf build/* \
	myos.iso \
	myos.bin \
	kernel/isodir/boot/myos.bin \
	out.log \