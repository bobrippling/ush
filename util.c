#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

#include "util.h"

extern jmp_buf allocerr;

void *umalloc(size_t l)
{
	void *p = malloc(l);
	if(!p)
		longjmp(allocerr, 1);
	return p;
}

char *ustrdup(const char *s)
{
	char *p = umalloc(strlen(s) + 1);
	strcpy(p, s);
	return p;
}
