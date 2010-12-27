#ifndef PATH_H
#define PATH_H

void path_init(void);
void path_rehash(void);
void path_term(void);
int  path_count(void);

struct exe
{
	char *path;
	char *basename;

	struct exe *next;
};

#endif
