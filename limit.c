#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <sys/resource.h>

#include "limit.h"

static const struct { const char *s; int rlim; } lims[] = {
	[LIMIT_CPUTIME]      = { "cputime",       RLIMIT_CPU },
	[LIMIT_FILESIZE]     = { "filesize",      RLIMIT_FSIZE },
	[LIMIT_DATASIZE]     = { "datasize",      RLIMIT_DATA },
	[LIMIT_STACKSIZE]    = { "stacksize",     RLIMIT_STACK },
	[LIMIT_COREDUMPSIZE] = { "coredumpsize",  RLIMIT_CORE },
/*[LIMIT_MEMORYUSE]    = { "memoryuse",     RLIMIT_ },*/
	[LIMIT_VMEMORYUSE]   = { "vmemoryuse",    RLIMIT_AS },
	[LIMIT_DESCRIPTORS]  = { "descriptors",   RLIMIT_NOFILE },
#ifdef RLIMIT_MEMLOCK
	[LIMIT_MEMORYLOCKED] = { "memorylocked",  RLIMIT_MEMLOCK },
	[LIMIT_MAXPROC]      = { "maxproc",       RLIMIT_NPROC },
#endif
	[LIMIT_LAST]         = { NULL,            -1 }
};

int limit_set(enum limit_type t, long val)
{
	struct rlimit rl;

	if(getrlimit(lims[t].rlim, &rl) == -1)
		return 1;
	rl.rlim_cur = val;
	if(setrlimit(lims[t].rlim, &rl) == -1)
		return 1;
	return 0;
}

int limit_get(enum limit_type t, long *val)
{
	struct rlimit rl;

	if(getrlimit(lims[t].rlim, &rl) == -1)
		return 1;
	*val = rl.rlim_cur;
	return 0;
}

int limit_parse(const char *s, enum limit_type *t)
{
	int i;

	for(i = 0; i < LIMIT_LAST; i++)
		if(!strcmp(s, lims[i].s)){
			*t = i;
			return 0;
		}

	return 1;
}

const char *limit_str(enum limit_type t)
{
	return lims[t].s;
}
