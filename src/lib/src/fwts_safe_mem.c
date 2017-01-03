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
static struct sigaction old_action;

/*
 *  If we hit a SIGSEGV then the port read
 *  failed and we longjmp back and return
 *  FWTS_ERROR
 */
static void segv_handler(int dummy)
{
	FWTS_UNUSED(dummy);

	fwts_sig_handler_restore(SIGSEGV, &old_action);
	siglongjmp(jmpbuf, 1);
}

/*
 *  fwts_safe_memcpy()
 *	memcpy that catches segfaults. to be used when
 *	attempting to read BIOS tables from memory which
 *	may segfault if the src address is corrupt
 */
int fwts_safe_memcpy(void *dst, const void *src, const size_t n)
{
	if (sigsetjmp(jmpbuf, 1) != 0)
		return FWTS_ERROR;
	
	fwts_sig_handler_set(SIGSEGV, segv_handler, &old_action);
	memcpy(dst, src, n);
	fwts_sig_handler_restore(SIGSEGV, &old_action);

	return FWTS_OK;
}

int fwts_safe_memread(const void *src, const size_t n)
{
	unsigned char buf[n];
	
	return fwts_safe_memcpy(buf, src, n);
}
