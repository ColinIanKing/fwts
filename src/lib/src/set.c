#include <stdlib.h>
#include <stdio.h>

int set(const char *text, const char *file)
{	
	FILE *fp;

	fp = fopen(file, "w");
	if (fp != NULL) {
		fprintf(fp, "%s\n", text);
		fclose(fp);	
		return 0;
	}	
	else
		return 1;
}
