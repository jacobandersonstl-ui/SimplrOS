#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Include our filesystem definition
#include "../kernel/fsso.h"

#define TOTAL_BLOCKS 4096
#define TOTAL_INODES 512

int fsso_add_file(uint8_t *disk, const char *name, uint8_t *data, uint32_t size) {
	printf("DEBUG: fsso_add_file called with name='%s' size=%d\n", name, size);
	FSSO_Superblock *sb = (FSSO_Superblock *)disk;
	uint8_t *inode_bitmap = disk + FSSO_BLOCK_SIZE;
	uint8_t *block_bitmap = disk + (FSSO_BLOCK_SIZE * 2);
	FSSO_Inode *inode_table = (FSSO_Inode *)(disk + (FSSO_BLOCK_SIZE * 3));

	// Find a free inode
	uint32_t free_inode = 0;
	for (uint32_t i = 1; i < sb->total_inodes; i++) {
		uint32_t byte = i / 8;
		uint32_t bit = i % 8;
		if (!(inode_bitmap[byte] & (1 << bit))) {
			free_inode = i;
			inode_bitmap[byte] |= (1 << bit);
			break;
		}
	}

	if (free_inode == 0) {
		printf("Error: no free inodes\n");
		return -1;
	}

// Find free blocks and write data
	uint32_t blocks_needed = (size + FSSO_BLOCK_SIZE - 1) / FSSO_BLOCK_SIZE;
	uint32_t allocated_blocks[FSSO_DIRECT_BLOCKS] = {0};

	uint32_t blocks_found = 0;
	for (uint32_t i = FSSO_FIRST_DATA_BLOCK; i < sb->total_blocks && blocks_found < blocks_needed; i++) {
		uint32_t byte = i / 8;
		uint32_t bit = i % 8;
		if (!(block_bitmap[byte] & (1 << bit))) {
			block_bitmap[byte] |= (1 << bit);
			allocated_blocks[blocks_found] = i;
			blocks_found++;
		}
	}

	if (blocks_found < blocks_needed) {
		printf("Error: not enough free blocks\n");
		return -1;
	}

	// Write file data to allocated blocks
	for (uint32_t i = 0; i < blocks_found; i++) {
		uint8_t *block = disk + (allocated_blocks[i] * FSSO_BLOCK_SIZE);
		uint32_t offset = i * FSSO_BLOCK_SIZE;
		uint32_t to_copy = size - offset;
		if (to_copy > FSSO_BLOCK_SIZE) to_copy = FSSO_BLOCK_SIZE;
		for (uint32_t j = 0; j < to_copy; j++) {
			block[j] = data[offset + j];
		}
	}

// Write the inode
	FSSO_Inode *inode = &inode_table[free_inode];
	inode->size = size;
	inode->type = FSSO_TYPE_FILE;
	inode->created = 0;
	inode->modified = 0;
	for (uint32_t i = 0; i < blocks_found; i++) {
		inode->direct[i] = allocated_blocks[i];
	}

	// Add entry to root directory
	FSSO_Inode *root = &inode_table[0];
	uint8_t *root_block = disk + (root->direct[0] * FSSO_BLOCK_SIZE);
	FSSO_DirEntry *entries = (FSSO_DirEntry *)root_block;

	printf("DEBUG: root direct[0] = %d\n", root->direct[0]);

	// Find empty slot in root directory
	uint32_t entries_per_block = FSSO_BLOCK_SIZE / sizeof(FSSO_DirEntry);
	for (uint32_t i = 0; i < entries_per_block; i++) {
		if (entries[i].inode == 0) {
			entries[i].inode = free_inode;
			// Copy name manually
			uint32_t j = 0;
			while (name[j] && j < FSSO_MAX_FILENAME) {
				entries[i].name[j] = name[j];
				j++;
			}
			entries[i].name[j] = '\0';
			printf("DEBUG: wrote entry inode=%d name=%s at slot %d\n",
				entries[i].inode, entries[i].name, i);
			printf("Debug: Copied %d chars, name[0]=%c entry.name[0]=%c\n",
				j, name[0], entries[i].name[0]);
			sb->free_inodes--;
			sb->free_blocks -= blocks_found;
			return 0;
		}
	}
	printf("Error: root directory full\n");
	return -1;
}

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

	// Add a test file
        char *test_data = "Hello from FSSO!\n";
        if (fsso_add_file(disk, "hello.txt", (uint8_t *)test_data, 18) != 0) {
                printf("Error: failed to add test file\n");
                free(disk);
                fclose(f);
                return 1;
        }
        printf("Test file written\n");

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
