/*
 * Copyright (C) 2010-2018 Canonical
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
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <regex.h>
#include <fcntl.h>

#include "fwts.h"

/*
 *  fwts_log_free()
 *  free log list
 */
void fwts_log_free(fwts_list *log)
{
	fwts_text_list_free(log);
}

/*
 *  fwts_log_find_changes()
 *      find new lines added to log, clone them from new list
 *      must be freed with fwts_list_free(log_diff, NULL);
 */
fwts_list *fwts_log_find_changes(fwts_list *log_old, fwts_list *log_new)
{
        fwts_list_link *l_old, *l_new = NULL;
        fwts_list *log_diff;

        if (log_new == NULL) {
                /* Nothing new to compare, return nothing */
                return NULL;
        }
        if ((log_diff = fwts_list_new()) == NULL)
                return NULL;

        if (log_old == NULL) {
                /* Nothing in old log, so clone all of new list */
                l_new = log_new->head;
        } else {
                fwts_list_link *l_old_last = NULL;

                /* Clone just the new differences */

                /* Find last item in old log */
                fwts_list_foreach(l_old, log_old)
                        l_old_last = l_old;

                if (l_old_last) {
                        /* And now look for that last line in the new log */
                        char *old = fwts_list_data(char *, l_old_last);
                        fwts_list_foreach(l_new, log_new) {
                                const char *new = fwts_list_data(char *, l_new);

                                if (!strcmp(new, old)) {
                                        /* Found last line that matches, bump to next */
                                        l_new = l_new->next;
                                        break;
                                }
                        }
                }
        }

        /* Clone the new unique lines to the log_diff list */
        for (; l_new; l_new = l_new->next) {
                if (fwts_list_append(log_diff, l_new->data) == NULL) {
                        fwts_list_free(log_diff, NULL);
                        return NULL;
                }
        }
        return log_diff;
}
