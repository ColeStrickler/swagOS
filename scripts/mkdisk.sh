#/bin/bash


# Function to detach loop devices and unmount mount points
cleanup() {
    # Unmount filesystems
    if mount | grep -q "/mnt"; then
        sudo umount /mnt
    fi

    if mount | grep -q "/mnt/iso"; then
        sudo umount /mnt/iso
    fi

    # Detach loop devices
    if losetup -a | grep -q "/dev/loop30"; then
        sudo losetup -d /dev/loop30
    fi

    if losetup -a | grep -q "/dev/loop31"; then
        sudo losetup -d /dev/loop31
    fi
}


# Trap cleanup function on exit or error
trap cleanup EXIT ERR

# create disk image
dd if=/dev/zero of=disk.img bs=512 count=131072
fdisk disk.img <<EOF
n
p
1


a
w
EOF

# Detach existing loop devices if any
cleanup

sudo losetup /dev/loop30 disk.img
sudo losetup /dev/loop31 disk.img -o 1048576

# Format partition with ext2
sudo mke2fs /dev/loop31

# Mount partition
sudo mount /dev/loop31 /mnt

# Mount ISO and copy its contents
sudo mkdir -p /mnt/iso 
sudo mount -o loop myos.iso /mnt/iso
sudo cp -r /mnt/iso/* /mnt

# Install GRUB
sudo grub-install --root-directory=/mnt --no-floppy --modules="normal part_msdos ext2 multiboot biosdisk" /dev/loop30

sudo chmod 777 disk.img
mv disk.img ./build/disk.img

cleanup

