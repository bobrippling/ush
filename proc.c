#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "util.h"
#include "proc.h"
#include "builtin.h"
#include "config.h"

#define EXEC_FUNC execvp

#ifdef USH_DEBUG
# define  PROC_DEBUG
#else
# undef  PROC_DEBUG
#endif

struct proc *proc_new(char **argv)
{
	struct proc *p = umalloc(sizeof *p);

	p->argv = argv;
	p->pid = -1;
	p->next = NULL;
	p->state = PROC_SPAWN;
	p->in = p->out = p->err = -1;

	return p;
}

int proc_exec(struct proc *p, int pgid)
{
	int ret;

	/* signals back in business */
	if(pgid == 0)
		pgid = getpid();

	setpgid(getpid(), pgid);
	/*term_give_to(pgid);*/

	signal(SIGINT,  SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);

	builtin_exec(p);

	EXEC_FUNC(*p->argv, p->argv);

	if(errno == ENOENT)
		ret = 127;
	else
		ret = 126;

	fprintf(stderr, "exec(): %s: %s\n", *p->argv, strerror(errno));

	_exit(ret);
	return ret;
}

void proc_free(struct proc *p)
{
	free(p);
}

char *proc_desc(struct proc *p)
{
	char **iter, *ret;
	int len = 1;

	for(iter = p->argv; *iter; iter++)
		len += strlen(*iter);

	ret = umalloc(len);
	*ret = '\0';

	for(iter = p->argv; *iter; iter++)
		strcat(ret, *iter);

	return ret;
}
