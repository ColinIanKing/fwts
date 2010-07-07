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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "fwts.h"

static void fwts_keymap_keycode_free(void *data)
{
	fwts_keycode *keycode = (fwts_keycode*)data;
	free(keycode->keyname);
	free(keycode->keytext);
	free(keycode);
}

void fwts_keymap_free(fwts_list *keylist)
{
	fwts_list_free(keylist, fwts_keymap_keycode_free);
}

fwts_list *fwts_keymap_load(const char *machine)
{	
	FILE *fp;
	char buffer[4096];
	char path[PATH_MAX];
	fwts_list *keymap_list;

	if ((keymap_list = fwts_list_init()) == NULL)
		return NULL;
	
	snprintf(path, sizeof(path), "%s/%s", FWTS_KEYMAP_PATH, machine);

	if ((fp = fopen(path, "r")) == NULL)
		return NULL;

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		fwts_keycode *key;
		char *str = buffer;
		int scancode;
		char *keyname = NULL;
		char *keytext = NULL;

		scancode = strtoul(buffer, &str, 16);

		if (str == NULL || *str == '\0')
			continue;

		/* Skip over whitespace */
		while (*str != '\0' && isspace(*str))
			str++;
		if (*str == '\0')
			continue;

		keyname = str;
		/* Skip over keyname */
		while (*str != '\0' && !isspace(*str))
			str++;

		keytext = keyname;  /* Default if we cannot find human readable name */
		if (*str != '\0') {
			*str++ = '\0';
			if ((str = strstr(str, "#")) != NULL) {
				str++;
				while (*str != '\0' && isspace(*str))
					str++;

				if (*str) {
					keytext = str;
					keytext[strlen(keytext)-1] = '\0';
				}
			}
		}

		if ((key = (fwts_keycode*)calloc(1, sizeof(fwts_keycode))) == NULL) {
			fclose(fp);
			fwts_keymap_free(keymap_list);
			return NULL;
		} else {
			key->scancode = scancode;
			key->keyname = strdup(keyname);
			key->keytext = strdup(keytext);
			fwts_list_append(keymap_list, key);
		}
	}

	fclose(fp);
	
	return keymap_list;
}

fwts_keycode *fwts_keymap_find_scancode(fwts_list *keymap, int scancode)
{
	fwts_list_link *link;

	for (link = keymap->head; link != NULL; link = link->next) {
		fwts_keycode *keycode = (fwts_keycode*)link->data;
		if (keycode->scancode == scancode)
			return keycode;
	}
	return NULL;
}
