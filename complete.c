#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

#include "util.h"
#include "path.h"

void complete_exe(char **bptr, unsigned int *sizptr, unsigned int *idxptr, int *reprompt);
void complete_arg(char **bptr, unsigned int *sizptr, unsigned int *idxptr, unsigned int startidx, int *reprompt);

static int find_gap(char *buffer, int index);

static const char   *global_basename = NULL;
static unsigned int  global_basename_len = 0;

static int find_gap(char *buffer, int index)
{
	int i;

	for(i = index; i >= 0; i--)
		if(buffer[i] == ' ' && (i > 0 ? buffer[i-1] != '\\' : 1))
			break;

	return i;
}

static void complete_to(char *to, int len_so_far, int to_len,
		char **bptr, unsigned int *sizptr, unsigned int *idxptr,
		char appendchar)
{
	char *buffer = *bptr;

	if(*sizptr <= strlen(*bptr) + strlen(to))
		buffer = *bptr = urealloc(buffer, *sizptr = strlen(to) + strlen(*bptr) + 1);

	strncpy(buffer + *idxptr, to + len_so_far, to_len - len_so_far);
	fwrite(to + len_so_far, sizeof(char), to_len - len_so_far, stdout);

	*idxptr += to_len - len_so_far;
	if(appendchar){
		putchar(appendchar);

		buffer[*idxptr] = appendchar;
		++*idxptr;
	}
	buffer[*idxptr] = '\0';
}

void complete(char **bptr, unsigned int *sizptr, unsigned int *idxptr, int *reprompt)
{
	int i = find_gap(*bptr, *idxptr);

	/* (buffer + i) is the fist previous-non-escaped space */
	if(i == -1)
		complete_exe(bptr, sizptr, idxptr, reprompt);
	else
		complete_arg(bptr, sizptr, idxptr, i + 1, reprompt);
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
			complete_to(first->basename, len, strlen(first->basename), bptr, sizptr, idxptr, ' ');
		}else{
			/* no match - do nothing? */
		}
	}
}

static int complete_filter(const struct dirent *ent)
{
	/* return ~0 to add */
	return ent->d_name[0] != '.' && !strncmp(ent->d_name, global_basename, global_basename_len);
}

static int complete_best_match(struct dirent **ents, int nmatches, unsigned int *bestlen)
{
	int minlen = INT_MAX, min = -1, i;

	for(i = 0; i < nmatches; i++){
		const int len = strlen(ents[i]->d_name);

		if(len < minlen){
			minlen = len;
			min = i;
		}
	}

	for(i = 0; i < minlen; i++){
		char c = ents[0]->d_name[i];
		int j;

		for(j = 1; j < nmatches; j++)
			if(ents[j]->d_name[i] != c){
				*bestlen   = i;
				return 0;
			}
	}

	if(i == minlen){
		*bestlen = minlen;
		return 0;
	}else
		return 1;
}

void complete_arg(char **bptr, unsigned int *sizptr, unsigned int *idxptr, unsigned int startidx, int *reprompt)
{
	int len = *idxptr - startidx;
	char *arg = alloca(len + 1), *lastsep;
	const char *dirname;
	struct dirent **list = NULL;
	int nmatches;

	(void)reprompt;

	/* word begins at startidx and lasts until *idxptr */
	strncpy(arg, *bptr + startidx, len);
	arg[len] = '\0';

	lastsep = strrchr(arg, '/');
	if(lastsep){
		global_basename = lastsep + 1;
		dirname = arg;
		*lastsep = '\0';
		if(!*dirname)
			dirname = "/";
	}else{
		global_basename = arg;
		dirname = ".";
	}
	global_basename_len = strlen(global_basename);

	nmatches = scandir(dirname, &list, &complete_filter, &alphasort);

	switch(nmatches){
		case  0: /* none */
		case -1: /* error */
			break;

		default:
		{
			unsigned int bestmatchlen;
			/* multiple */

			if(!complete_best_match(list, nmatches, &bestmatchlen)){
				if(global_basename_len == bestmatchlen){
					/* already full, show them */
					int i;

					*reprompt = 1;
					putchar('\n');
					for(i = 0; i < nmatches; i++)
						printf("%s\n", list[i]->d_name);
				}else
					complete_to(list[0]->d_name, global_basename_len, bestmatchlen, bptr, sizptr, idxptr, 0);
			}
			break;
		}

		case 1:
		{
			char *fullpath = alloca(strlen(dirname) + strlen(list[0]->d_name) + 2);
			char appendchar = ' ';
			struct stat st;

			sprintf(fullpath, "%s/%s", dirname, list[0]->d_name);

			if(!stat(fullpath, &st) && S_ISDIR(st.st_mode))
				appendchar = '/';

			complete_to(list[0]->d_name, global_basename_len, strlen(list[0]->d_name),
					bptr, sizptr, idxptr, appendchar);
			break;
		}
	}


	for(len = 0; len < nmatches; len++)
		free(list[len]);
	free(list);
}
