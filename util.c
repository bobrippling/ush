#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdarg.h>

#include <unistd.h>
#include <fcntl.h>

#include "config.h"

int vsnprintf(char *str, size_t size, const char *format, va_list ap);

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

char *ustrdup_printf(const char *fmt, ...)
{
	char *buffer = NULL;
	int siz = strlen(fmt);
	va_list l;

	do{
		int ret;

		buffer = urealloc(buffer, siz);

		va_start(l, fmt);
		ret = vsnprintf(buffer, siz, fmt, l);
		va_end(l);

		if(ret < siz)
			break;
		siz += 32;
	}while(1);
	return buffer;
}

char *ustrndup(const char *s, size_t n)
{
	size_t len = strlen(s) + 1;
	char *p;

	if(len > n)
		len = n;

	p = umalloc(len);
	strncpy(p, s, len - 1);
	p[len-1] = '\0';
	return p;
}

char *ustrdup_argvp(char ***argvpp)
{
	char ***piter, **iter, *ret;
	int len = 1;

	for(piter = argvpp; *piter; piter++){
		for(iter = *piter; *iter; iter++)
			len += strlen(*iter) + 1;
		len += 3; /* " | " */
	}

	ret = umalloc(len);
	*ret = '\0';

	for(piter = argvpp; *piter; piter++){
		for(iter = *piter; *iter; iter++){
			strcat(ret, *iter);
			if(iter[1])
				strcat(ret, " ");
		}
		if(piter[1])
			strcat(ret, " | ");
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

const char *usignam(unsigned int sig)
{
	static const char *names[] = {
		NULL, "HUP", "INT", "QUIT",
		"ILL", "TRAP", "ABRT", "BUS",
		"FPE", "KILL", "USR1", "SEGV",
		"USR2", "PIPE", "ALRM", "TERM",
		"STKFLT", "CHLD", "CONT", "STOP",
		"TSTP", "TTIN", "TTOU", "URG",
		"XCPU", "XFSZ", "VTALRM", "PROF",
		"WINCH", "POLL", "PWR", "SYS"
	};

	if(sig < sizeof(names)/sizeof(names[0]))
		return names[sig];
	return NULL;
}

int block_set(int fd, int block)
{
	int flags = fcntl(fd, F_GETFL);
	if(block)
		flags &= ~O_NONBLOCK;
	else
		flags |=  O_NONBLOCK;
	return fcntl(fd, F_SETFL, flags) == -1;
}

int block_get(int fd)
{
	return !!(fcntl(fd, F_GETFL) & O_NONBLOCK);
}

unsigned int ugetuid()
{
	static int got = 0, uid;
	if(!got){
		got = 1;
		uid = getuid();
	}
	return uid;
}

unsigned int ugetgid()
{
	static int got = 0, gid;
	if(!got){
		got = 1;
		gid = getgid();
	}
	return gid;
}
