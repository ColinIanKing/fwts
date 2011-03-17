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
	
	ret = mmap(NULL, n, PROT_READ | PROT_WRITE, 
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);

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
