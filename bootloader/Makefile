ARCH = x86_64
CC = gcc
LD = ld

EFIINC = /usr/include/efi
EFIINCS = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
EFILIB = /usr/lib

EFI_CRT_OBJS = $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS = $(EFILIB)/elf_$(ARCH)_efi.lds

CFLAGS = $(EFIINCS) -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -DEFI_FUNCTION_WRAPPER

LDFLAGS = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L$(EFILIB) -lefi -lgnuefi

all: bootloader.efi

bootloader.o: bootloader.c
	$(CC) $(CFLAGS) -c -o bootloader.o bootloader.c

bootloader.so: bootloader.o
	$(LD) $(LDFLAGS) $(EFI_CRT_OBJS) bootloader.o -o bootloader.so

bootloader.efi: bootloader.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
		-j .rela -j .reloc --target=efi-app-$(ARCH) bootloader.so bootloader.efi

clean:
	rm -f *.o *.so *.efi
