#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>
#include <signal.h>

#include "term.h"
#include "util.h"
#include "proc.h"
#include "job.h"
#include "parse.h"
#include "task.h"

void task_rm(struct task **tasks, struct task *t);
void task_free(struct task *t);
char *task_desc(struct task *t);


struct task *task_new(struct parsed *prog)
{
	extern struct task *tasks;
	struct task *t, *thistask;
	struct job *jprev = NULL;
	struct parsed *iter;
	int jid = 1;

	for(t = tasks; t; t = t->next){
		struct job *j;
		for(j = t->jobs; j; j = j->next)
			if(j->job_id == jid){
				jid++;
				t = tasks;
				break;
			}
	}

	thistask = umalloc(sizeof *thistask);
	memset(thistask, 0, sizeof *thistask);

	thistask->state = TASK_BEGIN;

	for(iter = prog; iter; iter = iter->next){
		struct job *j = job_new(iter->argvp, jid, iter->redir);

		if(jprev)
			jprev->next = j;
		else
			thistask->jobs = j;
		jprev = j;
	}
	jprev->next = NULL;

	thistask->cmd = task_desc(thistask);

	return thistask;
}

int task_start(struct task *t)
{
	t->state = TASK_RUNNING;
	return job_start(t->jobs);
}

int task_check_all(struct task **tasks)
{
	/* return 1 if we handled a task */
	int ret = 0;
	struct task *t;

restart:
	for(t = *tasks; t; t = t->next)
		switch(t->state){
			case TASK_COMPLETE:
				task_rm(tasks, t);
				ret = 1;
				goto restart;

			case TASK_RUNNING:
				/* asyncronously check for completion */
				if(task_wait(tasks, t, 1)){
					ret = 1;
					goto restart;
				}
				break;

			case TASK_BEGIN:
				break;
		}

	return ret;
}

int task_wait(struct task **tasks, struct task *t, int async)
{
	struct proc *p;
	struct job *j;

	for(j = t->jobs; j; j = j->next)
		while(!job_complete(j)){
			/* TODO: term_give_to() */
			job_wait(j, async);

			if(async && j->state == JOB_RUNNING)
				/* no change */
				return 0;
			else if(j->state == JOB_COMPLETE){
				/* changed to complete, next job in the list */
				if(j->next){
					job_start(j->next);
					if(async)
						return 0;
					continue;
				}else{
					task_rm(tasks, t);
					return 1;
				}
			}

			/*
			* check for a stopped proc
			* if we find one, it's
			* been ^Z'd, stop other procs and return
			*/
			for(p = j->proc; p; p = p->next)
				if(p->state == PROC_STOP){
					job_sig(j, SIGSTOP);
					return 0;
					/* don't attempt to move onto any other jobs */
				}
		}

	return 0;
}

int task_sig(struct task *t, int sig)
{
	struct job *j;
	int ret = 0;

	for(j = t->jobs; j; j = j->next)
		ret |= job_sig(j, sig);

	return ret;
}

char *task_desc(struct task *t)
{
	char *ret;
	int len = 1;
	struct job *j;

	for(j = t->jobs; j; j = j->next)
		len += strlen(j->cmd) + 2; /* "; " */

	ret = umalloc(len);
	*ret = '\0';

	for(j = t->jobs; j; j = j->next){
		strcat(ret, j->cmd);
		if(j->next)
			strcat(ret, "; ");
	}

	return ret;
}

void task_rm(struct task **tasks, struct task *t)
{
	struct task *iter, *prev = NULL;

	for(iter = *tasks; iter; iter = iter->next)
		if(iter == t){
			if(prev)
				prev->next = iter->next;
			else
				*tasks = iter->next;

			task_free(t);
			return;
		}else
			prev = iter;

	fprintf(stderr, "task_rm(): \"%s\" not found\n", t->cmd);
}

void task_free(struct task *t)
{
	struct job *j, *j2;

	free(t->cmd);

	for(j = t->jobs; j; j2 = j, j = j->next, job_free(j2));
	free(t);
}

const char *task_state_name(struct task *t)
{
	switch(t->state){
		case TASK_BEGIN:    return "init";
		case TASK_RUNNING:  return "running";
		case TASK_COMPLETE: return "complete";
	}
	return NULL;
}
