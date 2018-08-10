/*
 * Copyright (C) 2010-2018 Canonical
 * Copyright (C) 2017-2018 ARM Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
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
#include "fwts.h"

#if defined(FWTS_HAS_ACPI)
#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"
#include "actables.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <inttypes.h>

#define TABLE_NAME_LEN (16)
#define MIN_SIG        ( 4)
#define OEM_ID         ( 6)
#define OEM_TABLE_ID   ( 8)
#define OEM_CREATOR_ID ( 4)

static bool acpi_table_check_field(const char *field, const size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		if (!isascii(field[i]))
			return false;

	return true;
}

static bool acpi_table_check_field_test(
	fwts_framework *fw,
	const char *table_name,
	const char *field_name,
	const char *field,
	const size_t len)
{
	if (!acpi_table_check_field(field, len)) {
		fwts_failed(fw, LOG_LEVEL_LOW, "ACPITableHdrInfo",
			"ACPI Table %s has non-ASCII characters in "
			"header field %s", table_name, field_name);
		return false;
	}
	return true;
}

/* Callback function used when searching for processor devices in namespace. */
static ACPI_STATUS processor_handler(ACPI_HANDLE ObjHandle, uint32_t level, void *context,
                              void **returnvalue)
{
	ACPI_NAMESPACE_NODE *node = (ACPI_NAMESPACE_NODE *)ObjHandle;
	ACPI_NAMESPACE_NODE *parent = node->Parent;

	/* Unused parameters trigger errors. */
	FWTS_UNUSED(level);
	FWTS_UNUSED(context);

	/* If the processor device is not located under _SB_, increment the error_count. */
	if (strncmp(parent->Name.Ascii, "_SB_", sizeof(int32_t)) != 0) {
		int error_count;

		error_count = *((int *)returnvalue);
		error_count++;
		*((int *)returnvalue) = error_count;
	}

	/* Return 0 so namespace search continues. */
	return 0;
}

/* Test function that makes sure processors are under the _SB_ namespace. */
static int acpi_table_sbbr_namespace_check_test1(fwts_framework *fw)
{
	int error_count = 0;

	/* Initializing ACPICA library so we can call AcpiWalkNamespace. */
	if (fwts_acpica_init(fw) != FWTS_OK)
		return FWTS_ERROR;

PRAGMA_PUSH
PRAGMA_NULL_PTR_MATH
	/* Searching for all processor devices in the namespace. */
	AcpiWalkNamespace(ACPI_TYPE_PROCESSOR, ACPI_ROOT_OBJECT, ACPI_UINT32_MAX,
	                  processor_handler, NULL, NULL, (void **)&error_count);
PRAGMA_POP

	/* Deinitializing ACPICA, if we don't call this the terminal will break on exit. */
	fwts_acpica_deinit();

	/* error_count variable counts the number of processors outside of the _SB_ namespace. */
	if (error_count > 0)
		fwts_failed(fw, LOG_LEVEL_HIGH, "SbbrAcpiCpuWrongNamespace", "%d Processor devices "
		            "were found outside of the _SB_ namespace.", error_count);
	else
		fwts_passed(fw, "All processor devices were located in the _SB_ namespace.");

	return FWTS_OK;
}

static int acpi_table_sbbr_check_test2(fwts_framework *fw)
{
	int i;
	bool checked = false;
	bool dsdt_checked = false;
	bool ssdt_checked = false;

	for (i = 0; ; i++) {
		fwts_acpi_table_info *info;

		if (fwts_acpi_get_table(fw, i, &info) != FWTS_OK)
			break;
		if (info == NULL)
			continue;

		checked = true;
		if (!strcmp(info->name, "DSDT") ||
		    !strcmp(info->name, "SSDT")) {
			fwts_acpi_table_header *hdr;
			char name[TABLE_NAME_LEN];
			bool passed = false;

			if (!strcmp(info->name, "DSDT")) {
				dsdt_checked = true;
			}
			if (!strcmp(info->name, "SSDT")) {
				ssdt_checked = true;
			}
			hdr = (fwts_acpi_table_header *)info->data;
			if (acpi_table_check_field(hdr->signature, MIN_SIG)) {
				snprintf(name, sizeof(name), "%4.4s", hdr->signature);
			} else {
				/* Table name not printable, so identify it by the address */
				snprintf(name, sizeof(name), "at address 0x%" PRIx64, info->addr);
			}

			/*
			 * Tables shouldn't be short, however, they do have at
			 * least 4 bytes with their signature else they would not
			 * have been loaded by this stage.
			 */
			if (hdr->length < sizeof(fwts_acpi_table_header)) {
				fwts_failed(fw, LOG_LEVEL_HIGH, "ACPITableHdrShort",
					"ACPI Table %s is too short, only %d bytes long. Further "
					"header checks will be omitted.", name, hdr->length);
				continue;
			}
			/* Warn about empty tables */
			if (hdr->length == sizeof(fwts_acpi_table_header)) {
				fwts_warning(fw,
					"ACPI Table %s is empty and just contains a table header. Further "
					"header checks will be omitted.", name);
				continue;
			}

			passed = acpi_table_check_field_test(fw, name, "Signature", hdr->signature, MIN_SIG) &
			    acpi_table_check_field_test(fw, name, "OEM ID", hdr->oem_id, OEM_ID) &
			    acpi_table_check_field_test(fw, name, "OEM Table ID", hdr->oem_tbl_id, OEM_TABLE_ID) &
			    acpi_table_check_field_test(fw, name, "Creator ID", hdr->creator_id, OEM_CREATOR_ID);
			if (passed)
				fwts_passed(fw, "Table %s has valid signature and ID strings.", name);
		}
	}
	if (!checked) {
		fwts_aborted(fw, "Cannot find any ACPI tables.");
		return FWTS_ABORTED;
	}
	if (!dsdt_checked)
		fwts_failed(fw, LOG_LEVEL_HIGH, "acpi_table_check_test4",
				"Test DSDT table is NOT implemented.");
	if (!ssdt_checked)
		fwts_warning(fw, "SSDT table is NOT implemented.");

	return FWTS_OK;
}

/* List of ACPI tables recommended by SBBR 4.2.2 */
static const char *recommended_acpi_tables[] = {
	"MCFG",
	"IORT",
	"BERT",
	"EINJ",
	"ERST",
	"HEST",
	"RASF",
	"SPMI",
	"SLIT",
	"SRAT",
	"CSRT",
	"ECDT",
	"MPST",
	"PCCT",
	NULL
};

/* Searches ACPI tables by signature. */
fwts_acpi_table_info *sbbr_search_acpi_tables(fwts_framework *fw, const char *signature)
{
	uint32_t i;
	fwts_acpi_table_info *info;

	i = 0;
	while (fwts_acpi_get_table(fw, i, &info) == FWTS_OK) {
		if (info != NULL && strncmp(info->name, signature, sizeof(uint32_t)) == 0) {
			return info;
		}
		i++;
	}

	return NULL;
}

static int acpi_table_sbbr_check_test3(fwts_framework *fw)
{
	uint32_t i;

	for (i = 0; recommended_acpi_tables[i] != NULL; i++) {
		fwts_acpi_table_info *info;

		info = sbbr_search_acpi_tables(fw, recommended_acpi_tables[i]);
		if (info == NULL) {
			fwts_warning(fw, "SBBR Recommended ACPI table \"%s\" not found.",
			            recommended_acpi_tables[i]);
		} else {
			fwts_passed(fw, "SBBR Recommended ACPI table \"%s\" found.",
			            recommended_acpi_tables[i]);
		}
	}
	return FWTS_OK;
}

static fwts_framework_minor_test acpi_table_sbbr_check_tests[] = {
	{ acpi_table_sbbr_namespace_check_test1, "Test that processors only exist in the _SB namespace." },
	{ acpi_table_sbbr_check_test2, "Test DSDT and SSDT tables are implemented." },
	{ acpi_table_sbbr_check_test3, "Check for recommended ACPI tables." },
	{ NULL, NULL }
};

static fwts_framework_ops acpi_table_sbbr_check_ops = {
	.description = "ACPI table headers sanity tests.",
	.minor_tests = acpi_table_sbbr_check_tests
};

FWTS_REGISTER("acpi_sbbr", &acpi_table_sbbr_check_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_TEST_SBBR)

#endif
