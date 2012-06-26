/*
 * Copyright (C) 2011-2012 Canonical
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

#define FWTS_HWINFO_LISTS_SAME		(0)
#define FWTS_HWINFO_LISTS_DIFFER	(1)
#define FWTS_HWINFO_LISTS_OUT_OF_MEMORY	(-1)

/*
 *  fwts_hwinfo_get()
 *	gather H/W information
 */
int fwts_hwinfo_get(fwts_framework *fw, fwts_hwinfo *hwinfo)
{
	fwts_pipe_exec("lspci | grep Network", &hwinfo->network);
	fwts_pipe_exec("lspci | grep Ethernet", &hwinfo->ethernet);
	fwts_pipe_exec("ifconfig -a | grep -A1 '^\\w'", &hwinfo->ifconfig);
	fwts_pipe_exec("iwconfig | grep -A1 '^\\w'", &hwinfo->iwconfig);
	fwts_pipe_exec("hciconfig -a | grep -A2 '^\\w", &hwinfo->hciconfig);
	fwts_pipe_exec("lspci | grep VGA", &hwinfo->videocard);
	fwts_pipe_exec("xinput list", &hwinfo->xinput);
	fwts_pipe_exec("pactl list | grep Sink | grep -v Latency", &hwinfo->pactl);
	fwts_pipe_exec("pactl list | grep Source | grep -v Latency", &hwinfo->pactl);

	return FWTS_OK;
}

/*
 *  fwts_hwinfo_list_dump()
 *	dump out a list
 */
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

/*
 *  fwts_hwinfo_lists_dump()
 *	dump out contents of two different lists
 */
static void fwts_hwinfo_lists_dump(fwts_framework *fw, fwts_list *l1, fwts_list *l2, char *message)
{
	fwts_log_info(fw, "%s configurations differ, before:", message);
	fwts_hwinfo_list_dump(fw, l1);
	fwts_log_info(fw, "versus after:");
	fwts_hwinfo_list_dump(fw, l2);
	fwts_log_nl(fw);
}

/*
 *  fwts_hwinfo_alpha_compare()
 *	alphanumeric sort compare function
 */
static int fwts_hwinfo_alpha_compare(void *data1, void *data2)
{
        return strcmp((char *)data1, (char*)data2);
}

/*
 *  fwts_hwinfo_list_sort_alpha()
 *	clone the contents of the given list and return an
 *	alphanumerically sorted version of the list. This
 *	list has cloned contents of the original, so it has
 *	to be free'd with fwts_list_free(list, NULL) since
 *	we don't want to free the cloned items and ruin the
 *	original list
 */
static fwts_list *fwts_hwinfo_list_sort_alpha(fwts_list *list)
{
	fwts_list_link *item;
	fwts_list *sorted;
	
	if ((sorted = fwts_list_new()) == NULL)
		return NULL;

	fwts_list_foreach(item, list) {
		char *str = fwts_list_data(char *, item);
		fwts_list_add_ordered(sorted, str, fwts_hwinfo_alpha_compare);
	}

	return sorted;
}

/*
 *  fwts_hwinfo_lists_differ()
 *	check lists to see if their contents differ, return 1 for differ, 0 for match,
 *	-1 for out of memory error
 */
static int fwts_hwinfo_lists_differ(fwts_list *l1, fwts_list *l2)
{
	fwts_list_link *item1;
	fwts_list_link *item2;
	
	fwts_list *sorted1, *sorted2;

	/* Both null - both the same then */
	if ((l1 == NULL) && (l2 == NULL))
		return FWTS_HWINFO_LISTS_SAME;

	/* Either null and the other not? */
	if ((l1 == NULL) || (l2 == NULL))
		return FWTS_HWINFO_LISTS_DIFFER;

	/* Different length, then they differ */
	if (fwts_list_len(l1) != fwts_list_len(l2))
		return FWTS_HWINFO_LISTS_DIFFER;

	if ((sorted1 = fwts_hwinfo_list_sort_alpha(l1)) == NULL)
		return FWTS_HWINFO_LISTS_OUT_OF_MEMORY;
		
	if ((sorted2 = fwts_hwinfo_list_sort_alpha(l2)) == NULL) {
		fwts_list_free(sorted1, NULL);
		return FWTS_HWINFO_LISTS_OUT_OF_MEMORY;
	}

	item1 = fwts_list_head(sorted1);
	item2 = fwts_list_head(sorted2);

	while ((item1 != NULL) && (item2 != NULL)) {
		char *str1 = fwts_list_data(char *, item1);
		char *str2 = fwts_list_data(char *, item2);

		if (strcmp(str1, str2)) {
			fwts_list_free(sorted1, NULL);
			fwts_list_free(sorted2, NULL);
			return FWTS_HWINFO_LISTS_DIFFER;
		}
		item1 = fwts_list_next(item1);
		item2 = fwts_list_next(item2);
	}

	fwts_list_free(sorted1, NULL);
	fwts_list_free(sorted2, NULL);

	if ((item1 == NULL) && (item2 == NULL))
		return FWTS_HWINFO_LISTS_SAME;
	else
		return FWTS_HWINFO_LISTS_DIFFER;
}

/*
 *  fwts_hwinfo_compare()
 *	check for differences in a list and if any found, dump out both lists
 */
static void fwts_hwinfo_lists_compare(fwts_framework *fw, fwts_list *l1, fwts_list *l2, char *message, int *differences)
{
	if (fwts_hwinfo_lists_differ(l1, l2) == FWTS_HWINFO_LISTS_DIFFER) {
		(*differences)++;
		fwts_hwinfo_lists_dump(fw, l1, l2, message);
	}
}

/*
 *  fwts_hwinfo_compare()
 *	compare data in each hwinfo list, produce a diff comparison output
 */
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

/*
 *  fwts_hwinfo_free()
 *	free hwinfo lists
 */
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
