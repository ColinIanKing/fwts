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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include "framework.h"
#include "iasl.h"

int iasl_disassemble(log *log, framework *fw, char *src)
{
	char tmpbuf[4096];
	text_list *output;
	int ret;
	char *iasl = fw->iasl ? fw->iasl : IASL;

	snprintf(tmpbuf, sizeof(tmpbuf), "%s -d %s", iasl, src);
	ret = pipe_exec(tmpbuf, &output);
	if (ret)
		log_warning(log, "exec of %s returned %d\n", iasl, ret);
	if (output)
		text_list_free(output);

	return ret;
}

text_list *iasl_assemble(log *log, framework *fw, char *src)
{
	char tmpbuf[4096];
	int ret;
	text_list *output;
	char *iasl = fw->iasl ? fw->iasl : IASL;

	/* Run iasl with -vs just dumps out line and error output */
	snprintf(tmpbuf, sizeof(tmpbuf), "%s %s", fw->iasl ? fw->iasl : IASL, src);
	ret = pipe_exec(tmpbuf, &output);
	if (ret)
		log_warning(log, "exec of %s returned %d\n", iasl, ret);

	return output;
}
