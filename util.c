#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

/* FIXME */
#include <stdio.h>

extern jmp_buf allocerr;

void *umalloc(size_t size)
{
	void *p = malloc(size);
	if(!p)
		longjmp(allocerr, 1);
	return p;
}

void *urealloc(void *orig, size_t size)
{
	void *p = realloc(orig, size);
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

char *ustrdup_argvp(char ***argvp)
{
	char ***piter, **iter, *ret;
	int len = 1;

	for(piter = argvp; *piter; piter++)
		for(iter = *piter; *iter; iter++)
			len += strlen(*iter) + 1;

	ret = umalloc(len);
	*ret = '\0';

	for(piter = argvp; *piter; piter++)
		for(iter = *piter; *iter; iter++){
			strcat(ret, *iter);
			strcat(ret, " ");
		}

	return ret;
}

void ufree_argvp(char ***argvp)
{
	char ***orig = argvp;
	for(; *argvp; argvp++){
		char **argv;
		for(argv = *argvp; *argv; argv++)
			free(*argv);
		free(*argvp);
	}
	free(orig);
}
