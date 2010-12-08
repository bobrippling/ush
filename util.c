#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

extern jmp_buf allocerr;

void *umalloc(size_t size)
{
	void *p = malloc(size);
	if(!p)
		longjmp(allocerr, 1);
	return p;
}

char *ustrdup(const char *s)
{
	char *d = umalloc(strlen(s) + 1);
	strcpy(d, s);
	return d;
}

char *ustrdup_argv(char **argv)
{
	char **iter, *ret;
	int len;

	for(len = 1, iter = argv; *iter; iter++)
		len += strlen(*iter) + 1;

	ret = umalloc(len);
	*ret = '\0';

	for(iter = argv; *iter; iter++){
		strcat(ret, *iter);
		strcat(ret, " ");
	}

	return ret;
}
