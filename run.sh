#!/bin/bash
set -e

echo "Building bootloader..."
cd ~/SimplrOS/edk2
source edksetup.sh
export GCC_BIN=/usr/bin/
build -a X64 -t GCC -p SimplrOsPkg/SimplrOsPkg.dsc

echo "Building kernel..."
cd ~/SimplrOS/kernel
gcc -ffreestanding -fno-stack-protector -fno-pic \
    -mno-red-zone -nostdlib -nodefaultlibs \
    -static -T linker.ld -o kernel.elf \
    kernel.c ahci.c pci.c

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

echo "Rebuilding FSSO image..."
~/SimplrOS/tools/mkfsso ~/SimplrOS/fsso.img

echo "Launching QEMU..."
qemu-system-x86_64 \
  -bios /usr/share/ovmf/OVMF.fd \
  -drive id=disk0,format=raw,file=simplros.img,if=none \
  -drive id=disk1,format=raw,file=fsso.img,if=none \
  -device ahci,id=ahci \
  -device ide-hd,drive=disk0,bus=ahci.0 \
  -device ide-hd,drive=disk1,bus=ahci.1 \
  -net none
