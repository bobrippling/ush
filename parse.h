#ifndef PARSE_H
#define PARSE_H

struct parsed
{
	char ***argvp;
	struct redir
	{
		int fd_in, fd_out;
		char *fname;
		struct redir *next;
	} *redir;

	struct parsed *next;
	/*
	 * .argvp = { { "echo", "hi", NULL }, { "grep", "i", NULL } }
	 * .redirs =
	 *
	 *   e.g. " > /dev/null 2>&1"
	 *   gives
	 *   { .fd_in = 1, .fd_out = -1, .fname = "/dev/null" }
	 *   { .fd_in = 2, .fd_out =  1, .fname =  NULL }
	 *
	 */
};

struct parsed *parse(char *in);

#endif
