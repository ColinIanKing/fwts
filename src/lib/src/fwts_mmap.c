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
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fwts.h"

#define PAGE_SIZE	4096

void *fwts_mmap(const off_t start, const size_t size)
{
	int fd;
	int page_size;
	off_t offset;
	size_t length;
	void *mem;
	void *ret = FWTS_MAP_FAILED;

 	if ((page_size = sysconf(_SC_PAGE_SIZE)) == -1)
		page_size = PAGE_SIZE;	/* Guess */

	offset = ((size_t)start) & (page_size - 1);
	length = (size_t)size + offset;

	if ((fd = open("/dev/mem", O_RDONLY)) < 0)
		return ret;

	if ((mem = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, start - offset)) != MAP_FAILED)
		ret = (mem + offset);

	close(fd);

	return ret;
}

int fwts_munmap(void *mem, const size_t size)
{
	int page_size;
	off_t offset;

 	if ((page_size = sysconf(_SC_PAGE_SIZE)) == -1)
		page_size = PAGE_SIZE;	/* Guess */

	offset = ((off_t)(mem)) & (page_size - 1);

	if (munmap(mem - offset, size + offset) < 0)
		return FWTS_ERROR;

	return FWTS_OK;
}
