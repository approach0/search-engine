#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

int latexml_gen_mathml_file(const char *fname, const char *tex)
{
	pid_t pid;
	int status, timeout = 3;
	const char *argv[] = {
		"/usr/bin/latexmlmath", tex,
		"--presentationmathml", fname,
		NULL
	};

	if (0 == (pid = fork())) {
		if (-1 == execv(argv[0], (char**)argv)) {
			perror("child process execve failed.");
			return -1;
		}
	}

	while (0 == waitpid(pid, &status, WNOHANG)) {
		if (timeout <= 0) {
			perror("child process execve timeout.");
			return -1;
		}
		timeout--;
		sleep(1);
	}

	if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
		perror("child process execve return non-zero.");
		return -1;
	}

	return 0;
}
