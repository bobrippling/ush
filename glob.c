#define _XOPEN_SOURCE
#include <wordexp.h>
#include <string.h>

#include <stdio.h>

#include "parse.h"
#include "util.h"
#include "glob.h"

void glob_expand(struct parsed *prog)
{
	int argvp_idx;

	return;

	for(argvp_idx = 0; prog->argvp[argvp_idx]; argvp_idx++){
		int argv_idx;

		for(argv_idx = 0; prog->argvp[argvp_idx][argv_idx]; argv_idx++){
			wordexp_t exp;
			int ret = wordexp(prog->argvp[argvp_idx][argv_idx], &exp, WRDE_NOCMD /*FIXME*/);

			switch(ret){
				case 0:
				{
					int newsize = null_array_len(prog->argvp) + 1 + exp.we_wordc, diff;
					int i;
					char **a = prog->argvp[argvp_idx];

					char **iter;

					a = prog->argvp[argvp_idx] = urealloc(a, newsize * sizeof(*a));

					for(iter = a; *iter; iter++)
						fprintf(stderr, "iter: \"%s\"\n", *iter);

					diff = newsize - argvp_idx;
					fprintf(stderr, "diff: %d\n", diff);
					for(i = newsize - 1; i > argvp_idx; i--)
						a[i] = a[i - diff];

					for(i = argvp_idx; i < newsize; i++)
						a[i] = ustrdup(exp.we_wordv[i]);

					for(iter = a; *iter; iter++)
						fprintf(stderr, "new : \"%s\"\n", *iter);

					break;
				}


				case WRDE_BADCHAR:
					/* Illegal occurrence of newline or one of |, &, ;, <, >, (, ), {, } */

				case WRDE_BADVAL:
					/*An undefined shell variable was referenced, and the WRDE_UNDEF flag told us to consider this an error*/

				case WRDE_CMDSUB:
					/*Command substitution occurred, and the WRDE_NOCMD flag  told  us  to  consider  this  an error*/

				case WRDE_NOSPACE:
					/*Out of memory*/

				case WRDE_SYNTAX:
					/*Shell syntax error, such as unbalanced parentheses or unmatched quotes*/
					break;
			}

			wordfree(&exp);
		}
	}
}
