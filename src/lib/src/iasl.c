/*
 * Copyright (C) 2010-2011 Canonical
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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>

#include "fwts.h"
#include "fwts_iasl_interface.h"

int fwts_iasl_disassemble(fwts_framework *fw,
	const char *tablename,
	const int which,
	fwts_list **iasl_output)
{
	fwts_acpi_table_info *table;
	char tmpfile[PATH_MAX];
	char amlfile[PATH_MAX];
	int fd;
	int pid = getpid();

	if (iasl_output == NULL)
		return FWTS_ERROR;

	*iasl_output = NULL;

	if (fwts_acpi_find_table(fw, tablename, which, &table) != FWTS_OK)
		return FWTS_ERROR;

	if (table == NULL)
		return FWTS_OK;

	snprintf(tmpfile, sizeof(tmpfile), "/tmp/fwts_iasl_%d_%s.dsl", pid, tablename);
	snprintf(amlfile, sizeof(amlfile), "/tmp/fwts_iasl_%d_%s.dat", pid, tablename);

	/* Dump the AML bytecode into a tempoary file so we can disassemble it */
	if ((fd = open(amlfile, O_WRONLY | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR)) < 0) {
		fwts_log_error(fw, "Cannot create temporary file %s", amlfile);
		return FWTS_ERROR;
	}

	if (write(fd, table->data, table->length) != table->length) {
		fwts_log_error(fw, "Cannot write all data to temporary file");
		close(fd);
		(void)unlink(amlfile);
		return FWTS_ERROR;
	}	
	close(fd);

	if (fwts_iasl_disassemble_aml(amlfile, tmpfile) < 0) {
		(void)unlink(tmpfile);
		(void)unlink(amlfile);
		return FWTS_ERROR;
	}

	(void)unlink(amlfile);
	*iasl_output = fwts_file_open_and_read(tmpfile);
	(void)unlink(tmpfile);

	return FWTS_OK;
}

int fwts_iasl_reassemble(fwts_framework *fw,
	const uint8_t *data, 
	const int len,
	fwts_list **iasl_disassembly,
	fwts_list **iasl_errors)
{
	char tmpfile[PATH_MAX];
	char amlfile[PATH_MAX];
	char *output_text = NULL;
	int fd;
	int pid = getpid();

	if ((iasl_disassembly  == NULL) || (iasl_errors == NULL))
		return FWTS_ERROR;

	*iasl_disassembly = NULL;

	snprintf(tmpfile, sizeof(tmpfile), "/tmp/fwts_iasl_%d.dsl", pid);
	snprintf(amlfile, sizeof(amlfile), "/tmp/fwts_iasl_%d.dat", pid);

	/* Dump the AML bytecode into a tempoary file so we can disassemble it */
	if ((fd = open(amlfile, O_WRONLY | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR)) < 0) {
		fwts_log_error(fw, "Cannot create temporary file %s", amlfile);
		return FWTS_ERROR;
	}

	if (write(fd, data, len) != len) {
		fwts_log_error(fw, "Cannot write all data to temporary file");
		close(fd);
		(void)unlink(amlfile);
		return FWTS_ERROR;
	}	
	close(fd);

	if (fwts_iasl_disassemble_aml(amlfile, tmpfile) < 0) {
		(void)unlink(tmpfile);
		(void)unlink(amlfile);
		return FWTS_ERROR;
	}
	(void)unlink(amlfile);

	/* Read in the disassembled text to return later */
	*iasl_disassembly = fwts_file_open_and_read(tmpfile);

	/* Now we have a disassembled source in tmpfile, so let's assemble it */

	if (fwts_iasl_assemble_aml(tmpfile, &output_text) < 0) {
		(void)unlink(tmpfile);
		(void)unlink(tmpfile);
		free(output_text);
		return FWTS_ERROR;
	}

	(void)unlink(tmpfile);

	/* For some reason that I've not yet fathomed the ACPICA assembler 
	   leaves a .src file lying around so let's remove it to be tidy */

	snprintf(tmpfile, sizeof(tmpfile), "/tmp/fwts_iasl_%d.src", pid);
	(void)unlink(tmpfile);

	*iasl_errors = fwts_list_from_text(output_text);
	free(output_text);

	return FWTS_OK;
}
