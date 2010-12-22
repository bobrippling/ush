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

struct job *job_new_single(char ***argvp, int jid);


/** create a struct proc for each argv */
struct job *job_new(char ****argvpp)
{
	extern struct job *jobs;
	struct job *j, *firstj = NULL, *currentj;
	int jid = 1;

	for(j = jobs; j; j = j->next)
		if(j->job_id == jid){
			jid++;
			j = jobs;
		}

	for(; *argvpp; argvpp++){
		j = job_new_single(*argvpp, jid);

		if(firstj)
			currentj->jobnext = j;
		else
			firstj = j;
		currentj = j;
	}
	currentj->jobnext = NULL;

	return firstj;
}

struct job *job_new_single(char ***argvp, int jid)
{
	struct job *j;
	struct proc **tail;

	j = umalloc(sizeof *j);
	memset(j, 0, sizeof *j);

	j->cmd    = ustrdup_argvp(argvp);
	j->argvp  = argvp;
	j->job_id = jid;
	j->state  = JOB_BEGIN;

	tail = &j->proc;
	while(*argvp){
		*tail = proc_new(*argvp++);
		tail = &(*tail)->next;
	}
	*tail = NULL;
	return j;
}

int job_next(struct job **jobs, struct job *j)
{
	/*
	 * start the next job in the job's list
	 * or rm the job if the last has completed
	 */
	struct job *iter;

	for(iter = j->jobnext; iter; iter = iter->jobnext)
		if(iter->state == JOB_BEGIN){
			job_start(iter);
			return 0;
		}

	if(job_fully_complete(j)){
		job_rm(jobs, j);
		return 1;
	}

	return 0;
}

int job_in_group(struct job *needle, struct job *group)
{
	for(; group; group = group->jobnext)
		if(needle == group)
			return 1;
	return 0;
}

void job_rm(struct job **jobs, struct job *j)
{
	if(j == *jobs){
		*jobs = j->next;

		job_free_all(j);
	}else{
		struct job *prev;

		/* FIXME - job_in_group business */
		for(prev = *jobs; prev->next; prev = prev->next)
			if(job_in_group(j, prev->next)){
				prev->next = j->next;
				job_free_all(j);
				return;
			}

		fprintf(stderr, "job_rm(): \"%s\" not found!\n", j->cmd);
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

	j->state = JOB_RUNNING;

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

int job_fg(struct job *j, struct job **jobs)
{
	job_cont(j);
	return job_wait_all(j, jobs, 0);
}

int job_check_all(struct job **jobs)
{
	struct job *jhead, *jactual;

restart:
	for(jhead = *jobs; jhead; jhead = jhead->next)
		for(jactual = jhead; jactual; jactual = jactual->jobnext)
			switch(jactual->state){
				case JOB_COMPLETE:
					jactual->state = JOB_MOVED_ON;
					job_next(jobs, jactual);
					goto restart;

				case JOB_RUNNING:
					/*
					 * asyncronously check for completion
					 * + remove if done
					 */
					if(job_wait_all(jactual, jobs, 1))
						goto restart;

					if(jactual->state == JOB_COMPLETE){
						job_next(jobs, jactual);
						goto restart;
					}
					break;

				case JOB_MOVED_ON:
				case JOB_BEGIN:
					break;
			}

	return 0;
}

int job_wait_all(struct job *j, struct job **jobs, int async)
{
	struct proc *p;
	struct job *iter;

	for(iter = j; iter; iter = iter->jobnext){
		job_wait(iter, async);

		if(async && iter->state == JOB_RUNNING)
			return 0;
		else if(iter->state == JOB_COMPLETE){
			if(job_next(jobs, iter))
				return 1; /* done with the job list - tell caller we changed something</matrix> */
			continue;
		}

		/*
		 * check for a stopped job
		 * if we find one, it's
		 * been ^Z'd, stop other procs and return
		 */
		for(p = iter->proc; p; p = p->next)
			if(p->state == PROC_STOP){
				job_stop(iter);
				return 0;
				/* don't attempt to move onto any other jobs */
			}
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
		j->state = JOB_COMPLETE;
	return 0;
}

void job_free_all(struct job *j)
{
	struct job *iter, *delthis;

	for(iter = j; iter;
			delthis = iter, iter = iter->jobnext, free(delthis)){
		struct proc *p, *next;

		for(p = j->proc; p; p = next){
			next = p->next;
			proc_free(p);
		}
		ufree_argvp(j->argvp);
		free(j->cmd);
	}
}

const char *job_state_name(struct job *j)
{
	switch(j->state){
		case JOB_BEGIN:    return "init";
		case JOB_RUNNING:  return "running";
		case JOB_COMPLETE: return "complete";
		case JOB_MOVED_ON: return "complete (moved on)";
	}
	return NULL;
}

int job_fully_complete(struct job *j)
{
	/* FIXME: rewind to job head and check those first */
	for(; j; j = j->jobnext)
		switch(j->state){
			case JOB_BEGIN:
			case JOB_RUNNING:
				return 0;
			default:
				break;
		}

	return 1;
}
