#ifndef LIMIT_H
#define LIMIT_H

enum limit_type
{
	LIMIT_CPUTIME,
	LIMIT_FILESIZE,
	LIMIT_DATASIZE,
	LIMIT_STACKSIZE,
	LIMIT_COREDUMPSIZE,
	LIMIT_VMEMORYUSE,
	LIMIT_DESCRIPTORS,
	LIMIT_MEMORYLOCKED,
	LIMIT_MAXPROC,
	LIMIT_LAST
};

int limit_set(enum limit_type, long  val);
int limit_get(enum limit_type, long *val);

int  limit_parse(const char *, enum limit_type *);
const char *limit_str(enum limit_type);

#endif
