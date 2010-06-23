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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "fwts.h"

int fwts_printf(fwts_framework *fw, char *fmt, ...)
{
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return len;
}

int fwts_press_enter(fwts_framework *fw)
{
	fprintf(stdout, "Press <Enter> to continue");
	
	while (getchar() != '\n')
		;
	
	return FWTS_OK;
}
