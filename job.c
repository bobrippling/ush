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
	int pgid = 0;

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

		if(proc_spawn(p, pgid))
			goto bail;
		if(p == j->proc)
			pgid = p->pid; /* p is group leader */
	}

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
	pid_t pid;
	struct proc *p;

rewait:
	pid = waitpid(-1, &exit_status, 0); /* WNOHANG for async */

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
