#ifndef PROC_H
#define PROC_H

struct job
{
	struct job *next;

	struct termios termmode;

	struct process
	{
		struct process *next;
		char **argv;
		int in, out, err;
		enum procstate { INIT, BG, FG, DONE } state;
		pid_t pid;
	} *proc; /* list */

	int exitcode, exitsig;
	int pgid, infile, outfile;

	char notified;
} *job_new(struct process *proclist, int infile, int outfile);
void job_free(struct job *);

void job_waitfor(struct job *, char block);
int job_spawn(struct job *, char bg);

void job_background(struct job *, char resume);
void job_foreground(struct job *, char resume);

struct job *job_find(struct job *list, pid_t pid);

char job_isdone(struct job *);
char job_isbackground(struct job *);

struct process *proc_new(char **, int files[3]);
void proc_free(struct process *);

#endif
