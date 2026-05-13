#pragma once
#include "fsso.h"
#include "shell.h"

extern void draw_char(volatile uint32_t *fb, uint32_t pixels_per_scanline,
                      int x, int y, char c, uint32_t color);
extern void draw_string(volatile uint32_t *fb, uint32_t pps,
                        int x, int y, const char *str, uint32_t color);

static void shell_putchar(ShellContext *ctx, char c) {
	if (c == '\n') {
		*ctx->cursor_x = 0;
		*ctx->cursor_y += 10;
		return;
	}
	draw_char(ctx->fb, ctx->pps, *ctx->cursor_x * 9, *ctx->cursor_y, c, 0xFFFFFF);
	(*ctx->cursor_x)++;
	if (*ctx->cursor_x > 80) {
		*ctx->cursor_x = 0;
		*ctx->cursor_y += 10;
	}
}

static void shell_print(ShellContext *ctx, const char *str) {
	for (int i = 0; str[i]; i++) {
		shell_putchar(ctx, str[i]);
	}
}

extern char read_key(void);

static int shell_readline(ShellContext *ctx, char *buffer, int max) {
	int len = 0;

	while (1) {
		char c = read_key();
		if (c == 0) continue;

		if (c == '\n') {
			buffer[len] = '\0';
			shell_putchar(ctx, '\n');
			return len;
		}

		if (c == '\b') {
			if (len > 0) {
				len--;
				(*ctx->cursor_x)--;
				for (int row = 0; row < 8; row++) {
					for (int col = 0; col < 8; col++) {
						ctx->fb[(*ctx->cursor_y + row) * ctx->pps +
							(*ctx->cursor_x * 9 + col)] = 0x000000;
					}
				}
			}
			continue;
		}

		if (len < max - 1) {
			buffer[len++] = c;
			shell_putchar(ctx, c);
		}
	}
}

static int shell_parse(char *input, char *argv[], int max_args) {
	int argc = 0;
	char *p = input;

	while (*p && argc < max_args) {
		// Skip spaces
		while (*p == ' ') p++;
		if (*p == '\0') break;

		// Start of a token
		argv[argc++] = p;

		// Find end of token
		while (*p && *p != ' ') p++;
		if (*p == ' ') {
			*p = '\0';
			p++;
		}
	}

	return argc;
}


static void cmd_echo(ShellContext *ctx, int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		shell_print(ctx, argv[i]);
		if (i < argc - 1) shell_putchar(ctx, ' ');
	}
	shell_putchar(ctx, '\n');
}

static void cmd_clear(ShellContext *ctx) {
	for (uint32_t i = 0; i < 768 * ctx->pps; i++) {
		ctx->fb[i] = 0x000000;
	}
	*ctx->cursor_x = 0;
	*ctx->cursor_y = 0;
}

static void cmd_ls(ShellContext *ctx) {
	FSSO_Inode root;
	fsso_read_inode(ctx->fs, 0, &root);

	uint8_t buffer[FSSO_BLOCK_SIZE];
	uint32_t entries_per_block = FSSO_BLOCK_SIZE / sizeof(FSSO_DirEntry);

	for (int i = 0; i < FSSO_DIRECT_BLOCKS; i++) {
		if (root.direct[i] == 0) break;
		read_block(root.direct[i], buffer);
		FSSO_DirEntry *entries = (FSSO_DirEntry *)buffer;
		for (uint32_t j = 0; j < entries_per_block; j++) {
			if (entries[j].inode == 0) continue;
			shell_print(ctx, entries[j].name);
			shell_putchar(ctx, '\n');
		}
	}
}

static void cmd_cat(ShellContext *ctx, int argc, char *argv[]) {
	if (argc < 2) {
		shell_print(ctx, "Usage: cat <filename>\n");
		return;
	}

	uint32_t inode_num;
	if (fsso_find_in_dir(ctx->fs, 0, argv[1], &inode_num) != 0) {
		shell_print(ctx, "File not found: ");
		shell_print(ctx, argv[1]);
		shell_putchar(ctx, '\n');
		return;
	}

	uint8_t file_buf[4096];
	int bytes = fsso_read_file(ctx->fs, inode_num, file_buf, sizeof(file_buf));
	if (bytes < 0) {
		shell_print(ctx, "Error reading file\n");
		return;
	}

	for (int i = 0; i < bytes; i++) {
		shell_putchar(ctx, (char)file_buf[i]);
	}
	shell_putchar(ctx, '\n');
}

static void cmd_help(ShellContext *ctx) {
	shell_print(ctx, "SimplrOS commands:\n");
	shell_print(ctx, "  echo <text>     - print text\n");
	shell_print(ctx, "  clear           - clear screen\n");
	shell_print(ctx, "  ls              - list files\n");
	shell_print(ctx, "  cat <file>      - read file\n");
	shell_print(ctx, "  help            - this message\n");
}

static int str_eq(const char *a, const char *b) {
	while (*a && *a == *b) { a++; b++; }
	return *a == *b;
}

void shell_run(ShellContext *ctx) {
	char input[SHELL_MAX_INPUT];
	char *argv[SHELL_MAX_ARGS];

	while (1) {
		shell_print(ctx, "SimplrOS> ");
		shell_readline(ctx, input, SHELL_MAX_INPUT);

		if (input[0] == '\0') continue;

		int argc = shell_parse(input, argv, SHELL_MAX_ARGS);
		if (argc == 0) continue;

		if (str_eq(argv[0], "echo")) {
			cmd_echo(ctx, argc, argv);
		} else if (str_eq(argv[0], "clear")) {
			cmd_clear(ctx);
		} else if (str_eq(argv[0], "ls")) {
			cmd_ls(ctx);
		} else if (str_eq(argv[0], "cat")) {
			cmd_cat(ctx, argc, argv);
		} else if (str_eq(argv[0], "help")) {
			cmd_help(ctx);
		} else {
			shell_print(ctx, "Unknown command: ");
			shell_print(ctx, argv[0]);
			shell_putchar(ctx, '\n');
		}
	}
}
