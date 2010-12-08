#include <unistd.h>
#include <termios.h>
#include <stdio.h>

#include "term.h"

struct termios attr_orig;


int term_init(void)
{
	if(tcgetattr(STDIN_FILENO, &attr_orig)){
		perror("tcgetattr()");
		return 1;
	}
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

	return 0;
}
