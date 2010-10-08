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

#include "fwts.h"

fwts_list *fwts_text_list_init(void)
{
	return fwts_list_init();
}

void fwts_text_list_free(fwts_list *list)
{
	fwts_list_free(list, free);
}

fwts_list_link *fwts_text_list_append(fwts_list *list, const char *text)
{
	char *str;

	if ((str = strdup(text)) == NULL)
		return NULL;
	else
		return fwts_list_append(list, str);
}

void fwts_text_list_dump(fwts_list *list)
{
	fwts_list_link *item;

	if (list == NULL)
		return;

	fwts_list_foreach(item, list) {
		char *str = (char *)item->data;
		printf("DUMP: %s\n", str);
	}
}

char *fwts_text_list_strstr(fwts_list *list, const char *needle)
{
	fwts_list_link *item;
	char *match;

	if (list == NULL)
		return NULL;

	fwts_list_foreach(item, list) {
		char *str = (char *)item->data;
		if ((match = strstr(str, needle)) != NULL)
			return match;
	}

	return NULL;
}

fwts_list *fwts_list_from_text(const char *text)
{
	char *ptr;
        fwts_list *list;

	if (text == NULL)
		return NULL;

	if ((list = fwts_list_init()) == NULL)
		return NULL;

	ptr = (char*)text;

	while (*ptr) {
		char *start = ptr;
		char *str;
		int len;

		while (*ptr && *ptr !='\n')
			ptr++;

		len = ptr - start;

		if (*ptr == '\n')
			ptr++;

		if ((str = calloc(1, len + 1)) == NULL) {
			fwts_text_list_free(list);
			return NULL;
		}
		strncpy(str, start, len);
		str[len] = '\0';

		fwts_list_append(list, str);
	}
	
	return list;
}
