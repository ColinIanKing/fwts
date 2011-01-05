#define _DECLARE_GLOBALS

#include <sys/types.h>
#include <sys/wait.h>

#include "fwts_iasl_interface.h"

#include "aslcompiler.h"
#include "acapps.h"

static void init_asl_core(void)
{
	int i;

	AcpiDbgLevel = 0;

	for (i=0; i<ASL_NUM_FILES; i++) {
		Gbl_Files[i].Handle = NULL;
		Gbl_Files[i].Filename = NULL;
	}
	
	Gbl_Files[ASL_FILE_STDOUT].Handle   = stdout;
	Gbl_Files[ASL_FILE_STDOUT].Filename = "STDOUT";
}

int fwts_iasl_disassemble_aml(const char *aml, const char *outputfile)
{
	ACPI_STATUS	status;
	pid_t	pid;

	pid = fork();
	switch (pid) {
	case -1:
		return -1;
	case 0:
		/* Child */
		init_asl_core();

		Gbl_DisasmFlag = TRUE;
        	Gbl_DoCompile = FALSE;
        	Gbl_OutputFilenamePrefix = (char*)outputfile;
        	Gbl_UseDefaultAmlFilename = FALSE;

		freopen("/dev/null", "w", stderr);

		status = AslDoOnePathname(aml, AslDoOneFile);
		_exit(0);
		break;
	default:
		/* Parent */
		waitpid(pid, &status, WUNTRACED | WCONTINUED);
	}
		
	return 0;
}

int fwts_iasl_assemble_aml(const char *source, char **output)
{
	int pipefds[2];
	pid_t	pid;
	char    *data = NULL;
	char	buffer[8192];
	int	n;
	int 	len = 0;
	int	status;	

	if (pipe(pipefds) < 0)
		return -1;

	pid = fork();
	switch (pid) {
	case -1:
		close(pipefds[0]);
		close(pipefds[1]);
		return -1;
	case 0:
		/* Child */
		if (pipefds[0] != STDOUT_FILENO) {
			dup2(pipefds[1], STDOUT_FILENO);
			close(pipefds[1]);
		}
		close(pipefds[0]);

		init_asl_core();

		Gbl_DisasmFlag = FALSE;
        	Gbl_DoCompile = TRUE;

		status = AslDoOnePathname(source, AslDoOneFile);

		close(pipefds[1]);

		_exit(0);
		break;
	default:
		/* Parent */
		close(pipefds[1]);
		
		while ((n = read(pipefds[0], buffer, sizeof(buffer))) > 0) {
			data = realloc(data, len + n + 1);
			memcpy(data + len, buffer, n);
			len += n;
			data[len] = 0;
		}
		waitpid(pid, &status, WUNTRACED | WCONTINUED);
		close(pipefds[0]);
	}

	*output = data;
	return 0;
}
