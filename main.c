#include <cgic.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "grade.h"
#include "globals.h"
#include "csv.h"

enum ACTION_T {
	VIEWFORM,
	GRADE,
	ADMIN,
	VIEW,
	ERROR
};

int viewForm();

int cgiMain() {
	enum ACTION_T action;

	//initialize randon number generation
	srand(time(NULL));

	cgiHeaderContentType("text/html");

	if(cgiFormSubmitClicked("grade") == cgiFormSuccess) {
		action = GRADE;
	} else {
		action = VIEWFORM;
	}
	fprintf(cgiOut, "<html>"
			"<head><title>APCS Autograder</title></head>"
			"<body>");

	switch(action) {
		case VIEWFORM:
			viewForm();
			break;
		case GRADE:
			beginGrading();
			break;
		case VIEW:
			//TODO view your results
			break;
		case ADMIN:
			//TODO setup new tests, modify existing
			break;
		case ERROR:
		default:
			fprintf(cgiOut, "ERROR");
			exit(EXIT_FAILURE);
			break;
	}

	fprintf(cgiOut, "</body></html>");


	return 0;
}

int viewForm() {
	FILE *list_file;
	char *line;
	char *projectdata[2];
	size_t line_len;

	fprintf(cgiOut, "<form method='post' enctype='multipart/form-data' action='");
	cgiValueEscape(cgiScriptName);
	fprintf(cgiOut, "'>");
	fprintf(cgiOut, "<div>Please upload a .java file containing your project: "
			"<input type='file' name='%s' value=''></div>", GRADE_FILE_NAME);
	fprintf(cgiOut, "<div>Please enter your username and password: "
			"<input type='text' name='username' value=''>"
			"<input type='password' name='password' value=''>"
			"</div>");

	fprintf(cgiOut, "<div>Please select the project to grade: <select name='project'>");

	//Print options
	line_len = 100;
	line = malloc(line_len);
	list_file = fopen(PROJECT_PATH "projectlist", "r");
	if(!list_file)
		return 1;
	while(getline(&line, &line_len, list_file) != -1) {
		parse_CSV(line, projectdata, 2);
		fprintf(cgiOut, "<option value='%s'>%s</option>", projectdata[0], projectdata[1]);
	}
	free(line);
	fclose(list_file);
	fprintf(cgiOut, "</select></div>");

	fprintf(cgiOut, "<input type='submit' name='grade' value='Grade Project'>");
  fprintf(cgiOut, "</form>");
	return 0;
}
