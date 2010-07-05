#define _POSIX_SOURCE
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "proc.h"
#include "util.h"

extern struct termios shell_termmode;
extern pid_t pgid;
extern char interactive;

static int proc_spawn(struct process *proc);
static char job_test(struct job *, enum procstate);
static struct process *job_findprocess(struct job *, pid_t);


int job_spawn(struct job *j, char bg)
{
	/*
	 * job_create must spawn each process in the j,
	 * into a new process group (hence j)
	 * which then gives this j to the terminal foreground
	 * via tcsetpgrp
	 */
	struct process *p;
	int pipefd[2];

	if(interactive)
		/* some bs about race conditions */
		setpgid(0, j->pgid);

	j->proc->in = j->infile;
	for(p = j->proc; p; p = p->next){
		if(p->next){
			if(pipe(pipefd) == -1){
				perror("pipe()");
				/* TODO: cleanup */
				return 1;
			}
			/*
			 * into pipe[1], out to pipe[0]
			 * aka pipe[0] is readable
			 */
			p->out = pipefd[1];
			p->next->in = pipefd[0];
		}else
			p->out = j->outfile;

		if(proc_spawn(p)){
			perror("proc_spawn()");
			/* TODO: cleanup */
			return 1;
		}
	}

	if(!interactive)
		job_waitfor(j, 1);
	else if(bg)
		job_background(j, 0);
	else
		job_foreground(j, 0);

	return 0;
}

static int proc_spawn(struct process *proc)
{
	pid_t pid;

	switch(pid = fork()){
		case -1:
			perror("fork()");
			return -1;

		case 0:
		{
			if(interactive){
				signal(SIGINT , SIG_DFL);
				signal(SIGQUIT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);
				signal(SIGTTIN, SIG_DFL);
				signal(SIGTTOU, SIG_DFL);
				signal(SIGCHLD, SIG_DFL);

				/* move to group */
				if(setpgid(0, pgid)){
					perror("setpgid()");
					_exit(-1);
				}
			}

#define REDIR(a, b) \
		do{ \
			/* copy a into b */ \
			if(dup2(a, b) == -1) \
				perror("dup2()"); \
			if(a != b && close(a) == -1) \
				perror("close()"); \
		}while(0)
			REDIR(proc->in,  STDIN_FILENO );
			REDIR(proc->out, STDOUT_FILENO);
			REDIR(proc->err, STDERR_FILENO);
#undef REDIR

			execv(*proc->argv, proc->argv);
			perror("execv()");
			_exit(-1);
		}

		default:
			proc->pid = pid;
			return 0;
	}

	return -1;
}

struct job *job_find(struct job *list, pid_t pid)
{
	struct job *j;
	struct process *p;

	for(j = list; j; j = j->next)
		for(p = j->proc; p; p = p->next)
			if(p->pid == pid)
				return j;

	return NULL;
}

void job_foreground(struct job *j, char resume)
{
	struct process *p;

	tcsetpgrp(STDIN_FILENO, j->pgid);

	if(resume){
		/* TCSADRAIN: set after all pending stuff's been done */
		tcsetattr(STDIN_FILENO, TCSADRAIN, &j->termmode);

		kill(-j->pgid, SIGCONT);
	}

	for(p = j->proc; p; p = p->next)
		p->state = FG;

	job_waitfor(j, 1);

	tcgetattr(STDIN_FILENO, &j->termmode);

	tcsetpgrp(STDIN_FILENO, pgid);
	tcsetattr(STDIN_FILENO, TCSADRAIN, &shell_termmode);
}

void job_background(struct job *j, char resume)
{
	struct process *p;

	if(resume)
		kill(-j->pgid, SIGCONT);

	for(p = j->proc; p; p = p->next)
		p->state = BG;
}

void job_waitfor(struct job *j, char block)
{
	struct process *p;
	int ret;
	pid_t pid;

	pid = waitpid(-j->pgid, &ret, WUNTRACED | (block ? 0 : WNOHANG));

	if(pid == 0 && !block)
		/* nothing happnin */
		return;

	p = job_findprocess(j, pid);

	if(WIFSTOPPED(ret)){
		job_background(j, 1);
	}else if(WIFEXITED(ret)){
		for(p = j->proc; p; p = p->next)
			p->state = DONE;

		j->exitcode = WEXITSTATUS(ret);
		j->exitsig  = WIFSIGNALED(ret) ? WTERMSIG(ret) : 0;
	}
}

/*
 * utility
 */

static struct process *job_findprocess(struct job *j, pid_t pid)
{
	struct process *p;
	for(p = j->proc; p; p = p->next)
		if(p->pid == pid)
			return p;
	return NULL;
}

struct job *job_new(struct process *proclist, int infile, int outfile)
{
	struct job *j = umalloc(sizeof *j);
	struct process *p;

	for(p = j->proc; p; p = p->next)
		p->state = INIT;

	j->infile  = infile;
	j->outfile = outfile;
	j->proc    = proclist;

	return j;
}

void job_free(struct job *j)
{
	proc_free(j->proc);
	free(j);
}

struct process *proc_new(char **argv, int files[3])
{
	struct process *p = umalloc(sizeof *p);

	p->argv = argv;
	p->in  = files[0];
	p->out = files[1];
	p->err = files[2];

	p->next = NULL;

	return p;
}

void proc_free(struct process *p)
{
	char **s;

	if(p->next)
		proc_free(p->next);

	for(s = p->argv; *s; s++)
		free(*s);

	free(p->argv);

	free(p);
}

char job_isdone(struct job *j)
{
	return job_test(j, DONE);
}

char job_isbackground(struct job *j)
{
	return job_test(j, BG);
}

static char job_test(struct job *j, enum procstate s)
{
	struct process *p;
	for(p = j->proc; p; p = p->next)
		if(p->state != s)
			return 0;
	return 1;
}
