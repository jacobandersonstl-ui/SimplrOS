#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <Protocol/GraphicsOutput.h>

typedef struct {
	UINT8 e_ident[16];
	UINT16 e_type;
	UINT16 e_machine;
	UINT32 e_version;
	UINT64 e_entry;
	UINT64 e_phoff;
	UINT64 e_shoff;
	UINT32 e_flags;
	UINT16 e_ehsize;
	UINT16 e_phentsize;
	UINT16 e_phnum;
	UINT16 e_shentsize;
	UINT16 e_shnum;
	UINT16 e_shstrndx;
} Elf64_Ehdr;

EFI_STATUS
EFIAPI
efi_main (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  Print(L"SimplrOS bootloader starting...\n");
  Print(L"Hello from Simplr!\n");

  EFI_STATUS Status;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;

  Status = gBS->HandleProtocol(
  	ImageHandle,
  	&gEfiLoadedImageProtocolGuid,
  	(VOID **)&LoadedImage
  );
  if (EFI_ERROR(Status)) {
  	Print(L"Failed to get loaded image protocol\n");
  	return Status;
  }

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  Status = gBS->HandleProtocol(
  	LoadedImage->DeviceHandle,
  	&gEfiSimpleFileSystemProtocolGuid,
  	(VOID **)&FileSystem
  );
  if (EFI_ERROR(Status)) {
  	Print(L"Failed to get filesystem protocol\n");
  	return Status;
  }

  EFI_FILE_PROTOCOL *Root;
  Status = FileSystem->OpenVolume(
	FileSystem,
	&Root
  );
  if (EFI_ERROR(Status)) {
	Print(L"Failed to open volume\n");
	return Status;
  }

  EFI_FILE_PROTOCOL *KernelFile;
  Status = Root->Open(
	Root,
	&KernelFile,
	L"kernel.elf",
	EFI_FILE_MODE_READ,
	0
  );
  if (EFI_ERROR(Status)) {
	Print(L"Failed to open kernel.elf\n");
	return Status;
  }

  UINTN FileInfoSize = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 256;
  EFI_FILE_INFO *FileInfo = AllocatePool(FileInfoSize);
  Status = KernelFile->GetInfo(
      KernelFile,
      &gEfiFileInfoGuid,
      &FileInfoSize,
      FileInfo
  );
  if (EFI_ERROR(Status)) {
      Print(L"Failed to get kernel file info\n");
      return Status;
  }
  UINTN KernelSize = FileInfo->FileSize;
  FreePool(FileInfo);

  VOID *KernelBuffer = AllocatePool(KernelSize);
  if (KernelBuffer == NULL) {
	Print(L"Failed to allocate memory for kernel\n");
	return EFI_OUT_OF_RESOURCES;
  }

  Status = KernelFile->Read(KernelFile, &KernelSize, KernelBuffer);
  if (EFI_ERROR(Status)) {
	Print(L"Failed to read kernel\n");
	return Status;
  }
  KernelFile->Close(KernelFile);

  typedef struct {
      UINT32 p_type;
      UINT32 p_flags;
      UINT64 p_offset;
      UINT64 p_vaddr;
      UINT64 p_paddr;
      UINT64 p_filesz;
      UINT64 p_memsz;
      UINT64 p_align;
  } Elf64_Phdr;

  Elf64_Ehdr *ElfHeader = (Elf64_Ehdr *)KernelBuffer;

  if (ElfHeader->e_ident[0] != 0x7f ||
	ElfHeader->e_ident[1] != 'E'  ||
      ElfHeader->e_ident[2] != 'L'  ||
      ElfHeader->e_ident[3] != 'F') {
      Print(L"kernel.elf is not a valid ELF file\n");
      return EFI_LOAD_ERROR;
  }

  Print(L"Kernel ELF valid. Entry point: 0x%lx\n", ElfHeader->e_entry);
  for (UINT16 i = 0; i < ElfHeader->e_phnum; i++) {
      Elf64_Phdr *phdr = (Elf64_Phdr *)((UINT8 *)KernelBuffer + ElfHeader->e_phoff + i * ElfHeader->e_phentsize);
      if (phdr->p_type == 1) {
          UINT8 *dest = (UINT8 *)phdr->p_vaddr;
          UINT8 *src  = (UINT8 *)KernelBuffer + phdr->p_offset;
          for (UINTN j = 0; j < phdr->p_filesz; j++)
              dest[j] = src[j];
          for (UINTN j = phdr->p_filesz; j < phdr->p_memsz; j++)
              dest[j] = 0;
      }
  }
  // 1. GOP first
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  Status = gBS->LocateProtocol(
      &gEfiGraphicsOutputProtocolGuid,
      NULL,
      (VOID **)&Gop
  );
  if (!EFI_ERROR(Status)) {
      Gop->SetMode(Gop, 0);
  }

  // 2. Debug print
  Print(L"Bytes at 0x100000: %02x %02x %02x %02x\n",
      ((UINT8*)0x100000)[0],
      ((UINT8*)0x100000)[1],
      ((UINT8*)0x100000)[2],
      ((UINT8*)0x100000)[3]);

  // 3. BootInfo
  typedef struct {
      UINT64 framebuffer;
      UINT32 width;
      UINT32 height;
      UINT32 pixels_per_scanline;
  } BootInfo;
  BootInfo info;
  info.framebuffer = Gop->Mode->FrameBufferBase;
  info.width = Gop->Mode->Info->HorizontalResolution;
  info.height = Gop->Mode->Info->VerticalResolution;
  info.pixels_per_scanline = Gop->Mode->Info->PixelsPerScanLine;

  // 4. KernelEntry
  typedef void (*KernelEntry)(BootInfo *);
  KernelEntry KernelStart = (KernelEntry)(ElfHeader->e_entry);

  // 5. Memory map + ExitBootServices
  UINTN MemoryMapSize = 0;
  EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;
  UINTN MapKey = 0;
  UINTN DescriptorSize = 0;
  UINT32 DescriptorVersion = 0;
  gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  MemoryMapSize += 2 * DescriptorSize;
  MemoryMap = AllocatePool(MemoryMapSize);
  Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (EFI_ERROR(Status)) {
      Print(L"Failed to get memory map\n");
      return Status;
  }
  Status = gBS->ExitBootServices(ImageHandle, MapKey);
  if (EFI_ERROR(Status)) {
      Print(L"Failed to exit boot services\n");
      return Status;
  }

  // 6. Jump
  KernelStart(&info);
  while(1) {}
  return EFI_SUCCESS;
}
