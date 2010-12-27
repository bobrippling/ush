#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "util.h"
#include "path.h"

struct exe *exes = NULL;

static int can_exe(struct stat *st)
{
#define m st->st_mode
	if(S_ISDIR(st->st_mode))
		return 0;
	if(st->st_uid == ugetuid() && m & S_IXUSR)
		return 1;
	else if(st->st_gid == ugetgid() && m & S_IXGRP)
		return 1;
	else if(m & S_IXOTH)
		return 1;
	return 0;
#undef m
}

void path_init()
{
	const char *path = getenv("PATH");
	char *dup, *iter;
	struct exe **pexe = &exes;

	if(!path)
		path = "/sbin:/bin:/usr/sbin:/usr/bin";

	dup = ustrdup(path);

	for(iter = strtok(dup, ":"); iter; iter = strtok(NULL, ":")){
		DIR *d;

		d = opendir(iter);

		if(d){
			struct dirent *ent;

			while((ent = readdir(d))){
				char *path = ustrdup_printf("%s/%s", iter, ent->d_name);
				struct stat st;

				if(!stat(path, &st) && can_exe(&st)){
					struct exe *new;

					*pexe = new = umalloc(sizeof *new);
					pexe = &new->next;

					new->next = NULL;
					new->path = path;
					if(!(new->basename = strrchr(new->path, '/')))
						new->basename = new->path;
					else
						new->basename++;
				}else
					free(path);
			}
			closedir(d);
		}
	}
}

void path_rehash()
{
	path_term();
	path_init();
}

void path_term()
{
	struct exe *iter, *del;
	for(iter = exes;
		iter;
		del = iter, iter = iter->next, free(del->path), free(del));
}
