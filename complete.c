#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "complete.h"
#include "esc.h"

#define CTRL_AND(k) (k - 'a' + 1)

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
	int siz = 256, index = 0;
	char *buffer = umalloc(siz);

	fputs("% ", stdout);

	do{
		int c = getchar();

		if(c == CTRL_AND('d')){
			if(index == 0)
				return NULL;
			else
				c = '\t';
		}

		switch(c){
			case '\b':   /* ^H aka CTRL_AND('h') */
			case '\177': /* ^? */
				if(index > 0){
					index--;
					putchar('\b');
					esc_clrtoeol();
				}
				break;

			case '\t':

				break;

			case '\n':
				buffer[index] = '\0';
				putchar('\n');
				return buffer;

			default:
				buffer[index++] = c;
				putchar(c);
				if(index == siz){
					siz += 256;
					buffer = urealloc(buffer, siz);
				}
		}
	}while(1);
#endif
}
