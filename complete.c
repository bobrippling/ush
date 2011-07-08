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

static int complete_filter(const struct dirent *ent)
{
	/* return ~0 to add */
	return ent->d_name[0] != '.' && !strncmp(ent->d_name, global_basename, global_basename_len);
}

static void complete_best_match(char **ents, int nmatches, unsigned int *bestlen)
{
	int minlen = INT_MAX, i;

	for(i = 0; i < nmatches; i++){
		const int len = strlen(ents[i]);
		if(len < minlen)
			minlen = len;
	}

	for(i = 0; i < minlen; i++){
		char c = ents[0][i];
		int j;

		for(j = 1; j < nmatches; j++)
			if(ents[j][i] != c){
				*bestlen = i;
				return;
			}
	}

	*bestlen = minlen;
}

void complete_exe(char **bptr, unsigned int *sizptr, unsigned int *idxptr, int *reprompt)
{
	extern struct exe *exes;
	struct exe *iter;
	char *buffer = *bptr;
	int i;
	const unsigned int len = *idxptr;
	unsigned int bestlen;

	if(strchr(buffer, '/')){
		complete_arg(bptr, sizptr, idxptr, 0, reprompt);
	}else{
		char **ents = umalloc(path_count() * sizeof(char *));

		for(i = 0, iter = exes; iter; iter = iter->next)
			if(!strncmp(iter->basename, buffer, len))
				ents[i++] = iter->basename;

		switch(i){
			case 1:
				complete_to(ents[0], strlen(buffer), strlen(ents[0]), bptr, sizptr, idxptr, ' ');
			case 0:
				break;

			default:
				complete_best_match(ents, i, &bestlen);

				if(bestlen > strlen(buffer)){
					complete_to(ents[0], strlen(buffer), bestlen, bptr, sizptr, idxptr, 0);
				}else{
					int j;
					*reprompt = 1;
					putchar('\n');
					for(j = 0; j < i; j++)
						printf("%s\n", ents[j]);
				}
		}

		free(ents);
	}
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
			/* multiple */
			unsigned int bestmatchlen;
			char **ents = umalloc(nmatches * sizeof(char *));
			int i;

			for(i = 0; i < nmatches; i++)
				ents[i] = list[i]->d_name;

			complete_best_match(ents, nmatches, &bestmatchlen);
			free(ents);

			if(global_basename_len == bestmatchlen){
				/* already full, show them */
				int i;

				*reprompt = 1;
				putchar('\n');
				for(i = 0; i < nmatches; i++)
					printf("%s\n", list[i]->d_name);
			}else
				complete_to(list[0]->d_name, global_basename_len, bestmatchlen, bptr, sizptr, idxptr, 0);

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
