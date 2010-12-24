#ifndef UTIL_H
#define UTIL_H

void *umalloc(size_t);
void *urealloc(void *, size_t);
char *ustrdup(const char *s);
char *ustrdup_printf(const char *, ...);
char *ustrndup(const char *s, size_t);
char *ustrdup_argvp(char ***argvpp);
void  ufree_argvp(char ***argvp);

const char *usignam(unsigned int);

int block_set(int fd, int block);
int block_get(int fd);

unsigned int ugetuid(void), ugetgid(void);

#endif
