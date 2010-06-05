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

#ifndef __FWTS_TEXT_LIST_H__
#define __FWTS_TEXT_LIST_H__

typedef struct fwts_text_list_element {
	char *text;
	struct fwts_text_list_element *next;
} fwts_text_list_element;

typedef struct {
	fwts_text_list_element *head;
	fwts_text_list_element *tail;
} fwts_text_list;

fwts_text_list *fwts_text_list_init(void);
void 	        fwts_text_list_free(fwts_text_list *list);
void	        fwts_text_list_dump(fwts_text_list *list);
char *	        fwts_text_list_strstr(fwts_text_list *list, const char *needle);
fwts_text_list *fwts_text_list_from_text(char *list);
fwts_text_list_element *fwts_text_list_append(fwts_text_list *list, const char *text);

#endif
