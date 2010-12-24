#ifndef TERM_H
#define TERM_H

int term_init(void);
int term_term(void);

void term_give_to(pid_t);

void term_attr_orig(void);

void term_attr_set(struct termios *t);
void term_attr_get(struct termios *t);

#endif
