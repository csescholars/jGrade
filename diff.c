#include "diff.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

/*
 * Returns whether two files are different
 *
 * @param f1  First file to compare
 * @param f2  Second file to compare
 * @return    0 if the files are the same, 1 otherwise
 */
int diff(char *f1, char *f2) {
	//TODO make this WAY less hackish
	char command[161];
	int status;
	snprintf(command, 160, "diff -w %s %s > /dev/null", f1, f2);
	status = system(command);
	if(status == -1 || !WIFEXITED(status))
		return 1;

	return WEXITSTATUS(status);
}
