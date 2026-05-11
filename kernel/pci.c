#include "pci.h"

static inline void outl(uint16_t port, uint32_t value) {
	__asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
	uint32_t value;
	__asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
	uint32_t address = (uint32_t)((bus << 16) | (slot << 11) |
		(func << 8) | (offset & 0xFC) | 0x80000000);
	outl(PCI_CONFIG_ADDRESS, address);
	return inl(PCI_CONFIG_DATA);
}

uint64_t pci_find_ahci(void) {
	for (uint16_t bus = 0; bus < 256; bus++) {
		for (uint8_t slot = 0; slot < 32; slot++) {
			uint32_t vendor = pci_read(bus, slot, 0, 0);
			if ((vendor & 0xFFFF) == 0xFFFF) continue;

			uint32_t class = pci_read(bus, slot, 0, 8);
			uint8_t class_code = (class >> 24) & 0xFF;
			uint8_t subclass = (class >> 16) & 0xFF;

			if (class_code == 0x01 && subclass == 0x06) {
				uint32_t bar5 = pci_read(bus, slot, 0, 0x24);
				return (uint64_t)(bar5 & 0xFFFFFFF0);
			}
		}
	}
	return 0;
}
