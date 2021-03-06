#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <signal.h>

#include <unistd.h>

#include "proc.h"
#include "job.h"
#include "parse.h"
#include "task.h"
#include "builtin.h"
#include "term.h"
#include "esc.h"
#include "path.h"
#include "limit.h"

#define LEN(a) (sizeof(a) / sizeof(a[0]))

#define BUILTIN(n) static int builtin_##n(int argc, char **argv)

BUILTIN(echo);
BUILTIN(ps);
BUILTIN(kill);
BUILTIN(bg);
BUILTIN(jobs);
BUILTIN(reset);
BUILTIN(colon);
BUILTIN(which);
BUILTIN(ulimit);

static struct builtin
{
	const char *argv0;
	int (*const f)(int, char **);
} builtins[] = {
#define STRUCT_BUILTIN(n) { #n, builtin_##n }
	STRUCT_BUILTIN(reset),
	STRUCT_BUILTIN(echo),
	STRUCT_BUILTIN(ps),
	STRUCT_BUILTIN(kill),
	STRUCT_BUILTIN(bg),
	STRUCT_BUILTIN(jobs),
	STRUCT_BUILTIN(which),
	STRUCT_BUILTIN(ulimit),
	{ ":", builtin_colon }
#undef STRUCT_BUILTIN
};


int proc_argc(struct proc *p)
{
	char **iter;
	int i = 0;
	for(iter = p->argv; *iter; i++, iter++);
	return i;
}

/* returns if not a builtin function */
void builtin_exec(struct proc *p)
{
	unsigned int i;

	for(i = 0; i < LEN(builtins); i++)
		if(!strcmp(p->argv[0], builtins[i].argv0))
			exit(builtins[i].f(proc_argc(p), p->argv));
}

BUILTIN(reset)
{
	if(argc)
		fprintf(stderr, "%s: ignoring args\n", *argv);
	esc_reset();
	term_attr_ush();
	return 0;
}

BUILTIN(echo)
{
	int i = 1, eol = 1;

	if(argc == 1)
		return 0;

	if(!strcmp(argv[i], "-n")){
		eol = 0;
		i++;
	}

	for(; i < argc; i++){
		fputs(argv[i], stdout);
		if(i < argc-1)
			putchar(' ');
	}

	if(eol)
		putchar('\n');

	return 0;
}

/*
BUILTIN(ls)
{
	int i, ret = ;

	if(argc > 1)
		for(i = 1; i < argv; i++)
			ret |= ls(argv[i]);
	else
		ret = ls(".");

	return ret;
}

static int ls(char *path)
{
	DIR *d;
	struct dirent *ent;
	struct stat st;

}
*/

BUILTIN(ps)
{
	/* TODO: kernel threads? */
	DIR *d = opendir("/proc");
	struct dirent *ent;

	if(argc > 1)
		fprintf(stderr, "%s: ignoring args\n", *argv);

	if(!d){
		perror("opendir()");
		return 1;
	}

	while((errno = 0, ent = readdir(d))){
		struct stat st;
		char *path = malloc(strlen(ent->d_name) + 7);

		if(!path){
			perror("malloc()");
			closedir(d);
			return 1;
		}

		sprintf(path, "/proc/%s", ent->d_name);

		if(stat(path, &st)){
			fprintf(stderr, "%s: stat: %s: %s\n", *argv, path, strerror(errno));
		}else if(S_ISDIR(st.st_mode)){
			int pid;

			if(sscanf(ent->d_name, "%d", &pid)){
				FILE *f;
				char *cmdln = malloc(strlen(ent->d_name) + 15);

				/* TODO: show user */
				sprintf(cmdln, "/proc/%d/cmdline", pid); /* TODO: move this up to before the stat() */

				f = fopen(cmdln, "r");
				if(f){
					char buffer[256];

					if(fgets(buffer, sizeof buffer, f)){
						char *sp = strchr(buffer, ' ');
						if(sp)
							*sp = '\0';

						printf("%d %s\n", pid, buffer);
					}else if(ferror(f))
						fprintf(stderr, "%s: read: %s: %s\n", *argv, cmdln, strerror(errno));
					/* else eof - linux won't let us see the command line */

					fclose(f);
				}else{
					fprintf(stderr, "%s: open: %s: %s\n", *argv, cmdln, strerror(errno));
				}
				free(cmdln);
			}
		}

		free(path);
	}

	if(errno)
		perror("readdir()");

	closedir(d);

	return 0;
}

BUILTIN(kill)
{
	struct
	{
		const char *name;
		int sig;
	} sigs[] = {
#define SIG(n) { #n, n }
		SIG(SIGHUP),  SIG(SIGINT),  SIG(SIGQUIT),
		SIG(SIGABRT), SIG(SIGKILL), SIG(SIGUSR1), SIG(SIGSEGV),
		SIG(SIGUSR2), SIG(SIGPIPE), SIG(SIGALRM), SIG(SIGTERM),
		SIG(SIGCHLD), SIG(SIGCONT), SIG(SIGSTOP), SIG(SIGTSTP),
		SIG(SIGTTIN), SIG(SIGTTOU),
#ifdef SIGTRAP
		SIG(SIGTRAP),
#endif
#ifdef SIGWINCH
		SIG(SIGWINCH)
#endif
#undef SIG
	};
	int sig = SIGTERM, i = 1, ret = 0;

	if(argc == 1){
usage:
		fprintf(stderr, "Usage: %s [-SIG] [pid...] [%%jid...] \n", *argv);
		return 1;
	}

	if(argv[i][0] == '-'){
		char *ssig = argv[i++] + 1;

		if(argc == 2)
			goto usage;

		if(sscanf(ssig, "%d", &sig) != 1){
			unsigned int nsig;

			if(strncmp(ssig, "SIG", 3))
				for(nsig = 0; nsig < LEN(sigs); nsig++)
					sigs[nsig].name += 3;

			sig = -1;
			for(nsig = 0; nsig < LEN(sigs); nsig++)
				if(!strcmp(ssig, sigs[nsig].name)){
					sig = sigs[nsig].sig;
					break;
				}

			if(sig == -1){
				fprintf(stderr, "%s: %s isn't a signal name nor number\n", *argv, ssig);
				return 1;
			}
		}
	}

	for(; i < argc; i++){
		int pid, is_jid = 0;

		if(*argv[i] == '%'){
			argv[i]++;
			is_jid = 1;
		}


		if(sscanf(argv[i], "%d", &pid) != 1){
			fprintf(stderr, "%s: \"%s\" not a pid/jid (ignored)\n", *argv, argv[i]);
			ret = 1;
			continue;
		}

		if(is_jid){
			extern struct task *tasks;
			struct task *t;

			for(t = tasks; t; t = t->next){
				struct job *j;
				for(j = t->jobs; j; j = j->next)
					if(j->job_id == pid){
						pid = -j->gid;
						goto kill;
					}
			}

			fprintf(stderr, "%s: job %%%d not found\n", *argv, pid);
		}else{
kill:
			if(kill(pid, sig)){
				fprintf(stderr, "%s: kill(%d): %s\n", *argv, pid, strerror(errno));
				ret = 1;
			}
		}
	}

	return ret;
}

BUILTIN(bg)
{
	extern struct task *tasks;
	struct task *t;
	int is_jid;
	int id;

	if(argc != 2){
		fprintf(stderr, "Usage: %s %%job_id | process_id\n", *argv);
		return 1;
	}

	if(*argv[1] == '%'){
		argv[1]++;
		is_jid = 1;
	}else
		is_jid = 0;

	if(sscanf(argv[1], "%d", &id) != 1){
		fprintf(stderr, "%s isn't a valid job id/pid\n", argv[1]);
		return 1;
	}

	for(t = tasks; t; t = t->next){
		struct job *j;
		for(j = t->jobs; j; j = j->next)
			if(is_jid){
				if(j->job_id == id)
					break;
			}else{
				struct proc *p;
				int found = 0;
				for(p = j->proc; p; p = p->next)
					if(p->state != PROC_FIN && p->pid == id){
						found = 1;
						break;
					}
				if(found)
					break;
			}
	}

	if(t){
		task_sig(t, SIGCONT);
		return 0;
	}else{
		if(is_jid)
			fprintf(stderr, "no such job %%%d\n", id);
		else
			fprintf(stderr, "no such job with pid %d (try %%jid)\n", id);
		return 1;
	}
}

BUILTIN(jobs)
{
	extern struct task *tasks;
	struct task *t;
	const pid_t mypid = getpid();

	if(argc > 1)
		fprintf(stderr, "%s: ignoring arguments\n", *argv);

	for(t = tasks; t; t = t->next){
		struct job *j;
		printf("task %d (\"%s\") %s\n", t->jobs->job_id, t->cmd, task_state_name(t));

		for(j = t->jobs; j; j = j->next){
			struct proc *p;

			printf("\tjob \"%s\": %s\n", j->cmd, job_state_name(j));

			for(p = j->proc; p; p = p->next)
				printf("\t\t%d \"%s\" %s%s\n", p->pid, *p->argv,
						proc_state_name(p), p->pid == mypid ? " (ME)":"");
		}
	}

	return 0;
}

BUILTIN(colon)
{
	(void)argc;
	(void)argv;
	return 0;
}

BUILTIN(which)
{
	extern struct exe *exes;
	struct exe *e;
	int i, ret = 0;
	int needspace = 0;

	if(argc == 1){
		fprintf(stderr, "Usage: %s command(s)...\n", *argv);
		return 1;
	}

	for(i = 1; i < argc; i++){
		int found = 0;
		printf("%s: ", argv[i]);
		for(e = exes; e; e = e->next)
			if(!strcmp(argv[i], e->basename)){
				found = 1;
				printf("%s%s", needspace ? " " : "", e->path);
				needspace = 1;
			}
		if(!found){
			printf("not found");
			ret = 1;
		}
		putchar('\n');
	}
	return ret;
}

BUILTIN(ulimit)
{
	enum limit_type t;

	if(argc >= 2 && limit_parse(argv[1], &t))
		goto usage;

	if(argc == 2){
		/* get */
		long val;
		if(limit_get(t, &val)){
			perror("getrlimit()");
			return 1;
		}else{
			if(val == -1)
				printf("%s: unlimited\n", argv[1]);
			else
				printf("%s: %ld\n", argv[1], val);
			return 0;
		}

	}else if(argc == 3){
		/* set */
		long val;
		char *end = NULL;

		if(!strcmp(argv[2], "unlimited")){
			val = -1;
		}else{
			val = strtol(argv[2], &end, 10);
			if(*end)
				goto usage;
		}

		if(limit_set(t, val)){
			perror("setrlimit()");
			return 1;
		}
		return 0;

	}else if(argc <= 1){
		int i;

		for(i = 0; i < LIMIT_LAST - 1; i++){
			long v = -1;
			limit_get(i, &v);
			printf("%s: ", limit_str(i));
			if(v == -1)
				puts("unlimited");
			else
				printf("%ld\n", v);
		}

		return 0;

	}else{
		int i;

usage:
		fprintf(stderr,
				"Usage: %s [lim] [value]\n"
				"limits:\n", *argv);

		for(i = 0; i < LIMIT_LAST - 1; i++)
			fprintf(stderr, "  %s\n", limit_str(i));

		return 1;
	}
}
