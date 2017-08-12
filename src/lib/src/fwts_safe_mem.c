/*
 * Copyright (C) 2014-2017 Canonical
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
#include <signal.h>
#include <setjmp.h>

#include "fwts.h"

static sigjmp_buf jmpbuf;
static struct sigaction old_segv_action, old_bus_action;

/*
 *  If we hit a SIGSEGV or SIGBUS then the read
 *  failed and we longjmp back and return
 *  FWTS_ERROR
 */
static void sig_handler(int dummy)
{
	FWTS_UNUSED(dummy);

	fwts_sig_handler_restore(SIGSEGV, &old_segv_action);
	fwts_sig_handler_restore(SIGBUS, &old_bus_action);
	siglongjmp(jmpbuf, 1);
}

/*
 *  fwts_safe_memcpy()
 *	memcpy that catches SIGSEFV/SIGBUS. To be used when
 *	attempting to read BIOS tables from memory which
 *	may segfault or throw a bus error if the src
 *	address is corrupt
 */
int OPTIMIZE0 fwts_safe_memcpy(void *dst, const void *src, const size_t n)
{
	if (sigsetjmp(jmpbuf, 1) != 0)
		return FWTS_ERROR;

	fwts_sig_handler_set(SIGSEGV, sig_handler, &old_segv_action);
	fwts_sig_handler_set(SIGBUS, sig_handler, &old_bus_action);
	memcpy(dst, src, n);
	fwts_sig_handler_restore(SIGSEGV, &old_segv_action);
	fwts_sig_handler_restore(SIGBUS, &old_bus_action);

	return FWTS_OK;
}

/*
 *  fwts_safe_memread()
 *	check we can safely read a region of memory. This catches
 *	SIGSEGV/SIGBUS errors and returns FWTS_ERROR if it is not
 *	readable or FWTS_OK if it's OK.
 */
int OPTIMIZE0 fwts_safe_memread(const void *src, const size_t n)
{
	static uint8_t buffer[256];
	const uint8_t *ptr, *end = src + n;
	uint8_t *bufptr;
	const uint8_t *bufend = buffer + sizeof(buffer);

	if (sigsetjmp(jmpbuf, 1) != 0)
		return FWTS_ERROR;

	fwts_sig_handler_set(SIGSEGV, sig_handler, &old_segv_action);
	fwts_sig_handler_set(SIGBUS, sig_handler, &old_bus_action);

	for (bufptr = buffer, ptr = src; ptr < end; ptr++) {
		/* Force data to be read */
		__builtin_prefetch(ptr, 0, 3);
		*bufptr = *ptr;
		bufptr++;
		bufptr = (bufptr >= bufend) ? buffer : bufptr;
	}

	fwts_sig_handler_restore(SIGSEGV, &old_segv_action);
	fwts_sig_handler_restore(SIGBUS, &old_bus_action);

	return FWTS_OK;
}
