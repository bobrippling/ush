#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#include "util.h"
#include "proc.h"
#include "job.h"

#if 0
static void job_spawn( struct job *j);
static void job_remove(struct job *j);
#endif


/** create a struct proc for each argv */
struct job *job_new(char *cmd, char ***argvp)
{
	struct job *j = umalloc(sizeof *j);
	struct proc **tail = &j->proc;

	memset(j, 0, sizeof *j);

	j->cmd = cmd;
	j->argvp = argvp;

	while(*argvp){
		*tail = proc_new(*argvp++);
		tail = &(*tail)->next;
	}

	*tail = NULL;

	return j;
}

int job_start(struct job *j)
{
	struct proc *p;
	int pipey[2];
	int orig_pgid = getpgid(STDIN_FILENO);

	j->proc->in = STDIN_FILENO;
	for(p = j->proc; p; p = p->next){
		p->err = STDERR_FILENO;
		if(p->next){
			if(pipe(pipey) < 0){
				perror("pipe()");
				return 1;
			}
			p->out = pipey[1];
			p->next->in = pipey[0];
		}else
			p->out = STDOUT_FILENO;

		if(proc_spawn(p, j->gid))
			goto bail;
		/* p->pid set by proc_spawn */

		if(p == j->proc){
			/* first */
			j->gid = p->pid; /* p is group leader */
			setpgid(p->pid, j->gid);

			printf("job_start: j->gid: %d\n", j->gid);
		}
	}

	tcsetpgrp(STDIN_FILENO, orig_pgid); /* move us back into the term */
	return 0;
bail:
	for(; p; p = p->next)
		p->state = FIN;
	return 1;
}


int job_wait_all(struct job *j)
{
	while(!j->complete)
		if(job_wait(j))
			return 1;
	return 0;
}

int job_wait(struct job *j)
{
	int exit_status, proc_still_running = 0;
	pid_t pid, orig_pgid = getpgid(STDIN_FILENO);
	struct proc *p;
	int save;

rewait:
	tcsetpgrp(STDIN_FILENO, j->gid);
	pid = waitpid(-j->gid, &exit_status, 0); /* WNOHANG for async */

	save = errno;
	tcsetpgrp(STDIN_FILENO, orig_pgid);
	errno = save;

	if(pid == -1){
		if(errno == EINTR)
			goto rewait;
		return 1;
	}

	for(p = j->proc; p; p = p->next)
		if(p->pid == pid){
			p->state = FIN;
			p->exit_status = exit_status;
		}else if(p->state != FIN)
			proc_still_running = 1;

	if(!proc_still_running)
		j->complete = 1;
	return 0;
}

void job_free(struct job *j)
{
	struct proc *p, *next;
	for(p = j->proc; p; p = next){
		next = p->next;
		proc_free(p);
	}
	ufree_argvp(j->argvp);
	free(j->cmd);
	free(j);
}
