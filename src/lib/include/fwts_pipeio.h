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

#ifndef __FWTS_PIPEIO_H__
#define __FWTS_PIPEIO_H__

#include <sys/types.h>
#include <sys/wait.h>

#include "fwts.h"

#define FWTS_EXEC_ERROR		(127)

int   fwts_pipe_open(const char *command, pid_t *childpid);
char *fwts_pipe_read(int fd, int *length);
int   fwts_pipe_close(int fd, pid_t pid);
int   fwts_pipe_exec(const char *command, fwts_list **list);

#endif
