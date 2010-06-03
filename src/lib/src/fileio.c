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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "text_list.h"
#include "fileio.h"

text_list *file_read(FILE *fp)
{
	text_list *list;
	char buffer[8192];

	if ((list = text_list_init()) == NULL)  {
		fclose(fp);
		return NULL;
	}

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		int len = strlen(buffer);
		buffer[len] = '\0';	/* Chop off "\n" */
		text_list_append(list, buffer);
	}

	return list;
}

text_list* file_open_and_read(const char *file)
{
	FILE *fp;
	text_list *list;

	if ((fp = fopen(file, "r")) == NULL)
		return NULL;

	list = file_read(fp);
	fclose(fp);

	return list;
}
