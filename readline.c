#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>

#include "complete.h"
#include "readline.h"
#include "parse.h"
#include "util.h"
#include "task.h"
#include "config.h"

#define BSIZ 256

static char *prompt_and_line(void);


char ****ureadline(int *eof)
{
	char ****ret;
	char *buffer, *nl;

	*eof = 0;

	buffer = prompt_and_line();
	if(!buffer){
		*eof = 1;
		return NULL;
	}

	nl = strchr(buffer, '\n');
	if(nl)
		*nl = '\0';

	if(*buffer == '\0')
		return NULL;

	ret = parse(buffer);
	free(buffer);
	return ret;
}

static char *prompt_and_line()
{
	char *buffer;
retry:
	if(!(buffer = ureadcomp())){
		if(ferror(stdin)){
			switch(errno){
				case EINTR:
					goto retry;

				case EAGAIN:
					if(block_set(0, 1)){
						perror("block()");
						return NULL;
					}
					clearerr(stdin);
					goto retry;

				default:
					break;
			}

			perror("ush: read()");
		}else
			putchar('\n');
		free(buffer);
		return NULL;
	}
	return buffer;
}
