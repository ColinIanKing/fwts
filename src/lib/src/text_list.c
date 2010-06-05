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

#include "framework.h"
#include "text_list.h"

fwts_text_list *fwts_text_list_init(void)
{
	fwts_text_list *list;

	if ((list = malloc(sizeof(fwts_text_list))) == NULL)
		return NULL;

	list->head = NULL;
	list->tail = NULL;

	return list;
}

void fwts_text_list_free(fwts_text_list *list)
{
	fwts_text_list_element *item;
	fwts_text_list_element *next;

	if (list == NULL)
		return;

	for (item = list->head; item != NULL; item = next) {
		next = item->next;
		if (item->text)
			free(item->text);
		free(item);
	}
	free(list);
}

fwts_text_list_element *fwts_text_list_append(fwts_text_list *list, const char *text)
{
	fwts_text_list_element *item;

	if (list == NULL)
		return NULL;

	if ((item = calloc(sizeof(fwts_text_list_element),1)) == NULL)
		return NULL;

	if (text == NULL)
		text = "";

	if ((item->text = malloc(strlen(text) + 1)) == NULL) {
		free(item);
		return NULL;
	}
	strcpy(item->text, text);

	if (list->head == NULL) {
		list->head = item;
		list->tail = item;
	} else {
		list->tail->next = item;
		list->tail = item;
	}
	return item;
}

void fwts_text_list_dump(fwts_text_list *list)
{
	fwts_text_list_element *item;

	if (list == NULL)
		return;

	for (item = list->head; item != NULL; item = item->next)
		printf("DUMP: %s\n", item->text);
}

char *fwts_text_list_strstr(fwts_text_list *list, const char *needle)
{
	fwts_text_list_element *item;
	char *match;

	if (list == NULL)
		return NULL;

	for (item = list->head; item != NULL; item = item->next)
		if ((match = strstr(item->text, needle)) != NULL)
			return match;

	return NULL;
}

fwts_text_list *fwts_text_list_from_text(char *text)
{
	char *ptr;
        fwts_text_list *list;
	fwts_text_list_element *item;

	if (!text && !*text)
		return NULL;

	if ((list = fwts_text_list_init()) == NULL)
		return NULL;

	ptr = text;

	while (*ptr) {
		char *start = ptr;
		int len;


		while (*ptr && *ptr !='\n')
			ptr++;

		len = ptr - start;

		if (*ptr == '\n')
			ptr++;

		if ((item = calloc(sizeof(fwts_text_list_element),1)) == NULL) {
			fwts_text_list_free(list);
			return NULL;
		}
		if ((item->text = malloc(len + 1)) == NULL) {
			fwts_text_list_free(list);
			return NULL;
		}
		strncpy(item->text, start, len);
		item->text[len] = '\0';

		if (list->head == NULL) {
			list->head = item;
			list->tail= item;
                } else {
			list->tail->next = item;
			list->tail = item;
		}
	}
	
	return list;
}
