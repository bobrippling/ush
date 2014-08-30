#define _XOPEN_SOURCE
#include <wordexp.h>
#include <string.h>
#include <stdlib.h>

#include <stdio.h>

#include "parse.h"
#include "util.h"
#include "glob.h"

int ush_wordexp(char *s, wordexp_t *exp, int flags)
{
	int ret = wordexp(s, exp, flags);

	if(ret == 0){
		/* TODO: $(cmd), `cmd` */
	}

	return ret;
}

void glob_expand_argv(char ***pargv)
{
	char **argv;
	unsigned int i;
	unsigned int len;

	argv = *pargv;
	len = null_array_len(argv);

	for(i = 0; i < len; i++){
		wordexp_t exp = { 0 };
		int ret;

		ret = ush_wordexp(argv[i], &exp, 0);

		switch(ret){
			case 0:
			if(exp.we_wordc){
				unsigned int j;

				len += exp.we_wordc - 1;
				argv = urealloc(argv, (len+1) * sizeof(*argv));

				/*
				 * move from argv[i + 1] to argv[i + 1 + exp.we_wordc - 1]
				 */
				memmove(&argv[i + exp.we_wordc], &argv[i + 1], sizeof(*argv));

				for(j = 0; j < exp.we_wordc; j++)
					argv[i + j] = ustrdup(exp.we_wordv[j]);

				i += exp.we_wordc - 1; /* skip over inserted entries */
			}
			break;

			case WRDE_BADCHAR: /* Illegal occurrence of newline or one of |, &, ;, <, >, (, ), {, } */
			case WRDE_BADVAL:  /* An undefined shell variable was referenced, and the WRDE_UNDEF flag told us to consider this an error */
			case WRDE_CMDSUB:  /* Command substitution occurred, and the WRDE_NOCMD flag told us to consider this an error */
			case WRDE_NOSPACE: /* ENOMEM */
			case WRDE_SYNTAX:  /* Shell syntax error, such as unbalanced parentheses or unmatched quotes */
				break;
		}

		wordfree(&exp);
	}

	*pargv = argv;
}

void glob_expand(struct parsed *prog)
{
	int argvp_idx;

	for(argvp_idx = 0; prog->argvp[argvp_idx]; argvp_idx++)
		glob_expand_argv(&prog->argvp[argvp_idx]);
}
