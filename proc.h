#ifndef PROC_H
#define PROC_H

struct job
{
	struct termios termmode;

	struct process
	{
		struct process *next;
		char **argv;
		int in, out, err;
	} *proc; /* list */

	int pgid, infile, outfile;
};

int job_spawn(struct job *, char bg);

void job_background(struct job *, char resume);
void job_foreground(struct job *, char resume);

#endif
