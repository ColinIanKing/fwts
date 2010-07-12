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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>

#include "fwts.h"

void fwts_gpe_free(fwts_gpe *gpe, const int count)
{
	int i;

	if (gpe == NULL)
		return;

	for (i=0;i<count;i++) {
		if (gpe[i].name)
			free(gpe[i].name);
	}
	free(gpe);
}

int fwts_gpe_read(fwts_gpe **gpes)
{
	DIR *dir;
	struct dirent *entry;

	*gpes = NULL;
	int n = 0;

	if ((dir = opendir(FWTS_GPE_PATH)) == NULL) {
		return FWTS_ERROR;
	}
	
	while ((entry = readdir(dir)) != NULL) {
		if ((strncmp(entry->d_name, "gpe", 3) == 0) ||
		    (strncmp(entry->d_name, "sci",3) == 0)) {
			if ((*gpes = realloc(*gpes, sizeof(fwts_gpe) * (n+1))) == NULL)
				goto error;
			else {
				char path[PATH_MAX];
				char *data;

				if (((*gpes)[n].name  = strdup(entry->d_name)) == NULL)
					goto error;
				
				snprintf(path, sizeof(path), "%s/%s", FWTS_GPE_PATH, entry->d_name);
				data = fwts_get(path);
				if (data) {
					(*gpes)[n].count = atoi(data);
					free(data);
				} else
					goto error;

				n++;
			}
		}
	}

	closedir(dir);
	return n;

error:
	fwts_gpe_free(*gpes, n);
	*gpes = NULL;
	closedir(dir);
	return FWTS_ERROR;
}

int fwts_gpe_delta(int **gpe_delta, const fwts_gpe *start, const fwts_gpe *end, const int n)
{	
	int i;
	if (((*gpe_delta) = calloc(n, sizeof(int))) == NULL)
		return FWTS_ERROR;

	for (i=0;i<n;i++) {
		(*gpe_delta)[i] = end[i].count - start[i].count;
	}

	return FWTS_OK;
}


void fwts_gpe_test(fwts_framework *fw, const fwts_gpe *gpes_start, const fwts_gpe *gpes_end, const int gpe_count)
{
	int *deltas = NULL;
	int sci = 0;
	int gpe = 0;

	if (fwts_gpe_delta(&deltas, gpes_start, gpes_end, gpe_count) == FWTS_ERROR)
		fwts_log_error(fw, "Cannot calculate GPE delta, out of memory.");
	else {
		int i;
		for (i=0;i<gpe_count;i++) {
			if ((strcmp(gpes_end[i].name, "sci") == 0) && (deltas[i] > 0))
				sci += deltas[i];
			if ((strncmp(gpes_end[i].name, "gpe", 3) == 0) && (deltas[i] > 0)) {
				fwts_log_info(fw, "Got %d interrupt(s) on GPE %s.", deltas[i], gpes_end[i].name);
				gpe += deltas[i];
			}
		}
	}
	free(deltas);
	
	if (sci == 0)
		fwts_failed_high(fw, "Did not detect any SCI interrupts.");

	if (gpe == 0)
		fwts_failed_high(fw, "Did not detect any GPE interrupts.");
}
