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

#ifndef __TEXT_LIST_H__
#define __TEXT_LIST_H__

typedef struct text_list {
	char *text;
	struct text_list *next;
} text_list;

text_list *text_read(FILE *file);
void 	   text_free(text_list *head);
void	   text_dump(text_list *head);
char *	   text_strstr(text_list *head, const char *needle);

#endif
