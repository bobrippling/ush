#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

#include "term.h"
#include "config.h"

#ifdef USH_DEBUG
# define TERM_DEBUG
#else
# undef TERM_DEBUG
#endif

struct termios attr_orig;


int term_init(void)
{
	int pgid = getpgrp();

#ifdef TERM_DEBUG
	fprintf(stderr, "tcgetattr(stdin, ...)\n");
#endif

	if(tcgetattr(STDIN_FILENO, &attr_orig)){
		perror("tcgetattr()");
		return 1;
	}

	while(tcgetpgrp(STDIN_FILENO) != (pgid = getpgrp()))
		kill(-pgid, SIGTTIN);

	/* foreground! */

	signal(SIGINT,  SIG_IGN);
	/*signal(SIGQUIT, SIG_IGN);*/
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
#ifdef TERM_DEBUG
	fprintf(stderr, "tcsetattr(stdin, TCSAFLUSH, ...)\n");
#endif

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr_orig)){
		perror("tcsetattr()");
		return 1;
	}

	return 0;
}

void term_attr_set(struct termios *t)
{
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, t))
		perror("tcsetattr()");
}

void term_attr_orig(void)
{
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr_orig))
		perror("tcsetattr()");
}
