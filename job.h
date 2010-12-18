#ifndef JOB_H
#define JOB_H

struct job
{
	char ***argvp;
	char *cmd;
	int gid;
	int job_id;
	int complete;
	struct proc *proc; /* list */
	/*struct job  *afterjob;*/
	/* next job in the list - "echo hi; vi test.c" */


	struct job *next; /* link-a-list helper */
};

struct job *job_new(char *cmd, char ***argvp);
void        job_rm(struct job **jobs, struct job *j);

int         job_start(   struct job *);
int         job_wait(    struct job *, int async);
int         job_wait_all(struct job *, int async);
int         job_check_all(struct job **jobs);

void        job_free(    struct job *);

int         job_sig( struct job *, int sig);
int         job_stop(struct job *);
int         job_cont(struct job *);
int         job_fg(  struct job *);
int         job_bg(  struct job *);

#define     job_complete(j) ((j)->complete)

#endif
