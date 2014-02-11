/*
 * Copyright (C) 2010-2014 Canonical
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

#ifndef __IASL_H__
#define __IASL_H__

#include "fwts.h"

int fwts_iasl_disassemble_all_to_file(fwts_framework *fw,
	const char *path);

int fwts_iasl_disassemble(fwts_framework *fw,
	const char *table,
	const int which,
	fwts_list **ias_output);

int fwts_iasl_reassemble(fwts_framework *fw,
	const uint8_t *data,
	const int len,
	fwts_list **iasl_disassembly,
	fwts_list **iasl_stdout,
	fwts_list **iasl_stderr);

#endif
