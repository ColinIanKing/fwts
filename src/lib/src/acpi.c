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

char *acpidump = "/usr/bin/acpidump";

#define GET_UINT32(buffer, offset)	\
	( (data[offset+3] << 24) |	\
	  (data[offset+2] << 16) |	\
	  (data[offset+1] << 8)  |	\
	  (data[offset]) )
	

unsigned char *get_acpi_table(const char *name, unsigned long *size)
{
	char buffer[1024];
	pid_t pid;
	int fd;
	unsigned char *data;
	acpi_table_header hdr;
	*size = 0;
	int len;
	int i;
	unsigned char checksum = 0;

	sprintf(buffer,"%s -t %s -b", acpidump, name);
	if ((fd = pipe_open(buffer, &pid)) < 0) {
		return NULL;
	}
	data = (unsigned char*) pipe_read(fd, &len);
	pipe_close(fd, pid);

	if (data == NULL)
		return NULL;

	memcpy(&hdr.signature, data, 4);
	hdr.length = GET_UINT32(data, 4);
	hdr.revision = data[8];
	hdr.checksum = data[9];
	memcpy(&hdr.oem_id, data+10, 6);
	memcpy(&hdr.oem_tbl_id, data+16, 8);
	hdr.oem_revision = GET_UINT32(data, 22);
	memcpy(&hdr.creator_id, data+28, 4);
	hdr.creator_revision = GET_UINT32(data, 32);
	
	memcpy(&hdr.oem_tbl_id, data+16, 6);

	printf("sig: %4.4s\n", hdr.signature);
	printf("len: %x\n", hdr.length);
	printf("rev: %d\n", hdr.revision);
	printf("chk: %d\n", hdr.checksum);
	printf("oem_id: %6.6s\n", hdr.oem_id);
	printf("oem_tbl_id: %6.6s\n", hdr.oem_tbl_id);
	printf("oem_rev: %d\n", hdr.oem_revision);
	printf("comp_id: %4.4s\n", hdr.creator_id);
	printf("comp_rev: %d\n", hdr.creator_revision);

	if (len != hdr.length) {
		printf("ACPI table %s, unexpected table size %d (expected %d)\n", 
			name, len, hdr.length);
		free(data);
		return NULL;
	}

	if (strncmp(name, hdr.signature, 4) != 0) {
		printf("ACPI table %s, mismatched table name (got %4.4s)\n", 
			name, hdr.signature);
		free(data);
		return NULL;
	}

	for (i=0; i<hdr.length; i++) 
		checksum += data[i];

	if (checksum != 0) {
		printf("ACPI table %s, bad checksum: %d\n", name, checksum);
		free(data);
		return NULL;
	}

	*size = hdr.length;

	return data;
}
