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

	free(fsso_disk);
	return 0;
}
