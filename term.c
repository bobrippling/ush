#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

#include "term.h"

struct termios attr_orig;


int term_init(void)
{
	int pgid = getpgrp();

	if(tcgetattr(STDIN_FILENO, &attr_orig)){
		perror("tcgetattr()");
		return 1;
	}

	while(tcgetpgrp(STDIN_FILENO) != (pgid = getpgrp()))
		kill(-pgid, SIGTTIN);

	/* foreground! */

	signal(SIGINT,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	/*signal(SIGCHLD, SIG_IGN);*/

	/* Put ourselves in our own process group.  */
	if(setpgid(getpid(), pgid) < 0){
		perror("setpgid()");
		return 1;
	}

	tcsetpgrp(STDIN_FILENO, pgid);

	return 0;
}

int term_term(void)
{
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr_orig)){
		perror("tcsetattr()");
		return 1;
	}

	return 0;
}

int term_fg_proc(pid_t pid)
{
	/*
	 * http://www.gnu.org/s/libc/manual/html_node/Implementing-a-Shell.html#Implementing-a-Shell
	 */
	(void)pid;

	return 0;
}
