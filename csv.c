#include <string.h>

/*
 * Parses a colon separated value file (yes, I know what CSV normally is)
 *
 * If nargs is less than the number of items on the line, this will only parse
 * the first nargs items.
 *
 * @param line  The text to parse
 * @param args  An array to put pointers to the found items in
 * @param nargs The number of items on the line
 * @return      0 if parsing successful, 1 otherwise
 */
int parse_CSV(char *line, char *args[], int nargs) {
	char *save = NULL, *parse;
	int i;

	for(i = 0, parse = line; i < nargs; i++) {
		args[i] = strtok_r(parse, ":\n", &save);
		if(!args[i])
			return 1;
		parse = NULL;
	}
	return 0;
}
