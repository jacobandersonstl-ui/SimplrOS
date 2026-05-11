#pragma once
#include <stdint.h>

typedef struct {
	uint8_t  cfl:5;
	uint8_t  a:1;
	uint8_t  w:1;
	uint8_t  p:1;
	uint8_t  r:1;
	uint8_t  b:1;
	uint8_t  c:1;
	uint8_t  reserved0:1;
	uint8_t  pmp:4;
	uint16_t prdtl;
	volatile uint32_t prdbc;
	uint32_t ctba;
	uint32_t ctbau;
	uint32_t reserved1[4];
} __attribute__((packed)) HBA_CMD_HEADER;

typedef struct {
	uint32_t dba;
	uint32_t dbau;
	uint32_t reserved0;
	uint32_t dbc:22;
	uint32_t reserved1:9;
	uint32_t i:1;
} __attribute__((packed)) HBA_PRDT_ENTRY;

typedef struct {
	uint8_t  cfis[64];
	uint8_t  acmd[16];
	uint8_t  reserved[48];
	HBA_PRDT_ENTRY prdt_entry[1];
} __attribute__((packed)) HBA_CMD_TBL;

typedef struct {
	uint8_t  fis_type;
	uint8_t  pmport:4;
	uint8_t  reserved0:3;
	uint8_t  c:1;
	uint8_t  command;
	uint8_t  featurel;
	uint8_t  lba0;
	uint8_t  lba1;
	uint8_t  lba2;
	uint8_t  device;
	uint8_t  lba3;
	uint8_t  lba4;
	uint8_t  lba5;
	uint8_t  featureh;
	uint16_t count;
	uint8_t  icc;
	uint8_t  control;
	uint8_t  reserved1[4];
} __attribute__((packed)) FIS_REG_H2D;

