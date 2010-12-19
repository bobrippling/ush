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
	/* signals back in business */
	if(pgid == 0)
		pgid = getpid();

	setpgid(getpid(), pgid);
	/*tcsetpgrp(STDIN_FILENO, pgid);*/

	signal(SIGINT,  SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);

	builtin_exec(p);

	EXEC_FUNC(*p->argv, p->argv);
	fprintf(stderr, "execv(): %s: %s\n", *p->argv, strerror(errno));
	_exit(-1);
	return 1;
}

void proc_free(struct proc *p)
{
	free(p);
}
