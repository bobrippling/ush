#include <stdio.h>

#include "esc.h"

static void esc(const char *);

static void esc(const char *e)
{
	putchar('\x1b');
	fputs(e, stdout);
}

void esc_reset()
{
	esc("c");
}

void esc_clrtoeol()
{
	esc("[K");
}
