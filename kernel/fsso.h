#pragma once
#include <stdint.h>

#define FSSO_MAGIC 0x4653534F5F5631
#define FSSO_BLOCK_SIZE 1024
#define FSSO_MAX_FILENAME 255
#define FSSO_FIRST_DATA_BLOCK 11

typedef struct {
	uint64_t magic;
	uint32_t block_size;
	uint32_t total_blocks;
	uint32_t total_inodes;
	uint32_t free_blocks;
	uint32_t free_inodes;
	uint32_t first_data_blockl;
	char fs_name[8];
	uint32_t version;
	uint32_t reserved[12];
} __attribute__((packed)) FSSO_Superblock;

#define FSSO_DIRECT_BLOCKS 12
#define FSSO_TYPE_FILE 1
#define FSSO_TYPE_DIR 2


typedef struct {
	uint32_t size;
	uint32_t created;
	uint32_t modified;
	uint8_t  type;
	uint8_t  reserved[3];
	uint32_t direct[FSSO_DIRECT_BLOCKS];
	uint32_t indirect;
	uint32_t double_indirect;
	uint32_t triple_indirect;
} __attribute__((packed)) FSSO_Inode;

typedef struct {
	uint32_t inode;
	char name[FSSO_MAX_FILENAME + 1];
} __attribute__((packed)) FSSO_DirEntry;

typedef struct {
	FSSO_Superblock superblock;
	uint32_t disk_start_block;
} FSSO_Filesystem;
