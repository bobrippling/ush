#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <sys/wait.h>

#include "util.h"
#include "job.h"
#include "proc.h"
#include "readline.h"
#include "term.h"

jmp_buf allocerr;

struct job *jobs = NULL;

int lewp()
{
	char **argvp[] = { NULL, NULL };

	do{
		struct job *j;
		char **argv = ureadline();

		if(!argv)
			break;

		argvp[0] = argv;

		j = job_new(ustrdup_argv(argv), argvp);
		j->next = jobs;
		jobs = j;

		if(job_start(j))
			perror("job_start()");
			/* FIXME: cleanup */
		else if(job_wait_all(j))
			perror("job_wait_all()");
			/* FIXME: cleanup */
	}while(1);

	return 0;
}

void sigh(int sig)
{
	switch(sig){
		case SIGCHLD:
			;
	}
}

int main(int argc, const char **argv)
{
	int ret;

	if(setjmp(allocerr)){
		perror("malloc()");
		return 1;
	}

	(void)argc;
	(void)argv;

	signal(SIGCHLD, sigh);

	term_init();
	ret = lewp();
	term_term();

	return ret;
}
