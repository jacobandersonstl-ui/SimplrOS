#pragma once
#include <stdint.h>

#define AHCI_BASE 0x400000000

#define AHCI_PORT_BASE   0x100
#define AHCI_PORT_SIZE   0x80

#define PORT_CLB    0x00
#define PORT_CLBU   0x04
#define PORT_FB     0x08
#define PORT_FBU    0x0C
#define PORT_IS     0x10
#define PORT_IE     0x14
#define PORT_CMD    0x18
#define PORT_TFD    0x20
#define PORT_SIG    0x24
#define PORT_SSTS   0x28
#define PORT_SERR   0x30
#define PORT_CI     0x38

#define SATA_SIG_ATA   0x00000101
#define SATA_SIG_ATAPI 0xEB140101

typedef struct {
	uint32_t clb;
	uint32_t clbu;
	uint32_t fb;
	uint32_t fbu;
	uint32_t is;
	uint32_t ie;
	uint32_t cmd;
	uint32_t reserved0;
	uint32_t tfd;
	uint32_t sig;
	uint32_t ssts;
	uint32_t sctl;
	uint32_t serr;
	uint32_t sact;
	uint32_t ci;
	uint32_t sntf;
	uint32_t fbs;
	uint32_t reserved1[11];
	uint32_t vendor[4];
} __attribute__((packed)) HBA_PORT;

typedef struct {
	uint32_t cap;
	uint32_t ghc;
	uint32_t is;
	uint32_t pi;
	uint32_t vs;
	uint32_t ccc_ctl;
	uint32_t ccc_pts;
	uint32_t em_loc;
	uint32_t em_ctl;
	uint32_t cap2;
	uint32_t bohc;
	uint8_t  reserved[0xA0-0x2C];
	uint8_t  vendor[0x100-0xA0];
	HBA_PORT ports[32];
} __attribute__((packed)) HBA_MEM;

int ahci_init(void);
int ahci_read(uint64_t lba, uint32_t count, uint8_t *buffer);
