#ifndef JOB_H
#define JOB_H

struct job
{
	char *cmd;
	int gid;
	int complete;
	struct proc *proc; /* list */

	struct job *next;
};

struct job *job_new(char *cmd, char ***argvp);

int         job_start(   struct job *);
int         job_wait(    struct job *);
int         job_wait_all(struct job *);

#endif
