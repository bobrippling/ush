#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "util.h"
#include "path.h"

void complete_exe(char **bptr, unsigned int *sizptr, unsigned int *idxptr, int *reprompt);
void complete_arg(char **bptr, unsigned int *sizptr, unsigned int *idxptr, unsigned int startidx, int *reprompt);

static int find_gap(char *buffer, int index);

static int find_gap(char *buffer, int index)
{
	int i;

	for(i = index; i >= 0; i--)
		if(buffer[i] == ' ' && (i > 0 ? buffer[i-1] != '\\' : 1))
			break;

	return i;
}

static void complete_to(char *to, int len_so_far,
		char **bptr, unsigned int *sizptr, unsigned int *idxptr,
		char appendchar)
{
	char *buffer = *bptr;
	char *append;
	int appendlen;

	if(*sizptr <= strlen(*bptr) + strlen(to))
		buffer = *bptr = urealloc(buffer, *sizptr = strlen(to) + strlen(*bptr) + 1);

	append = to + len_so_far;
	appendlen = strlen(append);
	strcpy(buffer + *idxptr, append);

	*idxptr += appendlen;
	buffer[*idxptr] = appendchar;
	buffer[++*idxptr] = '\0';

	fputs(to + len_so_far, stdout);
	putchar(appendchar);
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
			complete_to(first->basename, len, bptr, sizptr, idxptr, ' ');
		}else{
			/* no match - do nothing? */
		}
	}
}

void complete_arg(char **bptr, unsigned int *sizptr, unsigned int *idxptr, unsigned int startidx, int *reprompt)
{
	int len = *idxptr - startidx, basename_len;
	char *arg = alloca(len + 1), *lastsep, *basename;
	const char *dirname;
	DIR *d;

	/* word begins at startidx and lasts until *idxptr */
	strncpy(arg, *bptr + startidx, len);
	arg[len] = '\0';

	lastsep = strrchr(arg, '/');
	if(lastsep){
		basename = lastsep + 1;
		dirname = arg;
		*lastsep = '\0';
		if(!*dirname)
			dirname = "/";
	}else{
		basename = arg;
		dirname = ".";
	}
	basename_len = strlen(basename);

	fprintf(stderr, "opendir(): %s\n", dirname);
	d = opendir(dirname);

	if(d){
		struct dirent *ent;
		char *first = NULL; /* dup of the first one */
		int more = 0;

		while((ent = readdir(d)))
			if(strcmp(ent->d_name, "..") &&
					strcmp(ent->d_name, ".") &&
					!strncmp(ent->d_name, basename, basename_len)){

					/*
					 * TODO: if we have { "parse.c", "parse.h", "parse.o" },
					 * complete up to "parse."
					 */
					if(more){
						printf("%s\n", ent->d_name);
					}else if(first){
						more = 1;
						printf("\n%s\n", first);
						printf("%s\n", ent->d_name);
						*reprompt = 1;
					}else{
						first = alloca(strlen(ent->d_name) + 1);
						strcpy(first, ent->d_name);
					}
				}

		closedir(d);

		if(!more){
			if(first){
				char *fullpath = alloca(strlen(dirname) + strlen(first) + 2);
				char appendchar = ' ';
				struct stat st;

				sprintf(fullpath, "%s/%s", dirname, first);

				if(!stat(fullpath, &st) && S_ISDIR(st.st_mode))
					appendchar = '/';

				complete_to(first, basename_len, bptr, sizptr, idxptr, appendchar);
			}else{
				/* no entries found */
			}
		}
	}
}
