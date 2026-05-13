#pragma once
#include <stdint.h>
#include "fsso.h"


#define SHELL_MAX_INPUT 256
#define SHELL_MAX_ARGS  16

typedef struct {
	volatile uint32_t *fb;
	uint32_t pps;
	int *cursor_x;
	int *cursor_y;
	FSSO_Filesystem *fs;
} ShellContext;

void shell_run(ShellContext *ctx);
