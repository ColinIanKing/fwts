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

#ifndef __FWTS_GPE_H__
#define __FWTS_GPE_H__

#define FWTS_GPE_PATH	"/sys/firmware/acpi/interrupts"

typedef struct {
	char *name;
	int count;
} fwts_gpe;

void fwts_gpe_free(fwts_gpe *gpe, int count);
int  fwts_gpe_read(fwts_gpe **gpes);
int  fwts_gpe_delta(int **gpe_delta, fwts_gpe *start, fwts_gpe *end, int n);
void fwts_gpe_test(fwts_framework *fw, fwts_gpe *start, fwts_gpe *end, int n);

#endif
