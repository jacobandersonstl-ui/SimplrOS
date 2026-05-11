#include <stddef.h>
#include "ahci.h"
#include "ahci_structs.h"
#include "pci.h"

static HBA_MEM *hba = NULL;
static int active_port = -1;

static uint32_t port_read(int port, uint32_t offset) {
	volatile uint32_t *reg = (volatile uint32_t *)((uint8_t *)&hba->ports[port] + offset);
	return *reg;
}

static void port_write(int port, uint32_t offset, uint32_t value) {
	volatile uint32_t *reg = (volatile uint32_t *)((uint8_t *)&hba->ports[port] + offset);
	*reg = value;
}

static void port_rebase(int port) {
	uint32_t cmd = port_read(port, PORT_CMD);
	cmd &= ~0x01;
	cmd &= ~0x10;
	port_write(port, PORT_CMD, cmd);
	while ((port_read(port, PORT_CMD) & 0xC000) != 0) {}

	uint32_t clb = 0x100000 + (port * 0x400);
	port_write(port, PORT_CLB,  clb);
	port_write(port, PORT_CLBU, 0);

	uint32_t fb = 0x108000 + (port * 0x100);
	port_write(port, PORT_FB,  fb);
	port_write(port, PORT_FBU, 0);

	port_write(port, PORT_SERR, 0xFFFFFFFF);
	port_write(port, PORT_IS,   0xFFFFFFFF);

	while (port_read(port, PORT_CMD) & 0x8000) {}
	cmd = port_read(port, PORT_CMD);
	cmd |= 0x10;
	cmd |= 0x01;
	port_write(port, PORT_CMD, cmd);
}

int ahci_init(void) {
	uint64_t ahci_base = pci_find_ahci();
	if (ahci_base == 0) return -1;
	hba = (HBA_MEM *)ahci_base;

	hba->ghc |= (1 << 31);

	uint32_t pi = hba->pi;
	for (int i = 0; i < 32; i++) {
		if (!(pi & (1 << i))) continue;
		uint32_t ssts = port_read(i, PORT_SSTS);
		uint8_t det = ssts & 0x0F;
		uint8_t ipm = (ssts >> 8) & 0x0F;
		if (det == 3 && ipm == 1) {
			uint32_t sig = port_read(i, PORT_SIG);
			if (sig == SATA_SIG_ATA) {
				active_port = i;
			}
		}
	}

	if (active_port == -1) return -1;

	port_rebase(active_port);
	return 0;
}

int ahci_read(uint64_t lba, uint32_t count, uint8_t *buffer) {
	if (active_port == -1) return -1;

	uint32_t clb = 0x100000 + (active_port * 0x400);
	HBA_CMD_HEADER *cmd_header = (HBA_CMD_HEADER *)(uint64_t)clb;

	uint32_t ctba = 0x110000 + (active_port * 0x100);
	cmd_header->cfl    = sizeof(FIS_REG_H2D) / 4;
	cmd_header->w      = 0;
	cmd_header->prdtl  = 1;
	cmd_header->ctba   = ctba;
	cmd_header->ctbau  = 0;
	cmd_header->prdbc  = 0;

	HBA_CMD_TBL *cmd_tbl = (HBA_CMD_TBL *)(uint64_t)ctba;
	uint8_t *p = (uint8_t *)cmd_tbl;
	for (int i = 0; i < sizeof(HBA_CMD_TBL); i++) p[i] = 0;

	cmd_tbl->prdt_entry[0].dba  = (uint32_t)(uint64_t)buffer;
	cmd_tbl->prdt_entry[0].dbau = 0;
	cmd_tbl->prdt_entry[0].dbc  = (count * 512) - 1;
	cmd_tbl->prdt_entry[0].i    = 1;

	FIS_REG_H2D *fis = (FIS_REG_H2D *)cmd_tbl->cfis;
	fis->fis_type = 0x27;
	fis->c        = 1;
	fis->command  = 0x25;
	fis->device   = 1 << 6;

	fis->lba0 = (uint8_t)(lba);
	fis->lba1 = (uint8_t)(lba >> 8);
	fis->lba2 = (uint8_t)(lba >> 16);
	fis->lba3 = (uint8_t)(lba >> 24);
	fis->lba4 = (uint8_t)(lba >> 32);
	fis->lba5 = (uint8_t)(lba >> 40);
	fis->count = (uint16_t)count;

	port_write(active_port, PORT_IS, 0xFFFFFFFF);
	port_write(active_port, PORT_CI, 1);

	for (int timeout = 0; timeout < 1000000; timeout++) {
		if (!(port_read(active_port, PORT_CI) & 1)) break;
		if (port_read(active_port, PORT_IS) & (1 << 30)) return -1;
	}

	if (port_read(active_port, PORT_CI) & 1) return -2;

	return 0;
}
