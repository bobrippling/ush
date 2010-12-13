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

void job_close_fds(struct job *j)
{
	struct proc *p;
	for(p = j->proc; p; p = p->next){
		if(p->in != STDIN_FILENO)
			close(p->in);
		if(p->out != STDOUT_FILENO)
			close(p->out);
		if(p->err != STDERR_FILENO)
			close(p->err);
	}
}

int job_start(struct job *j)
{
	struct proc *p;
	int pipey[2] = { -1, -1 };

	j->proc->in = STDIN_FILENO;
	for(p = j->proc; p; p = p->next){
		p->err = STDERR_FILENO;
		if(p->next){
			if(pipe(pipey) < 0){
				perror("pipe()");
				goto bail;
			}
			p->out = pipey[1];
			p->next->in = pipey[0];
		}else
			p->out = STDOUT_FILENO;

		switch(p->pid = fork()){
			case 0:
#define REDIR(a, b) \
					do{ \
						/* close b and copy a into b */ \
						if(a != b){ \
							if(dup2(a, b) == -1) \
								perror("dup2()"); \
							if(close(a) == -1) \
								perror("close()"); \
						} \
					}while(0)
				REDIR(p->in,   STDIN_FILENO);
				REDIR(p->out, STDOUT_FILENO);
				REDIR(p->err, STDERR_FILENO);
#undef REDIR
				close(pipey[0]);
				close(pipey[1]);
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

	/* close our access to all these pipes */
	job_close_fds(j);

	return 0;
bail:
	for(; p; p = p->next)
		p->state = FIN;
	job_close_fds(j);
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
	struct proc *p;
	pid_t pid;
#define JOB_WAIT_CHANGE_GROUP
#ifdef JOB_WAIT_CHANGE_GROUP
	const pid_t orig_pgid = getpgid(STDIN_FILENO);
	int save;
#endif

rewait:
#ifdef JOB_WAIT_CHANGE_GROUP
	tcsetpgrp(STDIN_FILENO, j->gid);
	/*setpgid(getpid(), j->gid);*/
#endif
	pid = waitpid(-j->gid, &exit_status, 0); /* WNOHANG for async */

#ifdef JOB_WAIT_CHANGE_GROUP
	save = errno;
	tcsetpgrp(STDIN_FILENO, orig_pgid);
	/*setpgid(getpid(), orig_pgid);*/
	errno = save;
#endif

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
