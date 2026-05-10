#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Include our filesystem definition
#include "../kernel/fsso.h"

#define TOTAL_BLOCKS 4096
#define TOTAL_INODES 512

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage: mkfsso <output_file>\n");
		return 1;
	}
	FILE *f = fopen(argv[1], "wb");
	if (f == NULL) {
		printf("Error: Could not open %s for writing\n", argv[1]);
		return 1;
	}

	uint8_t *disk = calloc(TOTAL_BLOCKS, FSSO_BLOCK_SIZE);
	if (disk == NULL) {
		printf("Error: Not Enough Memory\n");
		fclose(f);
		return 1;
	}

	printf("Formatting %s as FSSO...\n", argv[1]);

	FSSO_Superblock *sb = (FSSO_Superblock *)disk;
	sb->magic = FSSO_MAGIC;
	sb->block_size = FSSO_BLOCK_SIZE;
	sb->total_blocks = TOTAL_BLOCKS;
	sb->total_inodes = TOTAL_INODES;
	sb->free_blocks = TOTAL_BLOCKS - FSSO_FIRST_DATA_BLOCK;
	sb->free_inodes = TOTAL_INODES - 1;
	sb->first_data_blockl = FSSO_FIRST_DATA_BLOCK;
	sb->version = 1;
	memcpy(sb->fs_name, "FSSO    ", 8);

	printf("Superblock written\n");

	uint8_t *inode_bitmap = disk + FSSO_BLOCK_SIZE;
	inode_bitmap[0] = 0x01;

	uint8_t *block_bitmap = disk + (FSSO_BLOCK_SIZE * 2);
	block_bitmap[0] = 0xFF;
	block_bitmap[1] = 0x07;

	printf("Bitmaps Written\n");

	FSSO_Inode *inode_table =  (FSSO_Inode *)(disk + (FSSO_BLOCK_SIZE * 3));
	FSSO_Inode *root = &inode_table[0];
	root->size = 0;
	root->created = 0;
	root->modified = 0;
	root->type = FSSO_TYPE_DIR;
	root->direct[0] = FSSO_FIRST_DATA_BLOCK;

	block_bitmap[1] |= 0x08;

	printf("Root directory inode written\n");

	// Write entire disk image to file
	size_t written = fwrite(disk, FSSO_BLOCK_SIZE, TOTAL_BLOCKS, f);
	if (written != TOTAL_BLOCKS) {
		printf("Error: only wrote %zu of %d blocks\n", written, TOTAL_BLOCKS);
		free(disk);
		fclose(f);
		return 1;
	}

	free(disk);
	fclose(f);

	printf("FSSO filesystem created successfully!\n");
	printf("Total blocks: %d\n", TOTAL_BLOCKS);
	printf("Total inodes: %d\n", TOTAL_INODES);
	printf("Block size: %d bytes\n", FSSO_BLOCK_SIZE);
	printf("Total size: %d KB\n", (TOTAL_BLOCKS * FSSO_BLOCK_SIZE) / 1024);

	return 0;
}
