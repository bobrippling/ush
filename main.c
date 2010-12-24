#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <termios.h>

#undef ARGV_PRINT

#ifdef ARGV_PRINT
# include <stdlib.h>
#endif

#include "util.h"
#include "proc.h"
#include "job.h"
#include "task.h"
#include "path.h"
#include "readline.h"
#include "term.h"
#include "config.h"


jmp_buf allocerr;
volatile int got_sigchld = 0;
struct task *tasks = NULL;

int lewp()
{
	char ****argvpp;

	do{
		struct task *t;
		int eof;

		argvpp = ureadline(&eof);
		if(eof)
			/* FIXME: kill children */
			break;
		else if(!argvpp)
			continue;

#ifdef ARGV_PRINT
		{
			int i, j, k;
			for(i = 0; argvpp[i]; i++)
				for(j = 0; argvpp[i][j]; j++)
					for(k = 0; argvpp[i][j][k]; k++)
						printf("argvpp[%d][%d][%d]: \"%s\"\n", i, j, k, argvpp[i][j][k]);
		}
#endif

		t = task_new(argvpp);
		t->next = tasks;
		tasks = t;

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

	setbuf(stdout, NULL);

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
