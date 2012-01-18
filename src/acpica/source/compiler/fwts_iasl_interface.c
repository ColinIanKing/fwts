/*
 * Copyright (C) 2011-2012 Canonical
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

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

		/* Throw away noisy errors */
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
	FILE 	*fp;

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
		init_asl_core();

		if (pipefds[0] != STDOUT_FILENO) {
			dup2(pipefds[1], STDOUT_FILENO);
			close(pipefds[1]);
		}
		close(pipefds[0]);

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
