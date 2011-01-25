/*
 * Copyright (C) 2010-2011 Canonical
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "fwts.h"
#include "fwts_uefi.h"


static inline void fwts_uefi_set_filename(char *filename, int len, const char *varname)
{
	snprintf(filename, len, "/sys/firmware/efi/vars/%s/raw_var", varname);
}

/*
 *  fwts_uefi_str16_to_str()
 *	convert 16 bit string to 8 bit C string.
 */
void fwts_uefi_str16_to_str(char *dst, int len, uint16_t *src)
{
	while ((*src) && (len > 1)) {
		*dst++ = *(src++) & 0xff;		
		len--;
	}
	*dst = '\0';
}

/*
 *  fwts_uefi_str16len()
 *	16 bit version of strlen()
 */
int fwts_uefi_str16len(uint16_t *str)
{
	int i;

	for (i=0; *str; i++, str++)
		;
	return i;
}

/*
 *  fwts_uefi_get_varname()
 *	fetch the UEFI variable name in terms of a 8 bit C string
 */
void fwts_uefi_get_varname(char *varname, int len, fwts_uefi_var *var)
{
	fwts_uefi_str16_to_str(varname, len, var->varname);
}

/*
 *  fwts_uefi_get_variable()
 *	fetch a UEFI variable given its name.
 */
int fwts_uefi_get_variable(const char *varname, fwts_uefi_var *var)
{
	int  fd;
	int  n;
	int  ret = FWTS_OK;
	char filename[PATH_MAX];

	if ((!varname) || (!var))
		return FWTS_ERROR;

	fwts_uefi_set_filename(filename, sizeof(filename), varname);

	if ((fd = open(filename, O_RDONLY)) < 0) 
		return FWTS_ERROR;

	memset(var, 0, sizeof(fwts_uefi_var));

	if ((n = read(fd, var, sizeof(fwts_uefi_var))) != sizeof(fwts_uefi_var))
		ret = FWTS_ERROR;

	close(fd);

	return ret;
}

/*
 *  fwts_uefi_set_variable()
 *	write back a UEFI variable given its name and contents in var.
 */
int fwts_uefi_set_variable(const char *varname, fwts_uefi_var *var)
{
	int  fd;
	int  n;
	int  ret = FWTS_OK;
	char filename[PATH_MAX];

	if ((!varname) || (!var))
		return FWTS_ERROR;

	fwts_uefi_set_filename(filename, sizeof(filename), varname);

	if ((fd = open(filename, O_WRONLY)) < 0) 
		return FWTS_ERROR;

	if ((n = write(fd, var, sizeof(fwts_uefi_var))) != sizeof(fwts_uefi_var))
		ret = FWTS_ERROR;

	close(fd);

	return ret;
}
