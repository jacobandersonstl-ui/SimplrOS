#include <string.h>
#include "fsso.h"

// Read a block from disk into buffer
// For now this is a stub - we'll connect it to real disk I/O later
static void read_block(uint32_t block_num, uint8_t *buffer) {
	// TODO: replace with real AHCI disk read
	// For now we'll use a memory-mapped disk for testing
	extern uint8_t *fsso_disk;
	uint8_t *src = fsso_disk + (block_num * FSSO_BLOCK_SIZE);
	for (uint32_t i = 0; i < FSSO_BLOCK_SIZE; i++) {
		buffer[i] = src[i];
	}
}

int fsso_mount(FSSO_Filesystem *fs) {
	uint8_t buffer[FSSO_BLOCK_SIZE];
	read_block(0, buffer);

	FSSO_Superblock *sb = (FSSO_Superblock *)buffer;

	if (sb->magic != FSSO_MAGIC) {
		return -1;
	}

	fs->superblock = *sb;
	fs->disk_start_block = 0;

	return 0;
}

int fsso_read_inode(FSSO_Filesystem *fs, uint32_t inode_num, FSSO_Inode *out) {
	if (inode_num >= fs->superblock.total_inodes) {
		return -1;
	}

	uint32_t inodes_per_block = FSSO_BLOCK_SIZE / sizeof(FSSO_Inode);
	uint32_t block_num = 3 + (inode_num / inodes_per_block);
	uint32_t offset = inode_num % inodes_per_block;

	uint8_t buffer[FSSO_BLOCK_SIZE];
	read_block(block_num, buffer);

	FSSO_Inode *table = (FSSO_Inode *)buffer;
	*out = table[offset];

	return 0;
}

int fsso_find_in_dir(FSSO_Filesystem *fs, uint32_t dir_inode, const char *name, uint32_t *out_inode) {
	FSSO_Inode inode;
	if (fsso_read_inode(fs, dir_inode, &inode) != 0) {
		return -1;
	}

	if (inode.type != FSSO_TYPE_DIR) {
		return -2;
	}

	uint8_t buffer[FSSO_BLOCK_SIZE];
	uint32_t entries_per_block = FSSO_BLOCK_SIZE / sizeof(FSSO_DirEntry);

	for (int i = 0; i < FSSO_DIRECT_BLOCKS; i++) {
		if (inode.direct[i] == 0) break;

		read_block(inode.direct[i], buffer);
		FSSO_DirEntry *entries = (FSSO_DirEntry *)buffer;

		for (uint32_t j = 0; j < entries_per_block; j++) {
			if (entries[j].inode == 0) continue;
			if (strcmp(entries[j].name, name) == 0) {
				*out_inode = entries[j].inode;
				return 0;
			}
		}
	}

	return -3;
}

int fsso_resolve_path(FSSO_Filesystem *fs, const char *path, uint32_t *out_inode) {
	char component[FSSO_MAX_FILENAME + 1];
	uint32_t current_inode = 0;
	const char *p = path;

	while (*p != '\0') {
		// Extract next component
		int i = 0;
		while (*p != '.' && *p != '\0') {
			if (i < FSSO_MAX_FILENAME) {
				component[i++] = *p;
			}
			p++;
		}
		component[i] = '\0';

		if (*p == '.') p++;

		if (i == 0) continue;

		uint32_t next_inode;
		if (fsso_find_in_dir(fs, current_inode, component, &next_inode) != 0) {
			return -1;
		}
		current_inode = next_inode;
	}

	*out_inode = current_inode;
	return 0;
}

int fsso_read_file(FSSO_Filesystem *fs, uint32_t inode_num, uint8_t *out_buffer, uint32_t max_size) {
	FSSO_Inode inode;
	if (fsso_read_inode(fs, inode_num, &inode) != 0) {
		return -1;
	}

	if (inode.type != FSSO_TYPE_FILE) {
		return -2;
	}

	uint32_t bytes_read = 0;
	uint8_t block_buffer[FSSO_BLOCK_SIZE];

	for (int i = 0; i < FSSO_DIRECT_BLOCKS; i++) {
		if (inode.direct[i] == 0) break;
		if (bytes_read >= max_size) break;

		read_block(inode.direct[i], block_buffer);

		uint32_t to_copy = FSSO_BLOCK_SIZE;
		if (bytes_read + to_copy > max_size) {
			to_copy = max_size - bytes_read;
		}

		for (uint32_t j = 0; j < to_copy; j++) {
			out_buffer[bytes_read + j] = block_buffer[j];
		}

		bytes_read += to_copy;
	}

	return (int)bytes_read;
}


