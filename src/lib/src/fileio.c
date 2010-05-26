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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

char* file_read(const char *file)
{
	struct stat buf;
	int fd;
	char *buffer;
	char *ptr;
	int n;
	int size;

	if (stat(file, &buf) < 0)
		return NULL;

	size = buf.st_size;

	if ((buffer = malloc(size)) == NULL) 
		return NULL;

	if ((fd = open(file, O_RDONLY)) < 0) {
		free(buffer);
		return NULL;
	}

	ptr = buffer;
	do {
		if ((n = read(fd, ptr, size)) < 0) {
			free(buffer);
			return NULL;
		}
		size -= n;
		ptr += n;
	} while (size);
	
	close(fd);

	return buffer;
}
