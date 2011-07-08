#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include "term.h"
#include "config.h"

struct termios attr_orig, attr_ush;

void term_raw(int);

int term_init(void)
{
	int pgid = getpgrp();

	if(tcgetattr(STDIN_FILENO, &attr_orig)){
		extern int interactive;
		perror("tcgetattr()");
		fputs("job control disabled\n", stderr);
		interactive = 0;
		return 0;
	}

	memcpy(&attr_ush, &attr_orig, sizeof attr_ush);

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

	term_raw(1);

	return 0;
}

int term_term(void)
{
	term_raw(0);

	term_attr_set(&attr_orig);

	return 0;
}

void term_attr_get(struct termios *t)
{
	if(tcgetattr(STDIN_FILENO, t)){
		perror("tcgetattr()");
		memset(t, 0, sizeof *t);
	}
}

void term_attr_set(struct termios *t)
{
	int n = 0;
start:
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, t)){
		if(errno == EINTR && n++ < 10)
			goto start;
		perror("tcsetattr()");
	}
}

void term_attr_orig(void)
{
	term_attr_set(&attr_orig);
}

void term_attr_ush(void)
{
	term_attr_set(&attr_ush);
}

void term_give_to(pid_t gid)
{
	if(tcsetpgrp(STDIN_FILENO, gid))
		perror("term_give_to()");
}

void term_raw(int on)
{
#ifndef READ_SIMPLE
	if(on){
		attr_ush.c_cc[VMIN]  = sizeof(char);
		attr_ush.c_lflag    &= ~(ICANON | ECHO);
	}else{
		attr_ush.c_cc[VMIN]  = 0;
		attr_ush.c_lflag    |= ICANON | ECHO;
	}
	term_attr_ush();
#endif
}
