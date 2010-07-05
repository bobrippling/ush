#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "proc.h"

extern struct termios shell_termmode;
extern pid_t pgid;

static int proc_spawn(char **argv, char background, pid_t pgid);
static void waitforjob(struct job *);


int job_spawn(struct job *job, char bg)
{
	/*
	 * job_create must spawn each process in the job,
	 * into a new process group (hence job)
	 * which then gives this job to the terminal foreground
	 * via tcsetpgrp
	 */
	struct process *p;
	int pipefd[2];

	if(interactive)
		/* some bs about race conditions */
		setpgid(0, job->pgid);

	job->proc->in = job->infile;
	for(p = job->proc; p; p = p->next){
		if(p->next){
			if(-1 == pipe(pipefd)){
				perror("pipe()");
				return 1;
			}
			/*
			 * into pipe[1], out to pipe[0]
			 * aka pipe[0] is readable
			 */
			p->out = pipe[1];
			p->next->in = pipe[0];
		}else
			p->out = job->outfile;

		if(proc_spawn(p, job->pgid)){
			perror("proc_spawn()");
			break;
		}
	}

	if(interactive || !bg)
		job_wait(j);
	else
		job_background(j);
}

static int proc_spawn(struct process *proc, int pgid)
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

#define REDIR(opn, cls) \
		do{ \
			if(dup2(open, cls) == -1) \
				perror("dup2()"); \
			if(close(cls) == -1) \
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
			return 0;
	}

	return -1;
}

void job_foreground(struct job *j, char resume)
{
	tcsetpgrp(j->pgrp);

	if(resume){
		/* TCSADRAIN: set after all pending stuff's been done */
		tcsetattr(STDIN_FILENO, TCSADRAIN, &j->termmode);

		kill(-j->pgid, SIGCONT);
	}

	waitforjob(j);

	tcgetattr(STDIN_FILENO, &j->termmode);

	tcsetpgrp(pgid);
	tcsetattr(STDIN_FILENO, TCSADRAIN, &shell_termmode);
}

void job_background(struct job *j, char resume)
{
	if(resume)
		kill(-j->pgid, SIGCONT);
}

static void waitforjob(struct job *j)
{
	int ret;

	if(waitpid(-j->pgid, &ret, WUNTRACED) == (pid_t)-1)
		perror("waitpid()");

	/* TODO FIXME XXX here */
	if(WIFSTOPPED(ret)){
	}else{
	}
}
