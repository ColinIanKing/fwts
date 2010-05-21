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

#include "text_file.h"

void text_free(text_list *head)
{
	while (head) {
		text_list *next = head->next;
		if (head->text)
			free(head->text);
		free(head);
		head = next;
	}
}

text_list *text_read(FILE *file)
{
	char buffer[4096];
	text_list *head = NULL;
	text_list *prev;

	while (fgets(buffer, sizeof(buffer)-1, file) != NULL) {		
		text_list *item;

		if ((item = calloc(sizeof(text_list),1)) == NULL) {
			text_free(head);
			return NULL;
		}
		if ((item->text = strdup(buffer)) == NULL) {
			text_free(head);
			return NULL;
		}
		strcpy(item->text, buffer);
		if (head == NULL) {
			head = item;
			prev = item;
		}
		else {
			prev->next = item;
			prev = item;
		}
	}
	return head;
}

text_list text_dump(text_list *head)
{
	while (head) {
		printf("%s", head->text);
		head = head->next;
	}
}

char *text_strstr(text_list *head, const char *needle)
{
	while (head) {
		char *match;
		if ((match = strstr(head->text, needle)) != NULL) {
			return match;
		}
		head = head->next;
	}
}
