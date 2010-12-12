#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#include "util.h"
#include "proc.h"
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
	p->state = SPAWN;
	p->in = p->out = p->err = -1;

	return p;
}

int proc_exec(struct proc *p, int pgid)
{
	/* signals back in business */
	if(pgid == 0)
		pgid = getpid();

	setpgid(getpid(), pgid);
	tcsetpgrp(STDIN_FILENO, pgid);

	signal(SIGINT,  SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);
#define REDIR(a, b) \
		do{ \
			/* copy a into b */ \
			if(dup2(a, b) == -1) \
				perror("dup2()"); \
			if(a != b && close(a) == -1) \
				perror("close()"); \
		}while(0)
	REDIR(p->in,   STDIN_FILENO);
	REDIR(p->out, STDOUT_FILENO);
	REDIR(p->err, STDERR_FILENO);
#undef REDIR

	EXEC_FUNC(*p->argv, p->argv);
	perror("execv()");
	_exit(-1);
	return 1;
}

void proc_free(struct proc *p)
{
	free(p);
}
