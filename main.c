#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/types.h>

#include "util.h"
#include "proc.h"
#include "job.h"
#include "parse.h"
#include "task.h"
#include "path.h"
#include "readline.h"
#include "term.h"
#include "config.h"


jmp_buf allocerr;
volatile int got_sigchld = 0;
struct task *tasks = NULL;
int dumb_term = 0;

int lewp()
{
	struct parsed *prog = NULL;

	do{
		struct task *t;
		int eof;

		prog = ureadline(&eof);
		if(eof)
			/* FIXME: kill children - "jobs are running... better go and catch them!" */
			break;
		else if(!prog)
			continue;

		t = task_new(prog);
		t->next = tasks;
		tasks = t;

		{
			struct parsed *iter, *del;
			for(iter = prog; iter; del = iter, iter = iter->next, free(del));
		}

		if(task_start(t))
			perror("task_start()");

		task_wait(&tasks, t, 0);
		while(task_check_all(&tasks));
	}while(1);

	return 0;
}

int shell_login()
{
	fprintf(stderr, "TODO: source ~/.ush_profile\n"); /* TODO */
	return 0;
}

int shell_rc()
{
	fprintf(stderr, "TODO: source ~/.ushrc\n"); /* TODO */
	return 0;
}

void sigh(int sig)
{
	if(sig == SIGCHLD)
		got_sigchld = 1;
}

int main(int argc, const char **argv)
{
	int ret, i, login = argv[0][0] == '-';

	signal(SIGCHLD, sigh);

	if(setjmp(allocerr)){
		perror("malloc()");
		return 1;
	}

#define ARG(x) !strcmp(argv[i], "-" x)

	for(i = 1; i < argc; i++)
		if(ARG("l"))
			login = 1;
		else{
			fprintf(stderr, "Usage: %s [-l]\n"
					            "  -l: login shell\n", *argv);
			return 1;
		}

	if(term_init()){
		fprintf(stderr, "%s: aborting - couldn't initialise terminal\n",
				*argv);
		return 1;
	}

	path_init();

	if(login && shell_login())
		return 1;
	if(shell_rc())
		return 1;

	ret = lewp();

	path_term();
	term_term();

	return ret;
}
