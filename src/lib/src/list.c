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

	if ((list = calloc(1, sizeof(fwts_list))) == NULL)
		return NULL;

	list->head = NULL;
	list->tail = NULL;
	list->len = 0;

	return list;
}

int fwts_list_len(fwts_list *list)
{
	if (list != NULL)
		return list->len;
	else
		return 0;
}

void fwts_list_foreach(fwts_list *list, fwts_list_foreach_callback callback, void *private)
{
	fwts_list_link *item;

	if (list == NULL)
		return;

	for (item = list->head; item != NULL; item = item->next)
		callback(item->data, private);
}

void fwts_list_free(fwts_list *list, fwts_list_link_free data_free)
{
	fwts_list_link *item;
	fwts_list_link *next;

	if (list == NULL)
		return;

	for (item = list->head; item != NULL; item = next) {
		next = item->next;
		if ((item->data != NULL) && (data_free != NULL))
			data_free(item->data);
		free(item);
	}

	free(list);
}

fwts_list_link *fwts_list_append(fwts_list *list, void *data)
{
	fwts_list_link *link;

	if (list == NULL)
		return NULL;

	if ((link = calloc(sizeof(fwts_list_link),1)) == NULL)
		return NULL;

	link->data = data;

	if (list->head == NULL) {
		list->head = link;
		list->tail = link;
	} else {
		list->tail->next = link;
		list->tail = link;
	}
	list->len++;

	return link;
}

fwts_list_link *fwts_list_add_ordered(fwts_list *list, void *new_data, fwts_list_compare compare)
{
	fwts_list_link   *new_list_item;
	fwts_list_link   **list_item;

	if ((new_list_item = calloc(1, sizeof(fwts_list_link))) == NULL)
		return NULL;

	new_list_item->data = new_data;

	for (list_item = &list->head; *list_item != NULL; list_item = &(*list_item)->next) {
		void *data = (void *)(*list_item)->data;
		if (compare(data, new_data) >= 0) {
			new_list_item->next = (*list_item);
			break;
		}
	}
	if (new_list_item->next == NULL)
		list->tail = new_list_item;

	*list_item = new_list_item;

	list->len++;

	return new_list_item;
}
