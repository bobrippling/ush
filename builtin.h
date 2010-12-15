#ifndef BUILTIN_H
#define BUILTIN_H

#define BUILTIN(n) int n(int argc, char **argv)

BUILTIN(echo);
BUILTIN(ls);
BUILTIN(ps);
BUILTIN(kill);

#undef BUILTIN

#endif
