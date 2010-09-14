/*
 * Copyright (C) 2010 Canonical
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <sys/klog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fwts.h"

static int dump_data(const char *path, const char *filename, char *data, const int len)
{
	FILE *fp;
	char name[PATH_MAX];

	snprintf(name, sizeof(name), "%s/%s", path, filename);
	if ((fp = fopen(name, "w")) == NULL)
		return FWTS_ERROR;

	if ((fwrite(data, sizeof(char), len, fp) != len)) {
		fclose(fp);
		return FWTS_ERROR;
	}

	fclose(fp);
	return FWTS_OK;
}

static int dump_dmesg(const char *path, const char *filename)
{
	int len;
	char *data;
	int ret;

	if ((len = klogctl(10, NULL, 0)) < 0)
		return FWTS_ERROR;

        if ((data = calloc(1, len)) < 0)
		return FWTS_ERROR;

        if (klogctl(3, data, len) < 0) {
		free(data);
		return FWTS_ERROR;
	}
	ret = dump_data(path, filename, data, strlen(data));

	free(data);

	return ret;
}

static int dump_exec(const char *path, const char *filename, const char *command)
{
	int fd;
	pid_t pid;
	int len;
	char *data;
	int ret;

	if ((fd = fwts_pipe_open(command, &pid)) < 0)
		return FWTS_ERROR;

	if ((data = fwts_pipe_read(fd, &len)) == NULL) {
		fwts_pipe_close(fd, pid);
		return FWTS_ERROR;
	}

	fwts_pipe_close(fd, pid);
	
	ret = dump_data(path, filename, data, len);

	free(data);

	return ret;
}

static int dump_dmidecode(fwts_framework *fw, const char *path, const char *filename)
{
	return dump_exec(path, filename, fw->dmidecode);
}

static int dump_lspci(fwts_framework *fw, const char *path, const char *filename)
{
	char command[1024];
	
	snprintf(command, sizeof(command), "%s -vv -nn", fw->lspci);

	return dump_exec(path, filename, command);
}

static int dump_acpi_table(fwts_acpi_table_info *table, FILE *fp)
{
	char buffer[128];
	int n;

	fprintf(fp, "%s @ 0x%x\n", table->name, (uint32)table->addr);

	for (n = 0; n < table->length; n+=16) {
		int left = table->length - n;
		fwts_dump_raw_data(buffer, sizeof(buffer), table->data + n, n, left > 16 ? 16 : left);
		fprintf(fp, "%s\n", buffer);
	}
	fprintf(fp, "\n");

	return FWTS_OK;
}

static int dump_acpi_tables(const char *path)
{
	char filename[PATH_MAX];
	fwts_acpi_table_info *table;
	FILE *fp;
	int i;

	snprintf(filename, sizeof(filename), "%s/acpidump.log", path);
	if ((fp = fopen(filename, "w")) == NULL)
		return FWTS_ERROR;

	for (i=0; (table = fwts_acpi_get_table(i)) != NULL; i++)
		dump_acpi_table(table, fp);

	fclose(fp);
		
	return FWTS_OK;
}

static int dump_readme(const char *path)
{
	char filename[PATH_MAX];
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
	FILE *fp;
	char *str;
	int len;

	snprintf(filename, sizeof(filename), "%s/README.txt", path);

	if ((fp = fopen(filename, "w")) == NULL)
		return FWTS_ERROR;

	str = asctime(tm);
	len = strlen(str) - 1;
	fprintf(fp, "This is output captured by fwts on %*.*s.\n\n", len, len, str);

	if ((str = fwts_get("/proc/version")) != NULL) {
		fprintf(fp, "Version: %s", str);
		free(str);
	}

	if ((str = fwts_get("/proc/version_signature")) != NULL) {
		fprintf(fp, "Signature: %s", str);
		free(str);
	}

	fclose(fp);
	
	return FWTS_OK;
}

int fwts_dump_info(fwts_framework *fw, const char *path)
{
	if (path == NULL)
		path = "./";

	if (access(path, F_OK) != 0)
		mkdir(path, 0777);

	if (dump_readme(path) != FWTS_OK)
		fprintf(stderr, "Failed to dump README.txt.\n");
	else
		printf("Created README.txt\n");

	if (dump_dmesg(path, "dmesg.log") != FWTS_OK) 
		fprintf(stderr, "Failed to dump kernel log.\n");
	else
		printf("Dumping dmesg to dmesg.log\n");

	if (dump_dmidecode(fw, path, "dmidecode.log") != FWTS_OK)
		fprintf(stderr, "Failed to dump output from dmidecode.\n");
	else
		printf("Dumped DMI data to dmidecode.log\n");

	if (dump_lspci(fw, path, "lspci.log") != FWTS_OK)
		fprintf(stderr, "Failed to dump output from lspci.\n");
	else
		printf("Dumped lspci data to lspic.log\n");

	if (dump_acpi_tables(path) != FWTS_OK) 
		fprintf(stderr, "Failed to dump ACPI tables.\n");
	else
		printf("Dumped ACPI tables to acpidump.log\n");

	return FWTS_OK;
}
