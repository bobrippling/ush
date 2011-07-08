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
	struct redir *redir;

	int tconf_got;
	struct termios tconf;


	struct job *next; /* link-a-list helper */
};

struct job *job_new(char ***argvpp, int jid, struct redir *);

struct proc *job_first_proc(struct job *);

int         job_start(    struct job *);
int         job_wait(     struct job *, int async);
int         job_wait_all( struct job *j, struct job **jobs, int async);
int         job_check_all(struct job **jobs);
int         job_complete( struct job *);

void        job_free(struct job *j);

const char *job_state_name(struct job *);

int         job_sig( struct job *, int sig);

#endif
