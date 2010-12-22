#ifndef JOB_H
#define JOB_H

struct job
{
	char ***argvp;
	char *cmd;
	int gid;
	int job_id;
	enum { JOB_BEGIN, JOB_RUNNING, JOB_COMPLETE, JOB_MOVED_ON } state;
	struct proc *proc; /* list */
	struct job  *jobnext;
	/* next job in the list - "echo hi; vi test.c" */

	struct job *next; /* link-a-list helper */
};

struct job *job_new(char ****argvpp);
void        job_rm(struct job **jobs, struct job *j);

int         job_start(   struct job *);
int         job_wait(    struct job *, int async);
int         job_wait_all(struct job *j, struct job **jobs, int async);
int         job_check_all(struct job **jobs);
int         job_fully_complete(struct job *);

void        job_free_all(struct job *j);

const char *job_state_name(struct job *);

int         job_sig( struct job *, int sig);
int         job_stop(struct job *);
int         job_cont(struct job *);
int         job_fg(  struct job *j, struct job **jobs);
int         job_bg(  struct job *);

#define job_stop(j) job_sig( j, SIGSTOP)
#define job_cont(j) job_sig( j, SIGCONT)
#define job_bg(  j) job_cont(j)

#endif
