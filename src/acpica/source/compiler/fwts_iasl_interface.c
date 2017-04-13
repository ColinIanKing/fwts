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
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "fwts_iasl_interface.h"

#include "aslcompiler.h"
#include "acdisasm.h"
#include "acapps.h"

/*
 *  init_asl_core()
 *	initialize iasl
 */
static void init_asl_core(void)
{
	int i;

	AcpiOsInitialize();
	ACPI_DEBUG_INITIALIZE();
	AcpiGbl_ExternalFileList = NULL;
	AcpiDbgLevel = 0;
	PrInitializePreprocessor();
	AcpiGbl_DmOpt_Verbose = FALSE;
	AcpiGbl_IntegerBitWidth = 64;
	AcpiGbl_IntegerNybbleWidth = 16;
	AcpiGbl_IntegerByteWidth = 8;

	for (i = 0; i < ASL_NUM_FILES; i++) {
		Gbl_Files[i].Handle = NULL;
		Gbl_Files[i].Filename = NULL;
	}

	Gbl_Files[ASL_FILE_STDOUT].Handle   = stdout;
	Gbl_Files[ASL_FILE_STDOUT].Filename = "STDOUT";
	Gbl_Files[ASL_FILE_STDERR].Handle   = stdout;
	Gbl_Files[ASL_FILE_STDERR].Filename = "STDOUT";

	Gbl_LineBufferSize = 1024;
	Gbl_CurrentLineBuffer = NULL;
	Gbl_MainTokenBuffer = NULL;
	UtExpandLineBuffers();
}

/*
 *  fwts_iasl_disassemble_aml()
 *	invoke iasl to disassemble AML
 */
int fwts_iasl_disassemble_aml(
	char *tables[],
	char *names[],
	const int table_entries,
	const int which,
	const bool use_externals,
	const char *outputfile)
{
	pid_t	pid;
	int	status;
	FILE	*fpout, *fperr;

	fflush(stdout);
	fflush(stderr);

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
		AcpiGbl_CstyleDisassembly = FALSE;
		AcpiGbl_DmOpt_Verbose = FALSE;
		UtConvertBackslashes (Gbl_OutputFilenamePrefix);

		/* Do we need to include external tables in? */
		if (use_externals) {
			int i;
			/*
			 * Add in external SSDT files and NOT the one we want
			 * disassemble
			 */
			for (i = 0; i < table_entries; i++) {
				if ((i != which) &&
				    (names[i] != NULL) &&
				    (tables[i] != NULL) &&
				    (!strcmp(names[i], "SSDT") ||
				     !strcmp(names[i], "DSDT"))) {
					ACPI_STATUS	acpi_status;
					/*
					 *  Add in external tables that are NOT the table
					 *  we intent to disassemble
					 */
					acpi_status = AcpiDmAddToExternalFileList(tables[i]);
					if (ACPI_FAILURE(acpi_status)) {
						(void)unlink(outputfile);
						_exit(1);
					}
				}
			}
		}
		/* Throw away noisy errors */
		if ((fpout = freopen("/dev/null", "w", stdout)) == NULL) {
			_exit(1);
		}
		if ((fperr = freopen("/dev/null", "w", stderr)) == NULL) {
			(void)fclose(fpout);
			_exit(1);
		}

		/* ...and do the ACPICA disassambly... */
		AslDoOneFile((char *)tables[which]);
		UtFreeLineBuffers();
		AslParserCleanup();
		if (AcpiGbl_ExternalFileList)
			AcpiDmClearExternalFileList();

		(void)fclose(fperr);
		(void)fclose(fpout);
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
		char *tmp;

		tmp = realloc(*data, *len + n + 1);
		if (!tmp) {
			free(*data);
			*data = NULL;
			return -1;
		}
		*data = tmp;
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

	fflush(stdout);
	fflush(stderr);

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
		UtConvertBackslashes (Gbl_OutputFilenamePrefix);

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

/*
 *  fwts_iasl_exception_level__()
 *	shim wrapper for AeDecodeExceptionLevel level decoding
 */
const char *fwts_iasl_exception_level__(uint8_t level)
{
	const char *str;

	bool tmp = Gbl_VerboseErrors;

	Gbl_VerboseErrors = true;
	str = AeDecodeExceptionLevel((UINT8)level);
	Gbl_VerboseErrors = tmp;

	return str;
}
