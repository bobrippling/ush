#define _POSIX_SOURCE

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "term.h"

/*
 * process group = job
 *
 *
 *                    / cat hi
 *             job1  <
 *           /        \ grep tim
 *          /
 * session <
 *          \
 *           \        / find . -iname hi
 *             job2  <
 *                    \ xargs rm
 *
 */

pid_t pgid;
struct termios shell_termmode;
extern char interactive;

int term_init()
{
	if(interactive){
		/*
		 * wait until foreground'd (in case invoked from another shell, like
		 * ush&
		 */
		pgid = getpgrp();
		while(tcgetpgrp(terminal) != pgid)
			kill(-pgid, SIGTTIN); /* all processes in this job */

		/* interactive and job-control sigs */
		signal(SIGINT , SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		signal(SIGCHLD, SIG_IGN);

		/* own process group */
		pgid = getpid();
		if(setpgid(pgid, pgid) < 0){
			perror("setpgid()");
			return 0;
		}

		/* set our process group as the term controlling process */
		tcsetpgrp(terminal, pgid);

		/* save term attrs (in case curses app does something) */
		tcgetattr(terminal, &tmodes);
	}

	return 1;
}

int term_term()
{
	tcsetattr(terminal, TCSANOW, &tmodes);
	return 1;
}
