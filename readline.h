#ifndef READLINE_H
#define READLINE_H

char ****ureadline(int *eof);
/*
 * returns this:
 *
 * {
 *   { { "echo", "hi", NULL }, { "grep", "i", NULL } },
 *   { { "printf", "%d", "yo" } }
 * }
 */

#endif
