#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

void latexml_gen_mathml_file(const char *fname, const char *tex)
{
// 	pid_t pid;
// 	int status, timeout = 3000;
// 	const char argv[] = {"usr/bin/latexml", "--presentationmathml"
// 
// 	if (0 == (pid = fork())) {
// 		if (-1 == execve(arg)
// 	}

	char cmdbuf[4096];
	snprintf(cmdbuf, 4096, "echo '%s' | "
	"latexmlmath --presentationmathml=%s -", tex, fname);
	printf("%s\n", cmdbuf);
	system(cmdbuf);
}
