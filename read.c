#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h> /* pid_t in proc.h */

#include "read.h"
#include "util.h"
#include "proc.h"

struct job *jobs = NULL;

static void parseline(char *);
static void createjob(char **);
static void checkjobs(void);
static void printjob(struct job *);

int readloop()
{
	char line[256];

	setvbuf(stdin, NULL, _IONBF, 0);

	for(;;){
		char *sptr;

		fputs("$ ", stdout);
		if(!fgets(line, 256, stdin)){
			if(feof(stdin)){
				putchar('\n');
				return 0;
			}else{
				perror("fgets()");
				return 1;
			}
		}

		if((sptr = strchr(line, '\n')))
			*sptr = '\0';

		if(!strcmp(line, "jobs")){
			struct job *j;

			for(j = jobs; j; j = j->next){
				char show = 1;

				if(job_isdone(j)){
					if(!j->notified)
						j->notified = 1;
					else
						show = 0;
				}

				if(show)
					printjob(j);
			}

		}else if(!strncmp(line, "fg ", 3)){
			pid_t pid;

			if(sscanf(line + 3, "%d", &pid) == 1){
				struct job *j = job_find(jobs, pid);

				if(j){
					puts("resuming job...");
					job_foreground(j, 1);
				}else
					puts("couldn't find job");

			}else
				puts("invalid job id");
		}else if(*line != '\0')
			parseline(line);

		checkjobs();
	}
}

static void parseline(char *line)
{
	char **argv, *s;
	int argc = 1, i;

	s = line;
	for(;;){
		s = strchr(s, ' ');
		if(s){
			argc++;
			s++;
		}else
			break;
	}

	argv = umalloc((argc+1) * sizeof *argv);

	s = strtok(line, " ");
	i = 0;
	do
		argv[i++] = ustrdup(s);
	while((s = strtok(NULL, " ")));
	argv[i] = NULL;

	createjob(argv);
}

static void createjob(char **argv)
{
	int fds[3];
	struct job *j;

	fds[0] = STDIN_FILENO;
	fds[1] = STDOUT_FILENO;
	fds[2] = STDERR_FILENO;

	j = job_new(proc_new(argv, fds), STDIN_FILENO, STDOUT_FILENO);

	j->next = jobs;
	jobs = j;

	{
		char **s;
		puts("--- createjob():");
		for(s = argv; *s; s++)
			printf(" argv[%ld] = \"%s\"\n", s - argv, *s);
	}

	job_spawn(j, 0);

	if(job_isdone(j)){
		j->notified = 1;
		printjob(j);
	}
}

static void checkjobs()
{
	struct job *j;

	for(j = jobs; j; j = j->next){
		if(!job_isdone(j)){
			job_waitfor(j, 0 /* don't block */);

			if(job_isdone(j) && !j->notified){
				printjob(j);
				j->notified = 1;
			}
		}
	}
}

static void printjob(struct job *j)
{
	struct process *p;

	puts("job:");

	for(p = j->proc; p; p = p->next){
		printf("  %d: %s: ", p->pid, p->argv[0]);

		switch(p->state){
			case DONE:
				printf("done");
				break;
			case INIT:
				puts("init");
				break;
			case FG:
				puts("fg");
				break;
			case BG:
				puts("bg");
				break;
		}
	}

	if(job_isdone(j)){
		printf(", returned %d", j->exitcode);
		if(j->exitsig)
			printf(" signal %d", j->exitsig);
		putchar('\n');
	}
}
