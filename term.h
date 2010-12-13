#ifndef TERM_H
#define TERM_H

int term_init(void);
int term_term(void);

void term_attr_set(struct termios *);
void term_attr_orig(void);

#endif
