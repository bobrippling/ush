#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#include "util.h"
#include "proc.h"
#include "job.h"
#include "config.h"

#ifdef USH_DEBUG
# define JOB_DEBUG
#else
# undef JOB_DEBUG
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

		switch(p->pid = fork()){
			case 0:
				proc_exec(p, j->gid);
				break; /* unreachable */

			case -1:
				perror("fork()");
				goto bail;

			default:
				if(!j->gid)
					j->gid = p->pid;
				setpgid(p->pid, j->gid);
		}
	}

	return 0;
bail:
	for(; p; p = p->next)
		p->state = FIN;
	job_cont(j);
	return 1;
}

int job_sig(struct job *j, int sig)
{
	struct proc *p;
	int ret = 0;

	for(p = j->proc; p; p = p->next)
		if(kill(p->pid, sig))
			ret = 1;

	return ret;
}

int job_stop(struct job *j)
{
	return job_sig(j, SIGSTOP);
}

int job_cont(struct job *j)
{
	return job_sig(j, SIGCONT);
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
	pid = waitpid(-j->gid, &exit_status, 0); /* WNOHANG for async */

#ifdef USH_DEBUG
	fprintf(stderr, "%d\n", pid);
#endif

	save = errno;
	tcsetpgrp(STDIN_FILENO, orig_pgid);
	errno = save;

	if(pid == -1){
		if(errno == EINTR)
			goto rewait;
		perror("waitpid()");
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
