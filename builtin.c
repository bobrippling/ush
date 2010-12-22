#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <signal.h>

#include <unistd.h>

#include "proc.h"
#include "job.h"
#include "builtin.h"

#define LEN(a) (sizeof(a) / sizeof(a[0]))

#define BUILTIN(n) static int builtin_##n(int argc, char **argv)

BUILTIN(echo);
BUILTIN(ps);
BUILTIN(kill);
BUILTIN(bg);
BUILTIN(jobs);

static struct builtin
{
	const char *argv0;
	int (*const f)(int, char **);
} builtins[] = {
#define STRUCT_BUILTIN(n) { #n, builtin_##n }
	STRUCT_BUILTIN(echo),
	STRUCT_BUILTIN(ps),
	STRUCT_BUILTIN(kill),
	STRUCT_BUILTIN(bg),
	STRUCT_BUILTIN(jobs)
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

	while((ent = readdir(d))){
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
				sprintf(cmdln, "/proc/%d/cmdline", pid);

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
		SIG(SIGHUP),  SIG(SIGINT),  SIG(SIGQUIT), SIG(SIGTRAP),
		SIG(SIGABRT), SIG(SIGKILL), SIG(SIGUSR1), SIG(SIGSEGV),
		SIG(SIGUSR2), SIG(SIGPIPE), SIG(SIGALRM), SIG(SIGTERM),
		SIG(SIGCHLD), SIG(SIGCONT), SIG(SIGSTOP), SIG(SIGTSTP),
		SIG(SIGTTIN), SIG(SIGTTOU), SIG(SIGWINCH)
#undef SIG
	};
	int sig = SIGTERM, i = 1, ret = 0;

	if(argc == 1){
		fprintf(stderr, "%s: need args\n", *argv);
		return 1;
	}

	if(argv[i][0] == '-'){
		char *ssig = argv[i++] + 1;

		if(argc == 2){
			fprintf(stderr, "%s: need pids\n", *argv);
			return 1;
		}

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
		int pid;
		if(sscanf(argv[i], "%d", &pid) == 1){
			if(kill(pid, sig)){
				fprintf(stderr, "%s: kill(%d): %s\n", *argv, pid, strerror(errno));
				ret = 1;
			}
		}else{
			fprintf(stderr, "%s: %s not a pid (ignored)\n", *argv, argv[i]);
			ret = 1;
		}
	}

	return ret;
}

BUILTIN(bg)
{
	extern struct job *jobs;
	struct job *j;
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

	for(j = jobs; j; j = j->next)
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

	if(j){
		job_bg(j);
		return 0;
	}else{
		fprintf(stderr, "no such job %s%d (use %% for a job id)\n", is_jid ? "%" : "", id);
		return 1;
	}
}

BUILTIN(jobs)
{
	extern struct job *jobs;
	struct job *j, *j2;

	if(argc > 1)
		fprintf(stderr, "%s: ignoring arguments\n", *argv);

	for(j = jobs; j; j = j->next){
		printf("job %d\n", j->job_id);

		for(j2 = j; j2; j2 = j2->jobnext){
			struct proc *p;

			printf("\t\"%s\": %s\n", j2->cmd, job_state_name(j2));

			for(p = j2->proc; p; p = p->next){
				printf("\t\tproc %d ", p->pid);
				switch(p->state){
					case PROC_SPAWN: /* probably us */
					case PROC_RUN:   printf("RS (running or sleeping)"); break;
					case PROC_STOP:  printf("T (stopped)");              break;
					case PROC_FIN:   printf("- (finished)");             break;
				}
				printf(" (\"%s\")\n", *p->argv);
			}
		}
	}

	return 0;
}
