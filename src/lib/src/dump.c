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

static int dump_data(char *path, char *filename, char *data, int len)
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


static int dump_dmesg(char *path, char *filename)
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

static int dump_exec(char *path, char *filename, char *command)
{
	int fd;
	pid_t pid;
	int len;
	char *data;

	if ((fd = fwts_pipe_open(command, &pid)) < 0)
		return FWTS_ERROR;

	if ((data = fwts_pipe_read(fd, &len)) == NULL) {
		fwts_pipe_close(fd, pid);
		return FWTS_ERROR;
	}

	fwts_pipe_close(fd, pid);
	
	return dump_data(path, filename, data, len);
}

static int dump_dmidecode(fwts_framework *fw, char *path, char *filename)
{
	return dump_exec(path, filename, fw->dmidecode);
}

static int dump_lspci(fwts_framework *fw, char *path, char *filename)
{
	char command[1024];
	
	snprintf(command, sizeof(command), "%s -vv -nn", fw->lspci);

	return dump_exec(path, filename, command);
}

static void dump_acpi_data(FILE *fp, unsigned char *data, int n)
{
	int i;

	for (i=0;i<n;i++)
		fprintf(fp, "%2.2x ", data[i]);

	for (;i<16;i++)
		fprintf(fp, "   ");

	fprintf(fp, " ");

	for (i=0;i<n;i++)
		fprintf(fp, "%c", (data[i] < 32 || data[i] > 126) ? '.' : data[i]);

	fprintf(fp, "\n");
}

static int dump_acpi_table(const char *pathname, const char *tablename, FILE *fpout)
{
	FILE *fpin;
	int ch;
	int n = 0;
	int i = 0;
	unsigned char data[16];

	if ((fpin = fopen(pathname, "r")) == NULL) {
		fclose(fpout);
		unlink(pathname);
		return FWTS_ERROR;
	}

	fprintf(fpout, "%s @ 0x00000000\n", tablename);

	while ((ch = fgetc(fpin)) != EOF) {
		data[i] = ch;
		if (i == 0)
			fprintf(fpout, "  %4.4x: ", n);
		if (i == 15)
			dump_acpi_data(fpout, data, 16);
		n++;
		i = n & 15;
	}
	if (i != 0)
		dump_acpi_data(fpout, data, i);

	fprintf(fpout, "\n");

	return FWTS_OK;
}

static int dump_all_acpi_tables(char *tablepath, FILE *fpout)
{
	DIR *dir;
	struct dirent *entry;
	struct stat buf;

	if ((dir = opendir(tablepath)) == NULL) {
		return FWTS_ERROR;
	}
	
	while ((entry = readdir(dir)) != NULL) {
		if (strlen(entry->d_name) > 2) {
			char pathname[PATH_MAX];

			snprintf(pathname, sizeof(pathname), "%s/%s", tablepath, entry->d_name);
			if (stat(pathname, &buf) != 0)
				continue;
			if (S_ISDIR(buf.st_mode))
				dump_all_acpi_tables(pathname, fpout);
			else
				dump_acpi_table(pathname, entry->d_name, fpout);
		}
	}
	closedir(dir);

	return FWTS_OK;
}

static int dump_acpi_tables(char *path)
{
	char filename[PATH_MAX];
	FILE *fp;

	snprintf(filename, sizeof(filename), "%s/acpidump.log", path);

	if ((fp = fopen(filename, "w")) == NULL)
		return FWTS_ERROR;
		
	dump_all_acpi_tables(FWTS_ACPI_TABLES_PATH, fp);

	fclose(fp);
		
	return FWTS_OK;
}

static int dump_readme(char *path)
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

int fwts_dump_info(fwts_framework *fw, char *path)
{
	if (path == NULL)
		path = "./";

	if (access(path, F_OK) != 0)
		mkdir(path, 0777);

	if (dump_readme(path) != FWTS_OK)
		fprintf(stderr, "Failed to dump README.txt.\n");
	if (dump_dmesg(path, "dmesg.log") != FWTS_OK) 
		fprintf(stderr, "Failed to dump kernel log.\n");
	if (dump_dmidecode(fw, path, "dmidecode.log") != FWTS_OK)
		fprintf(stderr, "Failed to dump output from dmidecode.\n");
	if (dump_lspci(fw, path, "lspci.log") != FWTS_OK)
		fprintf(stderr, "Failed to dump output from lspci.\n");
	if (dump_acpi_tables(path) != FWTS_OK) 
		fprintf(stderr, "Failed to dump ACPI tables.\n");

	return FWTS_OK;
}
