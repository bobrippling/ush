#ifndef UTIL_H
#define UTIL_H

void *umalloc(size_t);
char *ustrdup(const char *s);
char *ustrdup_argvp(char ***argvp);

#endif
