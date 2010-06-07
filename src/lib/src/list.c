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

fwts_list *fwts_list_init(void)
{
	fwts_list *list;

	if ((list = malloc(sizeof(fwts_list))) == NULL)
		return NULL;

	list->head = NULL;
	list->tail = NULL;

	return list;
}

void fwts_list_foreach(fwts_list *list, fwts_list_foreach_callback callback, void *private)
{
	fwts_list_element *item;

	if (list == NULL)
		return;

	for (item = list->head; item != NULL; item = item->next)
		callback(item->data, private);
}

void fwts_list_free(fwts_list *list, fwlist_element_free element_free)
{
	fwts_list_element *item;
	fwts_list_element *next;

	if (list == NULL)
		return;

	for (item = list->head; item != NULL; item = next) {
		next = item->next;
		if (item->data)
			element_free(item->data);
		free(item);
	}

	free(list);
}

fwts_list_element *fwts_list_append(fwts_list *list, void *data)
{
	fwts_list_element *element;

	if (list == NULL)
		return NULL;

	if ((element = calloc(sizeof(fwts_list_element),1)) == NULL)
		return NULL;

	element->data = data;

	if (list->head == NULL) {
		list->head = element;
		list->tail = element;
	} else {
		list->tail->next = element;
		list->tail = element;
	}
	return element;
}
