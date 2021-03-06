#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "util.h"
#include "proc.h"
#include "job.h"
#include "term.h"
#include "parse.h"
#include "config.h"

#ifdef USH_DEBUG
# define JOB_DEBUG
#else
# undef JOB_DEBUG
#endif

extern int interactive;

/** create a struct proc for each argv */
struct job *job_new(char ***argvp, int jid, struct redir *redir)
{
	struct job *j;
	struct proc **tail;

	j = umalloc(sizeof *j);
	memset(j, 0, sizeof *j);

	j->cmd    = ustrdup_argvp(argvp);
	j->argvp  = argvp;
	j->redir  = redir;
	j->job_id = jid;
	j->state  = JOB_BEGIN;

	tail = &j->proc;
	while(*argvp){
		*tail = proc_new(*argvp++);
		tail = &(*tail)->next;
	}
	*tail = NULL;
	return j;
}

void job_close_fds(struct job *j, struct proc *pbreak)
{
	struct proc *p;
	for(p = j->proc; p && p != pbreak; p = p->next){
#define CLOSE(a, b) \
		if(a != b) \
			close(a)

		CLOSE(p->in , STDIN_FILENO);
		CLOSE(p->out, STDOUT_FILENO);
		CLOSE(p->err, STDERR_FILENO);
#undef CLOSE
	}
}

int job_start(struct job *j)
{
	struct proc *p;
	struct redir *redir_iter;
	int pipey[2] = { -1, -1 };

#define REDIR(a, b) \
					do{ \
						if(a != b){ \
							/* close b and copy a into b */ \
							if(dup2(a, b) == -1) \
								perror("dup2()"); \
							if(close(a) == -1) \
								perror("close()"); \
						} \
					}while(0)

	for(redir_iter = j->redir; redir_iter; redir_iter = redir_iter->next){
		int fd;

		if(redir_iter->fname){
			fd = open(redir_iter->fname, O_WRONLY | O_CREAT | O_TRUNC); /* FIXME: only out for now - need "<" and ">>" */
			if(fd == -1){
				fprintf(stderr, "ush: open %s: %s\n", redir_iter->fname, strerror(errno));
				/* FIXME: close all other fds */
				return 1;
			}
		}else{
			fd = redir_iter->fd_out;
		}

		fprintf(stderr, "job_start(): REDIR(%d [%s], %d)\n", fd, redir_iter->fname, redir_iter->fd_in);
		REDIR(fd, redir_iter->fd_in);
	}

	j->proc->in = STDIN_FILENO;
	for(p = j->proc; p; p = p->next){
		p->err = STDERR_FILENO;
		if(p->next){
			if(pipe(pipey) < 0){
				perror("pipe()");
				goto bail;
			}
			p->out = pipey[1];
			p->next->in = pipey[0];
		}else{
			p->out = STDOUT_FILENO;
		}

		/* TODO: cd, fg, rehash */

		switch(p->pid = fork()){
			case 0:
				p->pid = getpid();
				REDIR(p->in,   STDIN_FILENO);
				REDIR(p->out, STDOUT_FILENO);
				REDIR(p->err, STDERR_FILENO);
#undef REDIR
				job_close_fds(j, p->next);
				proc_exec(p, j->gid);
				break; /* unreachable */

			case -1:
				perror("fork()");
				goto bail;

			default:
				if(interactive){
					if(!j->gid)
						j->gid = p->pid;
					setpgid(p->pid, j->gid);
				}
				p->state = PROC_RUN;
		}
	}

	/* close our access to all these pipes */
	job_close_fds(j, NULL);

	j->state = JOB_RUNNING;

	return 0;
bail:
	fprintf(stderr, "warning: error starting job: %s\n", strerror(errno));
	for(; p; p = p->next)
		p->state = PROC_FIN;
	job_close_fds(j, NULL);
	job_sig(j, SIGCONT);
	return 1;
}

int job_sig(struct job *j, int sig)
{
	struct proc *p;
	int ret = 0;

	for(p = j->proc; p; p = p->next)
		if(kill(p->pid, sig))
			ret = 1;

	return ret;
}

int job_wait(struct job *j, int async)
{
	int wait_status = 0, proc_still_running = 0;
	struct proc *p;
	pid_t pid;
	pid_t wait_on_me;

#define JOB_WAIT_CHANGE_GROUP
#ifdef JOB_WAIT_CHANGE_GROUP
	pid_t orig_pgid;
	int save;

	if(interactive)
		orig_pgid = getpgid(STDIN_FILENO);
#endif

rewait:
	if(interactive){
		if(j->tconf_got)
			term_attr_set(&j->tconf);
		else
			term_attr_orig();

#ifdef JOB_WAIT_CHANGE_GROUP
		term_give_to(j->gid);
		/*setpgid(getpid(), j->gid);*/
#endif
	}

	if(j->gid){
		wait_on_me = -j->gid;
	}else{
		struct proc *p = job_first_proc(j);

		if(p)
			wait_on_me = p->pid;
		else
			wait_on_me = j->proc->pid; /* guess */
	}

	pid = waitpid(wait_on_me, &wait_status,
			WUNTRACED | WCONTINUED | (async ? WNOHANG : 0));

	if(interactive){
#ifdef JOB_WAIT_CHANGE_GROUP
		save = errno;
		term_give_to(orig_pgid);
		/*setpgid(getpid(), orig_pgid);*/
		errno = save;
#endif

		term_attr_get(&j->tconf);
		j->tconf_got = 1;
		term_attr_ush();
	}


	if(pid == -1){
		if(errno == EINTR)
			goto rewait;
		else
			fprintf(stderr, "waitpid(%d [job %d: \"%s\"], async = %s): %s\n",
					-j->gid, j->job_id, j->cmd, async ? "true" : "false",
					strerror(errno));

		return 1;
	}else if(pid == 0 && async)
		return 0;


	for(p = j->proc; p; p = p->next)
		if(p->pid == pid){
			if(WIFSIGNALED(wait_status)){
				p->state = PROC_FIN;

				p->exit_sig = WTERMSIG(wait_status);
				p->exit_code = WEXITSTATUS(wait_status); /* 0 */
			}else if(WIFEXITED(wait_status)){
				p->state = PROC_FIN;

				p->exit_sig = 0;
				p->exit_code = WEXITSTATUS(wait_status);
			}else if(WIFSTOPPED(wait_status)){
				p->state = PROC_STOP;

				p->exit_sig = WSTOPSIG(wait_status); /* could be TTIN or pals */
				proc_still_running = 1;
			}else if(WIFCONTINUED(wait_status)){
				p->state = PROC_RUN;

				p->exit_sig = SIGCONT;
				proc_still_running = 1;
			}else
				fprintf(stderr, "job_wait(): wait_status unknown for pid %d: 0x%x\n", pid, wait_status);
		}else if(p->state != PROC_FIN)
			proc_still_running = 1;


	if(!proc_still_running)
		j->state = JOB_COMPLETE;
	return 0;
}

struct proc *job_first_proc(struct job *j)
{
	struct proc *p;
	for(p = j->proc; p; p = p->next)
		if(p->state == PROC_RUN || p->state == PROC_STOP)
			return p;
	return NULL;
}

void job_free(struct job *j)
{
	struct proc *p, *next;

	for(p = j->proc; p; p = next){
		next = p->next;
		proc_free(p);
	}
	ufree_argvp(j->argvp);
	free(j->cmd);
	free(j);
}

const char *job_state_name(struct job *j)
{
	switch(j->state){
		case JOB_BEGIN:    return "initialising";
		case JOB_RUNNING:  return "running";
		case JOB_COMPLETE: return "complete";
		case JOB_MOVED_ON: return "complete (moved on)";
	}
	return NULL;
}

int job_complete(struct job *j)
{
	struct proc *p;
	for(p = j->proc; p; p = p->next)
		if(p->state != PROC_FIN)
			return 0;
	return 1;
}

char *job_desc(struct job *j)
{
	char *ret;
	int len = 1;
	struct proc *p;

	for(p = j->proc; p; p = p->next)
		len += strlen(*p->argv) + 1; /* "|" */

	ret = umalloc(len + 1);
	*ret = '\0';

	for(p = j->proc; p; p = p->next){
		strcat(ret, *p->argv);
		if(p->next)
			strcat(ret, "|");
	}

	return ret;
}
