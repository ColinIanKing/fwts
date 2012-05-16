/*
 * Copyright (C) 2010-2012 Canonical
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
#include <string.h>

#include "fwts.h"

typedef struct {
	const fwts_tag	tag;
	const char	*tag_id;
	const char 	*tag_str;
} fwts_tag_info;

#define FWTS_TAG(tag, str)	\
	{ tag, #tag, str }

static fwts_tag_info fwts_tags[] = {
	FWTS_TAG(FWTS_TAG_NONE,	 		""),
	FWTS_TAG(FWTS_TAG_ERROR_CRITICAL,	"error-critical"),
	FWTS_TAG(FWTS_TAG_ERROR_HIGH,		"error-high"),
	FWTS_TAG(FWTS_TAG_ERROR_MEDIUM,		"error-medium"),
	FWTS_TAG(FWTS_TAG_ERROR_LOW,		"error-low"),
	FWTS_TAG(FWTS_TAG_BIOS,			"bios"),
	FWTS_TAG(FWTS_TAG_BIOS_THERMAL,		"bios-thermal"),
	FWTS_TAG(FWTS_TAG_BIOS_IRQ,		"bios-irq"),
	FWTS_TAG(FWTS_TAG_BIOS_AMD_POWERNOW,	"bios-amd-powernow"),
	FWTS_TAG(FWTS_TAG_BIOS_MMCONFIG,	"bios-mmconfig"),
	FWTS_TAG(FWTS_TAG_BIOS_DMI,		"bios-dmi"),
	FWTS_TAG(FWTS_TAG_ACPI,			"acpi"),
	FWTS_TAG(FWTS_TAG_ACPI_IO_PORT,		"acpi-io-port"),
	FWTS_TAG(FWTS_TAG_ACPI_INVALID_TABLE,	"acpi-invalid-table"),
	FWTS_TAG(FWTS_TAG_ACPI_BUFFER_OVERFLOW,	"acpi-buffer-overflow"),
	FWTS_TAG(FWTS_TAG_ACPI_AML_OPCODE,	"acpi-aml-opcode"),
	FWTS_TAG(FWTS_TAG_ACPI_NAMESPACE_LOOKUP,"acpi-namespace-lookup"),
	FWTS_TAG(FWTS_TAG_ACPI_PCI_EXPRESS,	"acpi-pci-express"),
	FWTS_TAG(FWTS_TAG_ACPI_BAD_RESULT,	"acpi-bad-result"),
	FWTS_TAG(FWTS_TAG_ACPI_NO_HANDLER,	"acpi-no-handler"),
	FWTS_TAG(FWTS_TAG_ACPI_PACKAGE_LIST,	"acpi-package-list"),
	FWTS_TAG(FWTS_TAG_ACPI_PARSE_EXEC_FAIL,	"acpi-parse-exec-fail"),
	FWTS_TAG(FWTS_TAG_ACPI_EVAL,		"acpi-eval"),
	FWTS_TAG(FWTS_TAG_ACPI_BAD_LENGTH,	"acpi-bad-length"),
	FWTS_TAG(FWTS_TAG_ACPI_BAD_ADDRESS,	"acpi-bad-address"),
	FWTS_TAG(FWTS_TAG_ACPI_METHOD_RETURN,	"acpi-method-return"),
	FWTS_TAG(FWTS_TAG_ACPI_BRIGHTNESS,	"acpi-brightness"),
	FWTS_TAG(FWTS_TAG_ACPI_BUTTON,		"acpi-button"),
	FWTS_TAG(FWTS_TAG_ACPI_EVENT,		"acpi-event"),
	FWTS_TAG(FWTS_TAG_ACPI_PARAMETER,	"acpi-parameter"),
	FWTS_TAG(FWTS_TAG_ACPI_THROTTLING,	"acpi-throttling"),
	FWTS_TAG(FWTS_TAG_ACPI_EXCEPTION,	"acpi-exception"),
	FWTS_TAG(FWTS_TAG_ACPI_PACKAGE,		"acpi-package"),
	FWTS_TAG(FWTS_TAG_ACPI_APIC,		"acpi-apic"),
	FWTS_TAG(FWTS_TAG_ACPI_DISPLAY,		"acpi-display"),
	FWTS_TAG(FWTS_TAG_ACPI_MULTIPLE_FACS,	"acpi-multiple-facs"),
	FWTS_TAG(FWTS_TAG_ACPI_POINTER_MISMATCH,"acpi-pointer-mismatch"),
	FWTS_TAG(FWTS_TAG_ACPI_TABLE_CHECKSUM,	"acpi-table-checksum"),
	FWTS_TAG(FWTS_TAG_ACPI_HOTPLUG,		"acpi-hotplug"),
	FWTS_TAG(FWTS_TAG_ACPI_RSDP,		"acpi-rsdp"),
	FWTS_TAG(FWTS_TAG_ACPI_MUTEX,		"acpi-mutex"),
	FWTS_TAG(FWTS_TAG_ACPI_THERMAL,		"acpi-thermal"),
	FWTS_TAG(FWTS_TAG_ACPI_LID,		"acpi-lid"),
	FWTS_TAG(FWTS_TAG_ACPI_METHOD,		"acpi-method"),
	FWTS_TAG(FWTS_TAG_EMBEDDED_CONTROLLER,	"embedded-controller"),
	FWTS_TAG(FWTS_TAG_POWER_MANAGEMENT,	"power-management"),
	FWTS_TAG(FWTS_TAG_WMI,			"wmi"),
	{ 0, NULL, NULL }
};


/*
 *  fwts_tag_id_str_to_tag()
 *	given a tag string, return the tag
 */
fwts_tag fwts_tag_id_str_to_tag(const char *tag)
{
	int i;

	for (i=0; fwts_tags[i].tag_id != NULL; i++)
		if (strcmp(tag, fwts_tags[i].tag_id) == 0)
			return fwts_tags[i].tag;
	return FWTS_TAG_NONE;
}

/*
 *  fwts_tag_to_str()
 *	given a tag, return the human readable tag name
 */
const char *fwts_tag_to_str(const fwts_tag tag)
{
	int i;
	static const char *none = "";

	for (i=0; fwts_tags[i].tag_id != NULL; i++)
		if (fwts_tags[i].tag == tag)
			return fwts_tags[i].tag_str;
	return none;
}

/*
 *  fwts_tag_compare()
 *	callback to enable tag name sorting
 */
static int fwts_tag_compare(void *data1, void *data2)
{
	return strcmp((char *)data1, (char*)data2);
}

/*
 *  fwts_tag_add()
 *	add a tag name to a list of tag names, ordered alphabetically
 * 	tag is added if it does already exist in the tag list
 */
void fwts_tag_add(fwts_list *taglist, const char *tag)
{
	fwts_list_link	*item;
	char *str;

	/* Exists already? then don't bother */
	fwts_list_foreach(item, taglist)
		if (strcmp(fwts_list_data(char *,item), tag) == 0)
			return;

	str = strdup(tag);
	if (str)
		fwts_list_add_ordered(taglist, str, fwts_tag_compare);
}

/*
 *  fwts_tag_list_to_str()
 *	given a list of tag names return a space delimited string
 *	containing the the tag names
 */
char *fwts_tag_list_to_str(fwts_list *taglist)
{
	fwts_list_link	*item;
	char *str = NULL;
	size_t len = 0;

	fwts_list_foreach(item, taglist) {
		char *tag = fwts_list_data(char *, item);
		size_t taglen = strlen(tag);
		len += taglen + 1;

		if (str) {
			if ((str = realloc(str, len)) == NULL)
				return NULL;
			strcat(str, " ");
			strcat(str, tag);
		} else {
			if ((str = malloc(len)) == NULL)
				return NULL;
			strcpy(str, tag);
		}
	}
	return str;
}

/*
 *  fwts_tag_report()
 *	report to the log the tags found in the taglist
 */
void fwts_tag_report(fwts_framework *fw, const fwts_log_field field, fwts_list *taglist)
{
	if ((taglist != NULL) && (fwts_list_len(taglist) > 0)) {
		char *tags = fwts_tag_list_to_str(taglist);
		if (tags) {
			fwts_log_printf(fw->results, field | LOG_VERBATUM, LOG_LEVEL_NONE, "", "", "", "Tags: %s", tags);
			free(tags);
		}
	}
}

/*
 *  fwts_tag_failed()
 *	add to the tag lists the tag:
 *	per test (this is emptied at end of each test)
 * 	all tests (this is total for all tests run)
 */
void fwts_tag_failed(fwts_framework *fw, const fwts_tag tag)
{
	const char *text = fwts_tag_to_str(tag);

	if (*text) {
		fwts_tag_add(&fw->test_taglist, text);
		fwts_tag_add(&fw->total_taglist, text);
	}
}
