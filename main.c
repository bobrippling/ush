#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#include "util.h"
#include "job.h"
#include "proc.h"
#include "readline.h"
#include "term.h"
#include "config.h"

#undef ARGV_PRINT

jmp_buf allocerr;

struct job *jobs = NULL;

void rm_job(struct job *j)
{
	if(j == jobs){
		jobs = j->next;
		job_free(j);
	}else{
		struct job *prev;

		for(prev = jobs; prev->next; prev = prev->next)
			if(j == prev->next){
				prev->next = j->next;
				job_free(j);
				break;
			}
	}
}

int lewp()
{
	char ***argvp;

	do{
		struct job *j;
		int eof;

		argvp = ureadline(&eof);
		if(eof)
			break;
		else if(!argvp)
			continue;

#ifdef ARGV_PRINT
		{
			int i, j;
			for(i = 0; argvp[i]; i++)
				for(j = 0; argvp[i][j]; j++)
					printf("argvp[%d][%d]: \"%s\"\n", i, j, argvp[i][j]);
			ufree_argvp(argvp);
		}
#else
		j = job_new(ustrdup_argvp(argvp), argvp);
		j->next = jobs;
		jobs = j;

		if(job_start(j))
			perror("job_start()");
			/* FIXME: cleanup */
		else if(job_wait_all(j) && errno != ECHILD)
			perror("job_wait_all()");
			/* FIXME: cleanup */

		rm_job(j);
#endif
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

	term_init();
	ret = lewp();
	term_term();

	return ret;
}
