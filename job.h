#ifndef JOB_H
#define JOB_H

struct job
{
	char ***argvp;
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

void        job_free(    struct job *);

int job_sig( struct job *, int sig);
int job_stop(struct job *);
int job_cont(struct job *);

#endif
