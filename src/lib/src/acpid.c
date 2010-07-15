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

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>

#include <sys/select.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include "fwts.h"

#define ACPID_SOCKET	"/var/run/acpid.socket"

int acpi_event_open(void)
{
	struct sockaddr_un addr;
	int ret;
	int fd;

        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
                return fd;

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, ACPID_SOCKET);

        if ((ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
		close(fd);
                return ret;
	}

	fcntl(fd, F_SETFD, FD_CLOEXEC);

        return fd;
}

char *acpi_event_read(const int fd, int *length, const int timeout)
{
	char *ptr = NULL;
	char buffer[8192];	
	struct timeval tv;
	fd_set rfds;

	int ret;
	int n;
	int size = 0;
	*length = 0;

	ptr = NULL;

	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	ret = select(fd+1, &rfds, NULL, NULL, &tv);
	switch (ret) {
	case 0:	/* timeout! */
		return ptr;
	case -1:
		free(ptr);
		return NULL;
	default:
		n = read(fd, buffer, sizeof(buffer));
		if (n < 0) {
			free(ptr);
			return NULL;
		}
		else {
			ptr = realloc(ptr, size + n + 1);
			memcpy(ptr + size, buffer, n);
			size += n;
			*(ptr+size) = 0;
		}
	}
	*length = size;
	return ptr;
}

void acpi_event_close(const int fd)
{
	close(fd);
}
