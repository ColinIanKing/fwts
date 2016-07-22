/*
 * Copyright (C) 2011-2016 Canonical
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
#include <stdint.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>

#include "fwts.h"

/*
 * We implement a low memory allocator to allow us to allocate
 * memory < 2G limit for the ACPICA table handling.  On 64 bit
 * machines we have to ensure that cached copies of ACPI tables
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
	unsigned int magic;
} fwts_mmap_header;

/*
 * CHUNK_SIZE controls the gap between mappings. This creates gaps
 * between each low memory allocation so that we have some chance
 * of catching memory accesses that fall off the mapping.  Without
 * a gap, memory mappings potentially become contiguous and hence
 * memory access errors are harder to catch.   This is wasteful
 * in terms of address space, but fwts doesn't do too many low memory
 * mappings since they are just used for cached copies of ACPI tables.
 */
#define CHUNK_SIZE	(64*1024)

#define LIMIT_2GB	(0x80000000ULL)
#define LIMIT_START	(0x00010000ULL)

/*
 *  fwts_low_mmap_walkdown()
 *	try to allocate a free space under the 2GB limit by
 *	walking down memory in CHUNK_SIZE steps for an unmapped region
 */
static void *fwts_low_mmap_walkdown(const size_t requested_size)
{
	void *addr;
	size_t page_size = fwts_page_size();
	size_t sz = (requested_size + page_size) & ~(page_size - 1);
	size_t pages = sz / page_size;
	unsigned char vec[pages];
	static void *last_addr = (void *)LIMIT_2GB;

	if (requested_size == 0)	/* Illegal */
		return MAP_FAILED;

	for (addr = last_addr - sz; addr > (void *)LIMIT_START; addr -= CHUNK_SIZE) {
		void *mapping;

		/* Already mapped? */
		if (mincore(addr, pages, vec) == 0)
			continue;

		/* Not mapped but mincore returned something unexpected? */
		if (errno != ENOMEM)
			continue;

		mapping = mmap(addr, requested_size, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
		if (mapping != MAP_FAILED) {
			last_addr = mapping;
			return mapping;
		}
	}
	/* We've scanned all of memory, give up on subsequent calls */
	last_addr = (void *)LIMIT_START;

	return MAP_FAILED;
}

/*
 *  fwts_low_mmap()
 *	try to find a free space under the 2GB limit and mmap it.
 *	returns address of mmap'd region or MAP_FAILED if failed.
 */
static void *fwts_low_mmap(const size_t requested_size)
{
	FILE *fp;
	char buffer[1024];
	char pathname[1024];
	void *addr_start;
	void *addr_end;
	void *last_addr_end = NULL;
	void *first_addr_start = NULL;
	void *ret = MAP_FAILED;

	if (requested_size == 0)	/* Illegal */
		return MAP_FAILED;

	/*
	 *  If we can't access our own mappings then find a
	 *  free page by just walking down memory
 	 */
	if ((fp = fopen("/proc/self/maps", "r")) == NULL)
		return fwts_low_mmap_walkdown(requested_size);

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		if (sscanf(buffer, "%p-%p %*s %*x %*s %*u %1023s",
		    &addr_start, &addr_end, pathname) != 3)
			continue;
		/*
		 *  Try and allocate under first mmap'd address space
		 */
		if ((first_addr_start == NULL) &&
		    (addr_start > (void*)LIMIT_START)) {
			size_t sz = (requested_size + CHUNK_SIZE) & ~(CHUNK_SIZE - 1);
			void *addr = (uint8_t*)addr_start - sz;

			/*
			 * If addr is over the 2GB limit and we know
			 * that this is the first mapping then we should
			 * be able to map a region below the 2GB limit as
			 * nothing is already mapped there
			 */
			if (addr > (void*)LIMIT_2GB)
				addr = (void*)LIMIT_2GB - sz;

			ret = mmap(addr, requested_size, PROT_READ | PROT_WRITE,
				MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
			if (ret != MAP_FAILED)
				break;	/* Success! */

			first_addr_start = addr_start;
		}

		/*
		 *  No allocation yet, so try now to squeeze one
		 *  in between already mapped regions
		 */
		if ((last_addr_end != NULL) &&
		    (last_addr_end < (void*)LIMIT_2GB)) {
			if (((uint8_t *)addr_start - (uint8_t *)last_addr_end) > (ptrdiff_t)requested_size) {
				void *addr = last_addr_end;

				ret = mmap(addr, requested_size, PROT_READ | PROT_WRITE,
					MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
				if (ret != MAP_FAILED)
					break;	/* Success! */
			}
		}

		/*
		 * We really don't want to mmap at the end of the heap
		 * and if we've reached the stack we've gone too far so
		 * abort
		 */
		if ((strncmp("[heap]", pathname, 6) == 0) ||
		    (strncmp("[stack]", pathname, 7) == 0)) {
			ret = MAP_FAILED;
			break;
		}

		last_addr_end = addr_end;
	}
	fclose(fp);

	/*
	 *  The "intelligent" memory hole finding strategy failed,
	 *  so try walking down memory instead.
	 */
	if (ret == MAP_FAILED)
		ret = fwts_low_mmap_walkdown(requested_size);

	return ret;
}

/*
 *  fwts_low_calloc()
 * 	same as calloc(), but ensure the region is mapped
 *	into the low 2GB of memory. This is required for
 *	allocated ACPI tables that need to be addressed
 *	via legacy 32 bit pointers on 64 bit architectures.
 */
void *fwts_low_calloc(const size_t nmemb, const size_t size)
{
	size_t n = nmemb * size;
	void *ret = MAP_FAILED;
	fwts_mmap_header *hdr;

	n += sizeof(fwts_mmap_header);

#ifdef MAP_32BIT
	/* Not portable, only x86 */
	ret = mmap(NULL, n, PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
#else
	if (sizeof(void *) == 4) {
		/* 32 bit mmap by default */
		ret = mmap(NULL, n, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	}
#endif
	/* 32 bit mmap failed, so bodge our own 32 bit mmap */
	if (ret == MAP_FAILED)
		ret = fwts_low_mmap(n);

	/* OK, really can't mmap, give up */
	if (ret == MAP_FAILED)
		return NULL;

	memset(ret, 0, n);

	/* save info so we can munmap() */
	hdr = (fwts_mmap_header*)ret;
	hdr->start = ret;
	hdr->size = n;
	hdr->magic = FWTS_ALLOC_MAGIC;

	return (void *)((uint8_t *)ret + sizeof(fwts_mmap_header));
}

/*
 *  fwts_low_malloc()
 */
void *fwts_low_malloc(const size_t size)
{
	return fwts_low_calloc(1, size);
}

/*
 *  fwts_low_realloc()
 *	realloc memory - ptr must be NULL or
 *	allocated with fwts_low_* allocators
 */
void *fwts_low_realloc(const void *ptr, const size_t size)
{
	void *ret;
	fwts_mmap_header *hdr;

	if (ptr == NULL)
		return fwts_low_malloc(size);

	hdr = (fwts_mmap_header *)
		((uint8_t *)ptr - sizeof(fwts_mmap_header));

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
void fwts_low_free(const void *ptr)
{
	if (ptr) {
		fwts_mmap_header *hdr = (fwts_mmap_header *)
			((uint8_t *)ptr - sizeof(fwts_mmap_header));
		if (hdr->magic == FWTS_ALLOC_MAGIC)
			munmap(hdr, hdr->size);
	}
}
