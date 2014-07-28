/*
 * Copyright (C) 2010-2014 Canonical
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
#include "fwts_acpica.h"

#define MAX_TABLES		(128)

static fwts_acpi_table_info *iasl_cached_table_info[MAX_TABLES];
static char *iasl_cached_table_files[MAX_TABLES];
static int iasl_cached_table_file_max = 0;
static bool iasl_init = false;

/*
 *  fwts_iasl_aml_file_count()
 *	return number of cached dumped amlfiles
 */
int fwts_iasl_aml_file_count(void)
{
	if (iasl_init)
		return iasl_cached_table_file_max;
	else
		return 0;
}

/*
 *  fwts_iasl_aml_name()
 *	return back nth iASL cached table name
 */
char *fwts_iasl_aml_name(const int nth)
{
	if (iasl_init && nth < iasl_cached_table_file_max)
		return iasl_cached_table_info[nth]->name;
	else
		return "<unknown>";
}

/*
 *  fwts_iasl_dump_aml_to_file()
 *	write AML data of given length to file amlfile.
 */
static int fwts_iasl_dump_aml_to_file(
	fwts_framework *fw,
	const uint8_t *data,
	const int length,
	const char *filename)
{
	int fd;

	/* Dump the AML bytecode into a tempoary file so we can disassemble it */
	if ((fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR)) < 0) {
		fwts_log_error(fw, "Cannot create temporary file %s", filename);
		return FWTS_ERROR;
	}

	if (write(fd, data, length) != length) {
		fwts_log_error(fw, "Cannot write all data to temporary file");
		close(fd);
		(void)unlink(filename);
		return FWTS_ERROR;
	}
	close(fd);

	return FWTS_OK;
}

/*
 *  fwts_iasl_cache_table_to_file()
 *	to disassemble an APCPI table we need to dump it
 *	to file. To save effort in saving these to file
 *	multiple times, we dump out all the DSDT and
 *	SSDTs and cache the references to these.
 */
static int fwts_iasl_cache_table_to_file(fwts_framework *fw, char *tablename, int which)
{
	static pid_t pid = 0;
	char tmpname[PATH_MAX];
	fwts_acpi_table_info *table;
	int ret;

	if (iasl_cached_table_file_max >= MAX_TABLES) {
		fwts_log_error(fw, "Temporary ACPI table lookup table full.");
		return FWTS_ERROR;
	}
	ret = fwts_acpi_find_table(fw, tablename, which, &table);
	if (ret != FWTS_OK) {
		fwts_log_error(fw, "Cannot load ACPI table %s.", tablename);
		return ret;
	}
	if (table == NULL)
		return FWTS_NO_TABLE;	/* Table does not exist */
	if (!pid)
		pid = getpid();

	snprintf(tmpname, sizeof(tmpname), "/tmp/fwts_tmp_table_%d_%s_%d.dsl", pid, tablename, which);
	iasl_cached_table_files[iasl_cached_table_file_max] = strdup(tmpname);
	if (iasl_cached_table_files[iasl_cached_table_file_max] == NULL) {
		fwts_log_error(fw, "Cannot allocate cached table file name.");
		return FWTS_ERROR;
	}
	if (fwts_iasl_dump_aml_to_file(fw, table->data, table->length, tmpname) != FWTS_OK) {
		free(iasl_cached_table_files[iasl_cached_table_file_max]);
		iasl_cached_table_files[iasl_cached_table_file_max] = NULL;
		return FWTS_ERROR;
	}
	iasl_cached_table_info[iasl_cached_table_file_max] = table;
	iasl_cached_table_file_max++;
	return FWTS_OK;
}

/*
 *  fwts_iasl_deinit()
 *	clean up cached files and references
 */
void fwts_iasl_deinit(void)
{
	int i;

	for (i = 0; i < iasl_cached_table_file_max; i++) {
		if (iasl_cached_table_files[i])
			(void)unlink(iasl_cached_table_files[i]);
		iasl_cached_table_files[i] = NULL;
		iasl_cached_table_info[i] = NULL;
	}
	iasl_cached_table_file_max = 0;
}

/*
 *  fwts_iasl_init()
 *	initialise iasl - cache DSDT and SSDT to file
 */
int fwts_iasl_init(fwts_framework *fw)
{
	int i;
	int ret;

	fwts_iasl_deinit();	/* Ensure it is clean */

	ret = fwts_iasl_cache_table_to_file(fw, "DSDT", 0);
	if (ret != FWTS_OK)
		return ret;

	for (i = 0; i < MAX_TABLES; i++) {
		if (fwts_iasl_cache_table_to_file(fw, "SSDT", i) != FWTS_OK)
			break;
	}

	iasl_init = true;

	return FWTS_OK;
}

/*
 *  fwts_iasl_disassemble_to_file()
 *	Disassemble a given table and dump disassembly to a file.
 *	For tables where there are multiple matches, e.g. SSDT, we
 *	specify the Nth table with 'which'.
 */
static int fwts_iasl_disassemble_to_file(fwts_framework *fw,
	const char *tablename,
	const int which,
	const char *filename)
{
	int i, count, n = 0;

	if (!iasl_init)
		return FWTS_ERROR;

	/* Find Nth table of a given name */
	count = fwts_iasl_aml_file_count();
	for (i = 0; i < count; i++) {
		if (!strcmp(tablename, iasl_cached_table_info[i]->name)) {
			if (n == which)
				break;
			n++;
		}
	}
	if (i >= count)
		return FWTS_NO_TABLE;

	fwts_acpcia_set_fwts_framework(fw);

	if (fwts_iasl_disassemble_aml(iasl_cached_table_files, iasl_cached_table_file_max, i, filename) < 0)
		return FWTS_ERROR;

	return FWTS_OK;
}

/*
 *  fwts_iasl_disassemble()
 *	Disassemble a given table and dump disassembly list of strings.
 *	For tables where there are multiple matches, e.g. SSDT, we
 *	specify the Nth table with 'which'.
 *
 */
int fwts_iasl_disassemble(fwts_framework *fw,
	const char *tablename,
	const int which,
	fwts_list **iasl_output)
{
	char tmpfile[PATH_MAX];
	int pid = getpid();
	int ret;

	if (!iasl_init)
		return FWTS_ERROR;
	if (iasl_output == NULL)
		return FWTS_ERROR;

	*iasl_output = NULL;

	snprintf(tmpfile, sizeof(tmpfile), "/tmp/fwts_iasl_disassemble_%d_%s.dsl", pid, tablename);

	if ((ret = fwts_iasl_disassemble_to_file(fw, tablename, which, tmpfile)) != FWTS_OK)
		return ret;

	*iasl_output = fwts_file_open_and_read(tmpfile);
	(void)unlink(tmpfile);

	return *iasl_output ? FWTS_OK : FWTS_ERROR;
}


/*
 *  fwts_iasl_disassemble_all_to_file()
 * 	Disassemble DSDT and SSDT tables to separate files.
 */
int fwts_iasl_disassemble_all_to_file(fwts_framework *fw,
	const char *path)
{
	int i, n;
	int ret;
	char filename[PATH_MAX];
	char pathname[PATH_MAX];

	ret = fwts_iasl_init(fw);
	if (ret == FWTS_ERROR_NO_PRIV) {
		fprintf(stderr, "Need to have root privilege to read ACPI tables from memory! Re-run using sudo.\n");
		return FWTS_ERROR;
	}
	if (ret != FWTS_OK) {
		fprintf(stderr, "Could not initialise disassembler.\n");
		return FWTS_ERROR;
	}

	n = fwts_iasl_aml_file_count();
	if (path == NULL)
		strncpy(pathname, "", sizeof(pathname));
	else
		snprintf(pathname, sizeof(pathname), "%s/", path);

	for (i = 0; i < n; i++) {
		snprintf(filename, sizeof(filename), "%s%s%d.dsl", pathname, iasl_cached_table_info[i]->name, i);
		fwts_iasl_disassemble_to_file(fw, "DSDT", 0, filename);
		if (fwts_iasl_disassemble_aml(iasl_cached_table_files, iasl_cached_table_file_max, i, filename) < 0)
			fprintf(stderr, "Could not disassemble %s\n", iasl_cached_table_info[i]->name);
		else
			printf("Disassembled %s to %s\n", iasl_cached_table_info[i]->name, filename);
	}
	fwts_iasl_deinit();

	return FWTS_OK;
}

/*
 *  fwts_iasl_reassemble()
 *	given a ACPI table in 'data', lenth 'len, go and disassemble it
 *	and re-assemble it.  Dump the disassembly into list iasl_disassembly and
 * 	any re-assembly errors into list iasl_errors.
 */
int fwts_iasl_reassemble(fwts_framework *fw,
	const int which,
	fwts_list **iasl_disassembly,
	fwts_list **iasl_stdout,
	fwts_list **iasl_stderr)
{
	char tmpfile[PATH_MAX];
	char *stdout_output = NULL, *stderr_output = NULL;
	int pid = getpid();

	if ((!iasl_init) ||
	    (iasl_disassembly  == NULL) ||
	    (iasl_stdout == NULL) ||
	    (iasl_stderr == NULL) ||
	    (which > iasl_cached_table_file_max))
		return FWTS_ERROR;

	fwts_acpcia_set_fwts_framework(fw);

	*iasl_disassembly = NULL;

	snprintf(tmpfile, sizeof(tmpfile), "/tmp/fwts_iasl_reassemble_%d.dsl", pid);

	if (fwts_iasl_disassemble_aml(iasl_cached_table_files, iasl_cached_table_file_max, which, tmpfile) < 0) {
		(void)unlink(tmpfile);
		return FWTS_ERROR;
	}

	/* Read in the disassembled text to return later */
	*iasl_disassembly = fwts_file_open_and_read(tmpfile);

	/* Now we have a disassembled source in tmpfile, so let's assemble it */
	if (fwts_iasl_assemble_aml(tmpfile, &stdout_output, &stderr_output) < 0) {
		(void)unlink(tmpfile);
		free(stdout_output);
		return FWTS_ERROR;
	}

	/* Remove these now we don't need them */
	(void)unlink(tmpfile);

	/* And remove aml file generated from ACPICA compiler */
	snprintf(tmpfile, sizeof(tmpfile), "/tmp/fwts_iasl_reassemble_%d.aml", pid);
	(void)unlink(tmpfile);

	*iasl_stdout = fwts_list_from_text(stdout_output);
	*iasl_stderr = fwts_list_from_text(stderr_output);
	free(stdout_output);

	return FWTS_OK;
}

const char *fwts_iasl_exception_level(uint8_t level)
{
	return fwts_iasl_exception_level__(level);
}
