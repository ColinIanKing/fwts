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

#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "fwts_iasl_interface.h"

#include "aslcompiler.h"
#include "acapps.h"

/*
 *  init_asl_core()
 *	initialize iasl
 */
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

/*
 *  fwts_iasl_disassemble_aml()
 *	invoke iasl to disassemble AML
 */
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
		Gbl_WarningLevel = ASL_WARNING3;
		Gbl_IgnoreErrors = TRUE;
		Gbl_DisasmFlag = TRUE;
		Gbl_DoCompile = FALSE;
		Gbl_OutputFilenamePrefix = (char*)outputfile;
		Gbl_UseDefaultAmlFilename = FALSE;

		/* Throw away noisy errors */
		if (freopen("/dev/null", "w", stderr) != NULL)
			AslDoOneFile((char *)aml);

		_exit(0);
		break;
	default:
		/* Parent */
		(void)waitpid(pid, &status, WUNTRACED | WCONTINUED);
	}

	return 0;
}

/*
 *  fwts_iasl_read_output()
 *	consume output from iasl.
 */
static int fwts_iasl_read_output(const int fd, char **data, size_t *len, bool *eof)
{
	char	buffer[8192];
	ssize_t	n;

	if (*eof)
		return 0;

	while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
		if ((*data = realloc(*data, *len + n + 1)) == NULL)
			return -1;
		memcpy(*data + *len, buffer, n);
		*len += n;
		(*data)[*len] = '\0';
	}
	if (n <= 0)
		*eof = true;
	return 0;
}

/*
 *  fwts_iasl_assemble_aml()
 *	invoke iasl and assemble some source, stdout into
 *	stdout_output, stderr into stderr_output
 */
int fwts_iasl_assemble_aml(const char *source, char **stdout_output, char **stderr_output)
{
	int 	stdout_fds[2], stderr_fds[2];
	int	status, ret = 0;
	size_t	stdout_len = 0, stderr_len = 0;
	pid_t	pid;
	bool	stdout_eof = false, stderr_eof = false;

	if (pipe(stdout_fds) < 0)
		return -1;
	if (pipe(stderr_fds) < 0)
		return -1;

	pid = fork();
	switch (pid) {
	case -1:
		(void)close(stdout_fds[0]);
		(void)close(stdout_fds[1]);
		(void)close(stderr_fds[0]);
		(void)close(stderr_fds[1]);
		return -1;
	case 0:
		/* Child */
		init_asl_core();

		/* stdout to be redirected down the writer end of pipe */
		if (stdout_fds[1] != STDOUT_FILENO)
			if (dup2(stdout_fds[1], STDOUT_FILENO) < 0)
				_exit(EXIT_FAILURE);
		if (stderr_fds[1] != STDERR_FILENO)
			if (dup2(stderr_fds[1], STDERR_FILENO) < 0)
				_exit(EXIT_FAILURE);
		/* Close reader end */
		(void)close(stdout_fds[0]);
		(void)close(stderr_fds[0]);

		/* Setup ACPICA compiler globals */
		Gbl_DisasmFlag = FALSE;
		Gbl_DoCompile = TRUE;
		Gbl_PreprocessFlag = TRUE;
		Gbl_UseDefaultAmlFilename = FALSE;
		Gbl_OutputFilenamePrefix = (char*)source;

		(void)AslDoOneFile((char *)source);

		/*
		 * We need to flush buffered I/O on IASL stdout
		 * before we exit
		 */
		(void)fflush(stdout);
		(void)fflush(stderr);

		_exit(0);
		break;
	default:
		/* Parent */
		/* Close writer end */
		(void)close(stdout_fds[1]);
		(void)close(stderr_fds[1]);

		while (!stdout_eof && !stderr_eof) {
			if (fwts_iasl_read_output(stdout_fds[0], stdout_output, &stdout_len, &stdout_eof) < 0)
				break;
			if (fwts_iasl_read_output(stderr_fds[0], stderr_output, &stderr_len, &stderr_eof) < 0)
				break;
		}
		(void)waitpid(pid, &status, WUNTRACED | WCONTINUED);
		(void)close(stdout_fds[0]);
		(void)close(stderr_fds[0]);
		break;
	}

	return ret;
}
