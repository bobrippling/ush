#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#include "util.h"
#include "proc.h"

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

int proc_spawn(struct proc *p, int pgid)
{
	pid_t pid;
	switch(pid = fork()){
		case -1:
			perror("fork()");
			return 1;

		case 0:
			/* signals back in business */
			signal(SIGINT , SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			signal(SIGTTIN, SIG_DFL);
			signal(SIGTTOU, SIG_DFL);
			signal(SIGCHLD, SIG_DFL);
			if(setpgid(0, pgid)){
				perror("setpgid()");
				_exit(1);
			}
#define REDIR(a, b) \
		do{ \
			/* copy a into b */ \
			if(dup2(a, b) == -1) \
				perror("dup2()"); \
			if(a != b && close(a) == -1) \
				perror("close()"); \
		}while(0)
			REDIR(p->in,  STDIN_FILENO );
			REDIR(p->out, STDOUT_FILENO);
			REDIR(p->err, STDERR_FILENO);
#undef REDIR
			execv(*p->argv, p->argv);
			perror("execv()");
			_exit(-1);
			return 1;

		default:
			p->pid = pid;
			return 0;
	}
}

void proc_free(struct proc *p)
{
	char **argv;
	for(argv = p->argv; *argv; argv++)
		free(*argv);
	free(p);
}
