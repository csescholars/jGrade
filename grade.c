#include <cgic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mkpath.h"
#include "diff.h"
#include "globals.h"
#include "grade.h"
#include "csv.h"

#define MAX_FILE_SIZE 0x10000 //64K, seems reasonable
#define BUFFER_SIZE MAX_FILE_SIZE

static int compile(char *path, int *result);
static int grade(char *test_path, char *user_path, char *proj_name, int *result);
static void write_feedback_line(FILE *test_res, char *test_name, int pass);

//TODO set up a better error code system
cgiFormResultType beginGrading() {
	char submit_username[81], submit_password[81];
	cgiFilePtr submit_file;
	FILE *drop_file, *feedback_file, *testlist_file, *file;
	char source_filename[80], pathname[80], project_name[80];
	int user_id = 0;
	int submit_id;
	int bytes_read, items_written, file_size;
	char buffer[BUFFER_SIZE];
	char *csv_data[10];
	char *line;
	size_t line_len;
	cgiFormResultType result;
	int run_result;
	int project_id, tmp_id;
	int fail, pass;
	int found;

	if((result = cgiFormStringNoNewlines("username", submit_username, 80)))
		return 1;
	if((result = cgiFormStringNoNewlines("password", submit_password, 80)))
		return 1;

	//lookup the user
	file = fopen(USER_PATH "userlist", "r");
	if(!file) {
		fprintf(cgiOut, "<p class='error'>User file is not set up</p>");
		result = 1; //HACK
		return 1;
	}
	found = 0;
	line_len = 100;
	line = malloc(line_len);
	while(getline(&line, &line_len, file) != -1) {
		parse_CSV(line, csv_data, 3);
		if(!strcmp(csv_data[1], submit_username)) {
			if(!strcmp(csv_data[2], submit_password)) {
				user_id = atoi(csv_data[0]);
				found = 1;
			}
			break;
		}
	}
	free(line);
	fclose(file);
	if(!found) {
		fprintf(cgiOut, "<p class='error'>Invalid username or password</p>");
		return 1;
	}

	//get project id
	if((result = cgiFormInteger("project", &project_id, 0))) {
		fprintf(cgiOut, "<p class='error'>Invalid project</p>");
		return 1;
	}

	//Look up the project information
	file = fopen(PROJECT_PATH "projectlist", "r");
	if(!file) {
		fprintf(cgiOut, "<p class='error'>Project index is not set up</p>");
		return 1;
	}
	found = 0;
	line_len = 100;
	line = malloc(line_len);
	while(getline(&line, &line_len, file) != -1) {
		parse_CSV(line, csv_data, 2);
		tmp_id = atoi(csv_data[0]);
		if(tmp_id == project_id) {
			strcpy(project_name, csv_data[1]);
			found = 1;
		}
	}
	free(line);
	fclose(file);

	if(!found) {
		fprintf(cgiOut, "<p class='error'>Could not find project</p>");
		return 1;
	}

	//set up submission id
	submit_id = rand();

	//check length, make sure it will fit in the buffer
	if((result = cgiFormFileSize(GRADE_FILE_NAME, &file_size))) {
		fprintf(cgiOut, "<p class='error'>Could not determine file size</p>");
		return 1;
	}
	if(file_size > MAX_FILE_SIZE) {
		fprintf(cgiOut, "<p class='error'>Uploaded file is too large at %d, max is %d", file_size, MAX_FILE_SIZE);
		return 1;
	}

	//set up submission directory
	snprintf(pathname, 80, "%s%d/%u/submit_%d", USER_PATH, user_id, project_id, submit_id);
	if(mkpath(pathname, 0700)) {
		fprintf(cgiOut, "<p class='error'>Error setting up submit directory</p>");
		return 1;
	}

	//open file submitted
	if((result = cgiFormFileOpen(GRADE_FILE_NAME, &submit_file))) {
		fprintf(cgiOut, "<p class='error'>File could not be opened</p>");
		return 1;
	}

	//open file to write to
	snprintf(source_filename, 80, "%s/%s.java", pathname, project_name);
	drop_file = fopen(source_filename, "w+");
	if(drop_file == NULL) {
		fprintf(cgiOut, "<p class='error'>Error opening submit file</p>");
		cgiFormFileClose(submit_file);
		return 1;
	}

	//copy data to on-disk file
	while((result = cgiFormFileRead(submit_file, buffer, BUFFER_SIZE, &bytes_read)) == cgiFormSuccess) {
		if(!(items_written = fwrite(buffer, bytes_read, 1, drop_file))) {
			fprintf(cgiOut, "<p class='error'>Error writing file</p>");
			cgiFormFileClose(submit_file);
			fclose(drop_file);
			return 1;
		}
	}

	//file needs to be closed so it can be used later
	fclose(drop_file);
	cgiFormFileClose(submit_file);

	if(result != cgiFormEOF)
	{
		fprintf(cgiOut, "<p class='error'>Error transferring data</p>");
		return 1;
	}

	//open a file to write the output to
	snprintf(buffer, BUFFER_SIZE, "%s/feedback.txt", pathname);
	feedback_file = fopen(buffer, "w+");

	//number of tests failed/passed
	fail = pass = 0;

	fprintf(cgiOut, "<table class='results'>");

	run_result = 0;
	if(compile(source_filename, &run_result)) {
		fclose(feedback_file);
		return 1;
	}
	write_feedback_line(feedback_file, "compile", !run_result);
	if(run_result) {
		fail++;
		fclose(feedback_file);
		fprintf(cgiOut, "</table>");
		return 0;
	} else {
		pass++;
	}

	snprintf(buffer, BUFFER_SIZE, PROJECT_PATH "%u/testlist", project_id);
	testlist_file = fopen(buffer, "r");
	line_len = 100;
	line = malloc(line_len);
	while(getline(&line, &line_len, testlist_file) != -1) {
		parse_CSV(line, csv_data, 1); //really, I am just stripping the \n...

		snprintf(buffer, BUFFER_SIZE, PROJECT_PATH "%u/%s", project_id, csv_data[0]);
		if(grade(buffer, pathname, project_name, &run_result)) {
			fail++;
			continue; //this is an error and should be reported somewhere
		}
		if(run_result)
			fail++;
		else
			pass++;

		write_feedback_line(feedback_file, csv_data[0], !run_result);
	}
	fclose(testlist_file);

	fprintf(cgiOut, "</table>");

	fclose(feedback_file);

	fprintf(cgiOut, "<p class='result'>%d tests passed, %d failed</p>", pass, fail);

	return 0;
}

/*
 * Compiles a .java file
 *
 * @param path   The path to the source file
 * @param result The result of compilation
 * @return       0 if successfully run, 1 otherwise
 */
static int compile(char *path, int *result) {
	pid_t pid;
	int javac_return; //return value of javac

	pid = fork();

	if(pid > 0) {
		//TODO set timeout
		waitpid(pid, &javac_return, 0); //wait for child to terminate
		if(WIFEXITED(javac_return))
			return WEXITSTATUS(javac_return);
		else
			return 1;
	} else if(pid == -1) {
		//failure
		return 1;
	} else {
		//disable stdout and stderr
		dup2(open("/dev/null", 0), fileno(stdout));
		dup2(open("/dev/null", 0), fileno(stderr));

		execlp("javac", "javac", path, (char*)NULL); //run javac

		_exit(EXIT_FAILURE);
	}
	return 1;
}

/*
 * Runs the tests for some project
 *
 * @param test_path The project that is being run
 * @param user_path The path to the user directory for this submission
 * @param proj_name The program that is being run
 * @param result    The result of the test run
 * @return          0 if test ran successfully, 1 otherwise
 */
static int grade(char *test_path, char *user_path, char *proj_name, int *result) {
	//TODO multiple tests
	pid_t pid;
	int java_return; //return value of the program

	char input_path[81], output_path[81], solution_output[81];
	snprintf(input_path, 80, "%s/input.txt", test_path);
	snprintf(solution_output, 80, "%s/output.txt", test_path);
	snprintf(output_path, 80, "%s/output.txt", user_path);

	pid = fork();

	if(pid > 0) {
		//TODO set timeout
		waitpid(pid, &java_return, 0); //wait for child to terminate
		if(!WIFEXITED(java_return))
			return 1;
		if(WEXITSTATUS(java_return))
			return WEXITSTATUS(java_return);

		*result = diff(output_path, solution_output);
		return 0;
	} else if(pid == -1) {
		//failure
		return 1;
	} else {
		dup2(open(input_path, O_RDONLY), fileno(stdin));
		dup2(open(output_path, O_WRONLY | O_CREAT, 0777), fileno(stdout)); //TODO mode
		dup2(open("/dev/null", 0), fileno(stderr)); //yeah, screw stderr

		if(execlp("java", "java", "-cp", user_path, proj_name, (char*)NULL))
			_exit(EXIT_FAILURE);
	}

	return 1; //should never reach
}

/*
 * Write the results of a test to a file
 *
 * @param test_res  File containing the results of the test
 * @param test_name The name of the test being run
 * @param pass      1 if the test passed, 0 otherwise
 */
static void write_feedback_line(FILE *test_res, char *test_name, int pass) {
	fprintf(test_res, "Test \"%s\" run: %s\n", test_name, (pass)?"PASS":"FAIL");
	fprintf(cgiOut, "<tr class='%s'><td>%s</td><td>%s</td></tr>",
			(pass)?"pass":"fail", test_name, (pass)?"PASS":"FAIL");
}
