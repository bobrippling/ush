#ifndef PROC_H
#define PROC_H

struct proc
{
	char **argv;
	pid_t pid;
	enum { PROC_SPAWN, PROC_RUN, PROC_STOP, PROC_FIN } state;
	int in, out, err;

	int exit_code, exit_sig;

	struct proc *next;
};

struct proc *proc_new(char **argv);
int          proc_exec(struct proc *, int pgid);
void         proc_free(struct proc *);
int          proc_argc(struct proc *);
char        *proc_desc(struct proc *);

#endif
