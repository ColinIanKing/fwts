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
#ifndef __FWTS_LIST_H__
#define __FWTS_LIST_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct fwts_list_element {
	void *data;
	struct fwts_list_element *next;
} fwts_list_element;

typedef struct {
	fwts_list_element *head;
	fwts_list_element *tail;
	int len;
} fwts_list;

typedef void (*fwlist_element_free)(void *);
typedef void (*fwts_list_foreach_callback)(void *data, void *private);
typedef int  (fwts_list_compare)(void *data1, void *data2);

fwts_list         *fwts_list_init(void);
int 		   fwts_list_len(fwts_list *list);
void               fwts_list_free(fwts_list *list, fwlist_element_free element_free);
void               fwts_list_foreach(fwts_list *list, fwts_list_foreach_callback callback, void *private);
fwts_list_element *fwts_list_append(fwts_list *list, void *data);
fwts_list_element *fwts_list_add_ordered(fwts_list *list, void *new_data, fwts_list_compare compare);

#endif
