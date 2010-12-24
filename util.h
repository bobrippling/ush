#ifndef UTIL_H
#define UTIL_H

void *umalloc(size_t);
void *urealloc(void *, size_t);
char *ustrdup(const char *s);
char *ustrndup(const char *s, size_t);
char *ustrdup_argvp(char ***argvpp);
void  ufree_argvp(char ***argvp);
const char *usignam(unsigned int);

#endif
