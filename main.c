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
#include "job.h"
#include "proc.h"
#include "readline.h"
#include "term.h"
#include "config.h"


jmp_buf allocerr;

struct job *jobs = NULL;

int lewp()
{
	char ****argvpp;

	do{
		struct job *j;
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

		j = job_new(argvpp);
		j->next = jobs;
		jobs = j;

		if(job_start(j))
			perror("job_start()");

		job_wait_all(j, &jobs, 0);
		job_check_all(&jobs);
	}while(1);

	return 0;
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

	if(term_init()){
		fprintf(stderr, "%s: aborting - couldn't initialise terminal\n",
				*argv);
		return 1;
	}
	ret = lewp();
	term_term();

	return ret;
}
