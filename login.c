#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	if(argc > 1)
		fprintf(stderr, "%s: ignoring args\n", *argv);

	execl("ush", "-ush", NULL);

	perror("exec ush");
	return 1;
}
