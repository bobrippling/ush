#ifndef TASK_H
#define TASK_H

struct task
{
	char *cmd;
	struct job *jobs;
	enum { TASK_BEGIN, TASK_RUNNING, TASK_COMPLETE } state;

	struct task *next;
};

struct task *task_new(char ****argvpp);

int          task_start(struct task *);

int          task_wait(     struct task **, struct task *, int async);
int          task_check_all(struct task **);

int          task_sig(struct task *, int);
void         task_fg(struct task *);

const char  *task_state_name(struct task *);

#endif
