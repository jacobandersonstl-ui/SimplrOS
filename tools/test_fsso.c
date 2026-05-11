#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../kernel/fsso.h"
#include "../kernel/fsso.c"

// This is the memory-mapped disk the stub reads from
uint8_t *fsso_disk;

int main(void) {
	// Load fsso.img into memory
	FILE *f = fopen("fsso.img", "rb");
	if (f == NULL) {
		printf("Error: could not open fsso.img\n");
		return 1;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	fsso_disk = malloc(size);
	fread(fsso_disk, 1, size, f);
	fclose(f);

	printf("Loaded %ld bytes from fsso.img\n", size);

	// Raw dump of block 11
	uint8_t *block11 = fsso_disk + (11 * FSSO_BLOCK_SIZE);
	printf("Block 11 first 32 bytes: ");
	for (int i = 0; i < 32; i++) {
		printf("%02x ", block11[i]);
	}
	printf("\n");

	// Try to mount
	FSSO_Filesystem fs;
	if (fsso_mount(&fs) != 0) {
		printf("Failed to mount FSSO filesystem\n");
		return 1;
	}

	printf("Mounted successfully!\n");
	printf("Total blocks: %d\n", fs.superblock.total_blocks);
	printf("Total inodes: %d\n", fs.superblock.total_inodes);
	printf("Free blocks: %d\n", fs.superblock.free_blocks);
	printf("Version: %d\n", fs.superblock.version);

	// Debug: print all root directory entries
	uint8_t dir_block[FSSO_BLOCK_SIZE];
	FSSO_Inode root_inode;
	fsso_read_inode(&fs, 0, &root_inode);
	printf("Root inode type: %d, direct[0]: %d\n", root_inode.type, root_inode.direct[0]);

	// Debug: dump raw directory entries
	extern uint8_t *fsso_disk;
	uint8_t *block = fsso_disk + (11 * FSSO_BLOCK_SIZE);
	FSSO_DirEntry *entries = (FSSO_DirEntry *)block;
	printf("First dir entry: inode=%d name=%s\n", entries[0].inode, entries[0].name);

	// Try to find and read hello.txt
	uint32_t inode_num;
	if (fsso_find_in_dir(&fs, 0, "hello.txt", &inode_num) != 0) {
		printf("Error: could not find hello.txt\n");
		free(fsso_disk);
		return 1;
	}

	printf("Found hello.txt at inode %d\n", inode_num);

	uint8_t file_buffer[1024];
	int bytes = fsso_read_file(&fs, inode_num, file_buffer, sizeof(file_buffer));
	if (bytes < 0) {
		printf("Error: could not read hello.txt\n");
		free(fsso_disk);
		return 1;
	}

	file_buffer[bytes] = '\0';
	printf("File contents: %s\n", (char *)file_buffer);

	free(fsso_disk);
	return 0;
}
