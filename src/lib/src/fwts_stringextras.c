/*
 * Copyright (C) 2010-2011 Canonical
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

#include <string.h>

#include "fwts.h"

/*
 *  fwts_chop_newline()
 *	strip off trailing \n and ' ' from end of a string
 */
void fwts_chop_newline(char *str)
{
	int len;

	if (!str)
		return;

	len = strlen(str);
	while (len > 0 && str[len-1] == '\n')
		str[--len] = '\0';

	while (len > 0 && str[len-1] == ' ')
		str[--len] = '\0';
}

/*
 *  fwts_realloc_strcat()
 *	append newstr to orig (allocated on the heap)
 *	return NULL if fails, otherwise return expanded string
 */
char *fwts_realloc_strcat(char *orig, const char *newstr)
{
	int newlen = strlen(newstr);

	if (orig) {
		if ((orig = realloc(orig, strlen(orig) + newlen + 1)) == NULL)
			return NULL;
		strcat(orig, newstr);
	} else {
		orig = malloc(newlen + 1);
		strcpy(orig, newstr);
	}
	return orig;
}
