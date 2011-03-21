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
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "fwts_alloc.h"

/*
 * We implement a low memory allocator to allow us to allocate
 * memory < 2G limit for the ACPICA table handling.  On 64 bit
 * machines we habe to ensure that cached copies of ACPI tables
 * have addresses that can be addressed by the legacy 32 bit 
 * ACPI table pointers.
 *
 * This implementation is not intended to be a malloc replacement
 * for all of fwts - only just for the cached ACPICA table allocations.
 *
 * Hence this implementation is not necessarily an efficient solution,
 * as it will only be used for a handful of tables.
 */

#define FWTS_ALLOC_MAGIC	0xf023cb1a

typedef struct {
	void   *start;
	size_t	size;
	int	magic;
} fwts_mmap_header;

#define LIMIT_2GB	0x80000000ULL

#ifndef MAP_32BIT
/*
 *  fwts_low_mmap()
 *	try to find a free space under the 2GB limit and mmap it.
 *	returns address of mmap'd region or MAP_FAILED if failed.
 */
static void *fwts_low_mmap(size_t requested_size)
{
	FILE *fp;
	char buffer[1024];
	char pathname[1024];
	void *addr_start;
	void *addr_end;
	void *last_addr_end = NULL;
	void *ret = MAP_FAILED;

	if (requested_size == 0)	/* Illegal */
		return MAP_FAILED;

	if ((fp = fopen("/proc/self/maps", "r")) == NULL)
		return MAP_FAILED;
	
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		sscanf(buffer, "%p-%p %*s %*x %*s %*u %s",
			&addr_start, &addr_end, pathname);

		if ((last_addr_end != NULL) &&
		    (last_addr_end < (void*)LIMIT_2GB)) {
			if ((addr_start - last_addr_end) > requested_size) {
				void *addr = last_addr_end;
				ret = mmap(addr, requested_size, PROT_READ | PROT_WRITE, 
					MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
				if (ret != MAP_FAILED)
					break;	/* Success! */
			}
		}

		/* We really don't want to mmap at the end of the heap
		 * and if we've reached the stack we've gone too far so
		 * abort */
		if ((strncmp("[heap]", pathname, 6) == 0) ||
		    (strncmp("[stack]", pathname, 7) == 0)) {
			ret = MAP_FAILED;
			break;
		}

		last_addr_end = addr_end;
	}
	fclose(fp);

	return ret;
}
#endif

/*
 *  fwts_low_calloc()
 * 	same as calloc(), but ensure the region is mapped
 *	into the low 2GB of memory. This is required for
 *	allocated ACPI tables that need to be addressed
 *	via legacy 32 bit pointers on 64 bit architectures.
 */
void *fwts_low_calloc(size_t nmemb, size_t size)
{
	size_t n = nmemb * size;
	void *ret;
	fwts_mmap_header *hdr;

	n += sizeof(fwts_mmap_header);
	
#ifdef MAP_32BIT
	/* Not portable, only x86 */
	ret = mmap(NULL, n, PROT_READ | PROT_WRITE, 
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
#else
	/* We don't have a native MAP_32BIT, so bodge our own */
	ret = fwts_low_mmap(n);
#endif

	if (ret == MAP_FAILED)
		return NULL;

	memset(ret, 0, n);

	/* save info so we can munmap() */
	hdr = (fwts_mmap_header*)ret;
	hdr->start = ret;
	hdr->size = n;
	hdr->magic = FWTS_ALLOC_MAGIC;

	return (ret + sizeof(fwts_mmap_header));
}

/*
 *  fwts_low_malloc()
 */
void *fwts_low_malloc(size_t size)
{
	return fwts_low_calloc(1, size);
}

/*
 *  fwts_low_realloc()
 *	realloc memory - ptr must be NULL or
 *	allocated with fwts_low_* allocators
 */
void *fwts_low_realloc(void *ptr, size_t size)
{
	void *ret;
	fwts_mmap_header *hdr;

	if (ptr == NULL)
		return fwts_low_malloc(size);

	hdr = (fwts_mmap_header *)
		(ptr - sizeof(fwts_mmap_header));

	/* sanity check */
	if (hdr->magic != FWTS_ALLOC_MAGIC) 
		return NULL;

	if ((ret = fwts_low_malloc(size)) == NULL)
		return NULL;

	memcpy(ret, ptr, hdr->size - sizeof(fwts_mmap_header));
	fwts_low_free(ptr);

	return ret;
}

/*
 *  fwts_low_free()
 *	free memory allocated by fwts_low_calloc()
 */
void fwts_low_free(void *ptr)
{
	if (ptr) {
		fwts_mmap_header *hdr = (fwts_mmap_header *)
			(ptr - sizeof(fwts_mmap_header));
		if (hdr->magic == FWTS_ALLOC_MAGIC)
			munmap(hdr, hdr->size);
	}
}
