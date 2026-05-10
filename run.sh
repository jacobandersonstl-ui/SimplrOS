#!/bin/bash
set -e

echo "Building bootloader..."
cd ~/SimplrOS/edk2
build -a X64 -t GCC -p SimplrOsPkg/SimplrOsPkg.dsc

echo "Building kernel..."
cd ~/SimplrOS/kernel
gcc -ffreestanding -fno-stack-protector -fno-pic \
    -mno-red-zone -nostdlib -nodefaultlibs \
    -static -T linker.ld -o kernel.elf kernel.c

echo "Building disk image..."
cd ~/SimplrOS
cp edk2/Build/SimplrOS/DEBUG_GCC/X64/Bootloader.efi iso/EFI/BOOT/BOOTX64.EFI
cp kernel/kernel.elf iso/kernel.elf
dd if=/dev/zero of=simplros.img bs=1M count=64 2>/dev/null
mkfs.fat -F 32 simplros.img
mmd -i simplros.img ::/EFI
mmd -i simplros.img ::/EFI/BOOT
mcopy -i simplros.img iso/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
mcopy -i simplros.img iso/kernel.elf ::/kernel.elf

echo "Launching QEMU..."
qemu-system-x86_64 \
  -bios /usr/share/ovmf/OVMF.fd \
  -drive format=raw,file=simplros.img \
  -net none
