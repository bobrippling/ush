#ifndef PROC_H
#define PROC_H

struct proc
{
	char **argv;
	pid_t pid;
	enum { SPAWN, FG, BG, FIN } state;
	int exit_status;
	int in, out, err;

	struct proc *next;
};

struct proc *proc_new(char **argv);
int          proc_spawn(struct proc *, int pgid);
void         proc_free( struct proc *);

#endif
