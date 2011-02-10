/*
 * Copyright (C) 2011 Canonical
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

#include "fwts.h"

int fwts_hwinfo_get(fwts_framework *fw, fwts_hwinfo *hwinfo)
{
	fwts_pipe_exec("lspci | grep Network", &hwinfo->network);
	fwts_pipe_exec("lspci | grep Ethernet", &hwinfo->ethernet);
	fwts_pipe_exec("ifconfig -a | grep -A1 '^\\w'", &hwinfo->ifconfig);
	fwts_pipe_exec("iwconfig | grep -A1 '^\\w'", &hwinfo->iwconfig);
	fwts_pipe_exec("hciconfig -a | grep -A2 '^\\w", &hwinfo->hciconfig);
	fwts_pipe_exec("lspci | grep VGA", &hwinfo->videocard);
	fwts_pipe_exec("xinput list", &hwinfo->xinput);
	fwts_pipe_exec("pactl | grep Sink", &hwinfo->pactl);

	return FWTS_OK;
}

static void fwts_hwinfo_list_dump(fwts_framework *fw, fwts_list *list)
{
	fwts_list_link *item;

	if (list == NULL)
		fwts_log_info_verbatum(fw, "  (null)");
	else  {
		fwts_list_foreach(item, list) {
			char *text = fwts_list_data(char *, item);
			fwts_log_info_verbatum(fw, "  %s", text);
		}
	}
}

static void fwts_hwinfo_lists_dump(fwts_framework *fw, fwts_list *l1, fwts_list *l2, char *message)
{

	fwts_log_info(fw, "%s configurations differ, before:", message);
	fwts_hwinfo_list_dump(fw, l1);
	fwts_log_info(fw, "versus after:");
	fwts_hwinfo_list_dump(fw, l2);
	fwts_log_nl(fw);
}

static int fwts_hwinfo_lists_differ(fwts_list *l1, fwts_list *l2)
{
	fwts_list_link *item1;
	fwts_list_link *item2;

	if ((l1 == NULL) && (l2 == NULL))
		return 0;
	if ((l1 == NULL) || (l2 == NULL))
		return 1;
	if (fwts_list_len(l1) != fwts_list_len(l2))
		return 1;

	item1 = fwts_list_head(l1);
	item2 = fwts_list_head(l2);

	while ((item1 != NULL) && (item2 != NULL)) {
		char *str1 = fwts_list_data(char *, item1);
		char *str2 = fwts_list_data(char *, item2);

		if (strcmp(str1, str2))
			return 1;
		item1 = fwts_list_next(item1);
		item2 = fwts_list_next(item2);
	}

	if ((item1 == NULL) && (item2 == NULL))
		return 0;
	else
		return 1;
}

static void fwts_hwinfo_lists_compare(fwts_framework *fw, fwts_list *l1, fwts_list *l2, char *message, int *differences)
{
	if (fwts_hwinfo_lists_differ(l1, l2)) {
		(*differences)++;
		fwts_hwinfo_lists_dump(fw, l1, l2, message);
	}
}

void fwts_hwinfo_compare(fwts_framework *fw, fwts_hwinfo *hwinfo1, fwts_hwinfo *hwinfo2, int *differences)
{
	*differences = 0;

	fwts_hwinfo_lists_compare(fw, hwinfo1->network, hwinfo2->network, "Network Controller", differences);
	fwts_hwinfo_lists_compare(fw, hwinfo1->ethernet, hwinfo2->ethernet, "Ethernet Controller", differences);
	fwts_hwinfo_lists_compare(fw, hwinfo1->ifconfig, hwinfo2->ifconfig, "Network - ifconfig", differences);
	fwts_hwinfo_lists_compare(fw, hwinfo1->iwconfig, hwinfo2->iwconfig, "Network - iwconfig", differences);
	fwts_hwinfo_lists_compare(fw, hwinfo1->hciconfig, hwinfo2->hciconfig, "Bluetooth Device", differences);
	fwts_hwinfo_lists_compare(fw, hwinfo1->videocard, hwinfo2->videocard, "Video", differences);
	fwts_hwinfo_lists_compare(fw, hwinfo1->xinput, hwinfo2->xinput, "Input Devices", differences);
	fwts_hwinfo_lists_compare(fw, hwinfo1->pactl, hwinfo2->pactl, "Pulse Audio Sink", differences);
}

int fwts_hwinfo_free(fwts_hwinfo *hwinfo)
{
	if (hwinfo == NULL)
		return FWTS_ERROR;

	fwts_list_free(hwinfo->network, free);
	fwts_list_free(hwinfo->ethernet, free);
	fwts_list_free(hwinfo->ifconfig, free);
	fwts_list_free(hwinfo->iwconfig, free);
	fwts_list_free(hwinfo->hciconfig, free);
	fwts_list_free(hwinfo->videocard, free);
	fwts_list_free(hwinfo->xinput, free);
	fwts_list_free(hwinfo->pactl, free);

	return FWTS_OK;
}
