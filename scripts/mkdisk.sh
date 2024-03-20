#/bin/bash
dd if=/dev/zero of=disk.img bs=512 count=131072
fdisk disk.img <<EOF
n
p
1


a
w
EOF
sudo losetup /dev/loop30 disk.img
sudo losetup /dev/loop31 disk.img -o 1048576
sudo mke2fs /dev/loop31
sudo umount /mnt
sudo mount /dev/loop31 /mnt
sudo grub-install --root-directory=/mnt --no-floppy --modules="normal part_msdos ext2 multiboot biosdisk" /dev/loop30