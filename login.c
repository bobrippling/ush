#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	(void)argc;

	argv[0] = "-ush";
	execv("ush", argv);

	perror("exec ush");
	return 1;
}
