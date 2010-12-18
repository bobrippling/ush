#define _XOPEN_SOURCE 500
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
struct job *job_new(char *cmd, char ****argvpp)
{
	struct job *prevj = NULL, *firstj;
	char ***argvpp_iter;

	for(argvpp_iter = *argvpp; *argvpp_iter; argvpp_iter++){
		extern struct job *jobs;
		struct job *j;
		struct proc **tail;
		int jid = 1;
		char ***argvp = argvpp_iter;

		for(j = jobs; j; j = j->next)
			if(j->job_id == jid){
				jid++;
				j = jobs;
			}

		j = umalloc(sizeof *j);
		memset(j, 0, sizeof *j);

		j->cmd = cmd;
		j->argvp = argvp;
		j->job_id = jid;

		tail = &j->proc;
		while(*argvp){
			*tail = proc_new(*argvp++);
			tail = &(*tail)->next;
		}

		*tail = NULL;

		if(prevj)
			prevj->afterjob = j;
		else
			firstj = j;

		prevj = j;
	}

	return firstj;
}

void job_rm(struct job **jobs, struct job *j)
{
	if(j == *jobs){
		*jobs = j->next;
		job_free(j);
	}else{
		struct job *prev;

		for(prev = *jobs; prev->next; prev = prev->next)
			if(j == prev->next){
				prev->next = j->next;
				job_free(j);
				break;
			}
	}
}

void job_close_fds(struct job *j, struct proc *pbreak)
{
	struct proc *p;
	for(p = j->proc; p && p != pbreak; p = p->next){
#define CLOSE(a, b) \
		if(a != b) \
			close(a)

		CLOSE(p->in , STDIN_FILENO);
		CLOSE(p->out, STDOUT_FILENO);
		CLOSE(p->err, STDERR_FILENO);
#undef CLOSE
	}
}

int job_start(struct job *j)
{
	struct proc *p;
	int pipey[2] = { -1, -1 };

	j->proc->in = STDIN_FILENO; /* TODO */
	for(p = j->proc; p; p = p->next){
		p->err = STDERR_FILENO; /* TODO */
		if(p->next){
			if(pipe(pipey) < 0){
				perror("pipe()");
				goto bail;
			}
			p->out = pipey[1];
			p->next->in = pipey[0];
		}else
			p->out = STDOUT_FILENO; /* TODO */

		/* TODO: cd, fg */

		switch(p->pid = fork()){
			case 0:
				p->pid = getpid();
#define REDIR(a, b) \
					do{ \
						if(a != b){ \
							/* close b and copy a into b */ \
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
				job_close_fds(j, p->next);
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
	job_close_fds(j, NULL);

	return 0;
bail:
	for(; p; p = p->next)
		p->state = PROC_FIN;
	job_close_fds(j, NULL);
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

int job_bg(struct job *j)
{
	return job_cont(j);
}

int job_fg(struct job *j)
{
	job_cont(j);
	return job_wait_all(j, 0);
}

int job_check_all(struct job **jobs)
{
	struct job *j;

restart:
	for(j = *jobs; j; j = j->next)
		if(job_complete(j)){
			job_rm(jobs, j);
			goto restart;
		}else{
			if(job_wait_all(j, 1))
				return 1;

			if(job_complete(j)){
				job_rm(jobs, j);
				goto restart;
			}
		}

	return 0;
}

int job_wait_all(struct job *j, int async)
{
	struct proc *p;

	if(job_wait(j, async))
		return 1;

	if(job_complete(j))
		return 0;
	/*
	 * check for a stopped job
	 * if we find one, it's
	 * been ^Z'd, stop the rest and return
	 */

	for(p = j->proc; p; p = p->next)
		if(p->state == PROC_STOP){
			job_stop(j);
			return 0;
		}

	return 0;
}

int job_wait(struct job *j, int async)
{
	int wait_status = 0, proc_still_running = 0;
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
	pid = waitpid(-j->gid, &wait_status,
			WUNTRACED | WCONTINUED | (async ? WNOHANG : 0));

#ifdef JOB_WAIT_CHANGE_GROUP
	save = errno;
	tcsetpgrp(STDIN_FILENO, orig_pgid);
	/*setpgid(getpid(), orig_pgid);*/
	errno = save;
#endif

	if(pid == -1){
		if(errno == EINTR)
			goto rewait;
		fprintf(stderr, "waitpid(%d [job %d: \"%s\"], async = %s): %s\n",
				-j->gid, j->job_id, j->cmd, async ? "true" : "false",
				strerror(errno));
		return 1;
	}else if(pid == 0 && async)
		return 0;


	for(p = j->proc; p; p = p->next)
		if(p->pid == pid){
			int processed = 0;

			if(WIFSIGNALED(wait_status)){
				p->last_sig = WSTOPSIG(wait_status);
				processed = 1;
			}

			if(WIFEXITED(wait_status)){
				p->state = PROC_FIN;

				if(WIFSIGNALED(wait_status))
					p->exit_sig = WTERMSIG(wait_status);
				else
					p->exit_sig = 0;

				p->exit_code = WEXITSTATUS(wait_status);

				processed = 1;
			}else if(WIFSTOPPED(wait_status)){
				p->state = PROC_STOP;
				p->exit_sig = WSTOPSIG(wait_status); /* could be TTIN or pals */
				proc_still_running = 1;

				processed = 1;
			}else if(WIFCONTINUED(wait_status)){
				p->state = PROC_RUN;
				proc_still_running = 1;

				processed = 1;
			}

			if(!processed)
				fprintf(stderr, "ush: wait_status unknown for pid %d: %d\n", pid, wait_status);

			break;
		}else if(p->state != PROC_FIN)
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
