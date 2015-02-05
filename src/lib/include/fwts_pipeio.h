/*
 * Copyright (C) 2010-2015 Canonical
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

#ifndef __FWTS_PIPEIO_H__
#define __FWTS_PIPEIO_H__

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#include "fwts.h"

#define FWTS_EXEC_ERROR		(127)

int   fwts_pipe_open(const char *command, pid_t *childpid);
char *fwts_pipe_read(const int fd, ssize_t *length);
int   fwts_pipe_close(const int fd, const pid_t pid);
int   fwts_pipe_exec(const char *command, fwts_list **list, int *status);
int   fwts_exec(const char *command, int *status);
int   fwts_write_string_to_file(const fwts_framework *fw, FILE *file, const char *str);
int   fwts_write_string_file(const fwts_framework *fw, const char *file_name, const char *str);
int   fwts_read_file_first_line(const fwts_framework *fw, const char *file_name, char **line);
bool  fwts_file_first_line_contains_string(const fwts_framework *fw, const char *file_name, const char *str);

#endif
