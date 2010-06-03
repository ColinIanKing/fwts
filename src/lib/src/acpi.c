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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "acpi.h"
#include "pipeio.h"
#include "log.h"

char *acpidump = "/usr/bin/acpidump";

unsigned char *get_acpi_table(log *results, const char *name, unsigned long *size)
{
	char buffer[1024];
	pid_t pid;
	int fd;
	u8 *data;
	acpi_table_header hdr;
	*size = 0;
	int len;
	int i;
	unsigned char checksum = 0;

	sprintf(buffer,"%s -t %s -b", acpidump, name);
	if ((fd = pipe_open(buffer, &pid)) < 0) {
		return NULL;
	}
	data = (u8*) pipe_read(fd, &len);
	pipe_close(fd, pid);

	if (data == NULL)
		return NULL;

	if (len < 36) {
		*size = len;
		return data;
	}
	
	memset(&hdr, 0xff, sizeof(acpi_table_header));

	memcpy(&hdr.signature, data, 4);
	GET_UINT32(hdr.length, data, 4);
	GET_UINT8 (hdr.revision, data, 8);
	GET_UINT8 (hdr.checksum, data, 9);
	memcpy(&hdr.oem_id, data+10, 6);
	memcpy(&hdr.oem_tbl_id, data+16, 8);
	GET_UINT32(hdr.oem_revision, data, 22);
	memcpy(&hdr.creator_id, data+28, 4);
	GET_UINT32(hdr.creator_revision, data, 32);
	memcpy(&hdr.oem_tbl_id, data+16, 6);
	
	/*
	printf("sig: %4.4s\n", hdr.signature);
	printf("len: %lu\n", hdr.length);
	printf("rev: %d\n", hdr.revision);
	printf("chk: %d\n", hdr.checksum);
	printf("oem_id: %6.6s\n", hdr.oem_id);
	printf("oem_tbl_id: %6.6s\n", hdr.oem_tbl_id);
	printf("oem_rev: %lu\n", hdr.oem_revision);
	printf("comp_id: %4.4s\n", hdr.creator_id);
	printf("comp_rev: %lu\n", hdr.creator_revision);
	*/

	if (len != hdr.length) {
		log_error(results, "ACPI table %s, unexpected table size %d (expected %lu)\n", 
			name, len, hdr.length);
		free(data);
		return NULL;
	}

	if (strncmp(name, hdr.signature, 4) != 0) {
		log_error(results, "ACPI table %s, mismatched table name (got %4.4s)\n", 
			name, hdr.signature);
		free(data);
		return NULL;
	}

	/* Header checksum is sum of header in 8 bit adds is zero */
	for (i=0; i<hdr.length; i++) 
		checksum += data[i];

	if (!checksum) {
		log_error(results, "ACPI table %s, bad checksum: %d\n", name, checksum);
		free(data);
		return NULL;
	}

	*size = hdr.length;

	return data;
}
