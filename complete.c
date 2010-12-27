#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "util.h"
#include "complete.h"
#include "esc.h"
#include "path.h"

#define CTRL_AND(k) (k - 'A' + 1)
#define AND_CTRL(k) (k + 'A' - 1)

void complete(char **, unsigned int *, unsigned int *, int *reprompt);
void complete_exe(char **bptr, unsigned int *sizptr, unsigned int *idxptr, int *reprompt);
void complete_arg(char **bptr, unsigned int *sizptr, unsigned int *idxptr, unsigned int startidx, int *reprompt);


char *ureadcomp()
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
			if(index == 0)
				return NULL;
			else
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
					/* FIXME: handle non ctrl_and chars */
					printf("^%c", AND_CTRL(c));

				if(index == siz){
					siz += 256;
					buffer = urealloc(buffer, siz);
				}
		}
	}while(1);
#endif
}

void complete(char **bptr, unsigned int *sizptr, unsigned int *idxptr, int *reprompt)
{
	char *buffer = *bptr;
	unsigned int index = *idxptr;
	int i;

	for(i = index; i >= 0; i--)
		if(buffer[i] == ' ' && (i > 0 ? buffer[i-1] != '\\' : 1))
			break;

	/* (buffer + i) is the fist previous-non-escaped space */
	if(i == -1)
		complete_exe(bptr, sizptr, idxptr, reprompt);
	else
		complete_arg(bptr, sizptr, idxptr, i, reprompt);
}

void complete_exe(char **bptr, unsigned int *sizptr, unsigned int *idxptr, int *reprompt)
{
	extern struct exe *exes;
	struct exe *iter, *first = NULL;
	const int len = *idxptr;
	char *buffer = *bptr;
	int multi = 0;

	(void)sizptr;

	for(iter = exes; iter; iter = iter->next)
		if(!strncmp(iter->basename, buffer, len)){
			if(multi){
				printf("%s\n", iter->basename);
			}else if(first){
				/* multiple possibilities, print them all */
				multi = 1;
				*reprompt = 1;
				putchar('\n');
				printf("%s\n", first->basename);
				printf("%s\n", iter->basename);
			}else
				first = iter;
		}

	if(!multi){
		if(first){
			/* single match - fill line */
			char *append;
			int appendlen, back;

			if(*sizptr <= strlen(first->basename))
				buffer = *bptr = urealloc(buffer, *sizptr = strlen(first->basename));

			append = first->basename + *idxptr;
			appendlen = strlen(append);
			strcpy(buffer + *idxptr, append);

			*idxptr += appendlen;
			buffer[*idxptr] = ' ';
			buffer[++*idxptr] = '\0';

			back = strlen(first->basename) - appendlen;
			while(back --> 0)
				putchar('\b');
			fputs(buffer, stdout);
		}else{
			/* no match - do nothing? */
		}
	}
}

void complete_arg(char **bptr, unsigned int *sizptr, unsigned int *idxptr, unsigned int startidx, int *reprompt)
{
	(void)bptr;
	(void)sizptr;
	(void)idxptr;
	(void)reprompt;
	(void)startidx;
	printf("\ncomplete_arg()\n");
}
