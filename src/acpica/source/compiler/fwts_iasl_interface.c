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

	for (i = 0; i < ASL_NUM_FILES; i++) {
		Gbl_Files[i].Handle = NULL;
		Gbl_Files[i].Filename = NULL;
	}

	Gbl_Files[ASL_FILE_STDOUT].Handle   = stdout;
	Gbl_Files[ASL_FILE_STDOUT].Filename = "STDOUT";
	Gbl_Files[ASL_FILE_STDERR].Handle   = stdout;
	Gbl_Files[ASL_FILE_STDERR].Filename = "STDOUT";

	Gbl_LineBufferSize = 16384;
	Gbl_CurrentLineBuffer = NULL;
	Gbl_MainTokenBuffer = NULL;
	UtExpandLineBuffers();
}

int fwts_iasl_disassemble_aml(const char *aml, const char *outputfile)
{
	pid_t	pid;
	int	status;

	pid = fork();
	switch (pid) {
	case -1:
		return -1;
	case 0:
		/* Child */
		init_asl_core();

		/* Setup ACPICA disassembler globals */
		Gbl_DisasmFlag = TRUE;
		Gbl_DoCompile = FALSE;
		Gbl_OutputFilenamePrefix = (char*)outputfile;
		Gbl_UseDefaultAmlFilename = FALSE;

		/* Throw away noisy errors */
		if (freopen("/dev/null", "w", stderr) != NULL)
			(void)AslDoOnePathname((char *)aml, AslDoOneFile);

		_exit(0);
		break;
	default:
		/* Parent */
		(void)waitpid(pid, &status, WUNTRACED | WCONTINUED);
	}

	return 0;
}

int fwts_iasl_assemble_aml(const char *source, char **output)
{
	int 	pipefds[2];
	int	status;
	int 	ret = 0;
	size_t	len = 0;
	ssize_t	n;
	pid_t	pid;
	char    *data = NULL;
	char	buffer[8192];

	if (pipe(pipefds) < 0)
		return -1;

	pid = fork();
	switch (pid) {
	case -1:
		(void)close(pipefds[0]);
		(void)close(pipefds[1]);
		return -1;
	case 0:
		/* Child */
		init_asl_core();

		/* stdout to be redirected down the writer end of pipe */
		if (pipefds[1] != STDOUT_FILENO) {
			if (dup2(pipefds[1], STDOUT_FILENO) < 0) {
				_exit(EXIT_FAILURE);
			}
		}
		/* Close reader end */
		(void)close(pipefds[0]);

		/* Setup ACPICA compiler globals */
		Gbl_DisasmFlag = FALSE;
		Gbl_DoCompile = TRUE;
		Gbl_PreprocessFlag = TRUE;
		Gbl_UseDefaultAmlFilename = FALSE;
		Gbl_OutputFilenamePrefix = (char*)source;

		(void)AslDoOnePathname((char*)source, AslDoOneFile);

		/*
		 * We need to flush buffered I/O on IASL stdout
		 * before we exit
		 */
		fflush(stdout);

		_exit(0);
		break;
	default:
		/* Parent */

		/* Close writer end */
		(void)close(pipefds[1]);

		/* Gather output from IASL */
		while ((n = read(pipefds[0], buffer, sizeof(buffer))) > 0) {
			data = realloc(data, len + n + 1);
			if (data == NULL) {
				ret = -1;
				break;
			}
			memcpy(data + len, buffer, n);
			len += n;
			data[len] = '\0';
		}
		(void)waitpid(pid, &status, WUNTRACED | WCONTINUED);
		(void)close(pipefds[0]);
		break;
	}

	*output = data;
	return ret;
}
