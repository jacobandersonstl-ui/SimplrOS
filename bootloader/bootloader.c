// SimplrOS Bootloader
// This is the first code that runs after UEFI hands control to us.
// Our only job right now: tell the screen we exist.

#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    
    // Initialize the UEFI library so we can use its helper functions
    InitializeLib(ImageHandle, SystemTable);

    // Print to the UEFI console
    Print(L"SimplrOS bootloader starting...\n");
    Print(L"Hello from Simplr!\n");

    // Hang here forever for now — we have nowhere to jump to yet
    while(1) {}

    return EFI_SUCCESS;
}