#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <ctype.h>

#include "complete.h"
#include "readline.h"
#include "parse.h"
#include "util.h"
#include "task.h"
#include "esc.h"
#include "path.h"
#include "glob.h"
#include "config.h"

#define BSIZ 256

#define CTRL_AND(k) (k - 'A' + 1)
#define AND_CTRL(k) (k + 'A' - 1)

static char *prompt_and_line(void);
static char *uread_and_comp(void);


struct parsed *ureadline(int *eof)
{
	struct parsed *ret;
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
		ret = NULL;
	else
		ret = parse(buffer);
	free(buffer);

	glob_expand(ret);
	return ret;
}

static char *prompt_and_line()
{
	char *buffer;
retry:
	if(!(buffer = uread_and_comp())){
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
			}

			perror("ush: read()");
		}else
			putchar('\n');
		return NULL;
	}
	return buffer;
}

static char *uread_and_comp()
{
#ifdef READ_SIMPLE
	char *buffer = umalloc(256);
	fputs("% ", stdout);
	if(fgets(buffer, 256, stdin))
		return buffer;
	free(buffer);
	return NULL;
#else
	unsigned int siz = 256, index = 0;
	int reprompt = 0;
	char *buffer = umalloc(siz);
	*buffer = '\0';

reprompt:
	printf("%% %s", buffer);

	do{
		int c = getchar();

		if(c == CTRL_AND('D')){
			if(index == 0){
				free(buffer);
				return NULL;
			}else
				c = '\t';
		}

		switch(c){
			case '\b':   /* ^H aka CTRL_AND('H') */
			case '\177': /* ^? */
				if(index > 0){
					putchar('\b');
					if(!isprint(buffer[--index]))
						putchar('\b');
					esc_clrtoeol();
				}
				break;

			case '\t':
				buffer[index] = '\0';
				complete(&buffer, &siz, &index, &reprompt);
				if(reprompt){
					reprompt = 0;
					buffer[index] = '\0';
					goto reprompt;
				}
				break;

			case CTRL_AND('V'):
				/*
				 * TODO
				 * read a seq of 3 digits (oct char code)
				 * etc
				 */
				break;

			case CTRL_AND('U'):
				index = 0;
				*buffer = '\0';
				putchar('\r');
				esc_clrtoeol();
				goto reprompt;

			case '\n':
				buffer[index] = '\0';
				putchar('\n');
				return buffer;

			default:
				buffer[index++] = c;

				if(isprint(c))
					putchar(c);
				else
					/* FIXME: handle non ctrl_and chars? */
					printf("^%c", AND_CTRL(c));

				if(index == siz){
					siz += 256;
					buffer = urealloc(buffer, siz);
				}
		}
	}while(1);
#endif
}

