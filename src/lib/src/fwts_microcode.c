/*
 *
 * Copyright 2000 (c) Simon Trimmer, Tigran Aivazian.
 * Copyright (C) 2010-2011 Canonical
 *
 * Originally microcode_ctl.c
 * Manipulate /dev/cpu/microcode under Linux
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "fwts.h"

#define BUFFER_SIZE	(4096)
#define MICROCODE_SIZE 	(128*1024) 	/* initial size, expanded as needed */

/*
 *  fwts_update_microcode()
 * 	load given microcode file into a processor
 */
int fwts_update_microcode(fwts_framework *fw, const char *device, const char *filename)
{
	FILE *fp;
	char line_buffer[BUFFER_SIZE];
	size_t microcode_size = MICROCODE_SIZE/sizeof(unsigned);
	unsigned *microcode;
	size_t pos = 0;
	int fd;
	int wrote, length;

	if ((microcode = calloc(1, microcode_size * sizeof(unsigned))) == NULL) {
		fwts_log_error(fw, "Cannot allocate memory.");
		return FWTS_ERROR;
	}

	if ((fp = fopen(filename, "r")) == NULL) {
		fwts_log_error(fw, "Cannot open source '%s'.", filename);
		return FWTS_ERROR;
	}

	while (fgets(line_buffer, BUFFER_SIZE, fp) != NULL) {
		/*
		 * Expand microcode buffer if needed
		 */
		if (microcode_size < pos + 4) {
                    microcode_size *= 2;
                    microcode = realloc(microcode,
                                       microcode_size * sizeof(unsigned int));
			if (microcode == NULL) {
				fwts_log_error(fw, "Cannot allocate memory.");
				fclose(fp);
				return FWTS_ERROR;
			}
		}
		/*
		 * Data lines will are of the form "%x, %x, %x, %x", therefore
		 * lines start with a 0
		 */
		if (*line_buffer == '0') {
			int scanned;
			scanned = sscanf(line_buffer, "%x, %x, %x, %x",
					microcode + pos,
					microcode + pos + 1,
					microcode + pos + 2,
					microcode + pos + 3);
			if (scanned != 4) {
				fwts_log_error(fw, "%s: invalid file format", filename);
				fclose(fp);
				free(microcode);
				return FWTS_ERROR;
			}
			pos += 4;
		}
	}
	fclose(fp);

	length = pos * sizeof(unsigned int);

	if ((fd = open(device, O_WRONLY)) == -1) {
		fwts_log_error(fw, "Cannot open %s for writing errno=%d (%s)\n",
				device, errno, strerror(errno));
		free(microcode);
		return FWTS_ERROR;
	}

	if ((wrote = write(fd, microcode, length)) < 0) {
		fwts_log_error(fw, "Error writing microcode.");
		close(fd);
		free(microcode);
		return FWTS_ERROR;
	}

	close(fd);	
	free(microcode);

	return FWTS_OK;
}
