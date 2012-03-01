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
#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

typedef enum {
	FWTS_WMI_EXPENSIVE 	= 0x00000001,
	FWTS_WMI_METHOD		= 0x00000002,
	FWTS_WMI_STRING		= 0x00000004,
	FWTS_WMI_EVENT		= 0x00000008
} fwts_wmi_flags;

typedef struct {
	char *guid;
	char *driver;
	char *vendor;
} fwts_wmi_known_guid;

/*
 *  Packed WMI WDG data
 */
typedef struct {
	uint8_t	guid[16];			/* GUID */
	union {
		uint8_t 	obj_id[2];	/* Object Identifier */
		struct {
			uint8_t	notify_id;	/* Notify Identifier */
			uint8_t	reserved;	/* Reserved? */
		};
	};
	uint8_t	instance;			/* Instance */
	uint8_t	flags;				/* fwts_wmi_flags */
} __attribute__ ((packed)) fwts_guid_info;

static fwts_wmi_known_guid fwts_wmi_known_guids[] = {
	{ "67C3371D-95A3-4C37-BB61-DD47B491DAAB", "acer-wmi",	"Acer" },
	{ "431F16ED-0C2B-444C-B267-27DEB140CF9C", "acer-wmi",	"Acer" },
	{ "6AF4F258-B401-42FD-BE91-3D4AC2D7C0D3", "acer-wmi",	"Acer" },
	{ "95764E09-FB56-4e83-B31A-37761F60994A", "acer-wmi",	"Acer" },
	{ "61EF69EA-865C-4BC3-A502-A0DEBA0CB531", "acer-wmi",	"Acer" },
	{ "676AA15E-6A47-4D9F-A2CC-1E6D18D14026", "acer-wmi",	"Acer" },
	{ "9DBB5994-A997-11DA-B012-B622A1EF5492", "dell-wmi",	"Dell" },
	{ "284A0E6B-380E-472A-921F-E52786257FB4", "dell-wmi-aio","Dell" },
	{ "02314822-307C-4F66-BF0E-48AEAEB26CC8", "dell-wmi-aio","Dell" },
	{ "ABBC0F72-8EA1-11D1-00A0-C90629100000", "eeepc-wmi",	"Asus" },
	{ "97845ED0-4E6D-11DE-8A39-0800200C9A66", "eeepc-wmi",	"Asus" },
	{ "95F24279-4D7B-4334-9387-ACCDC67EF61C", "hp-wmi",	"HP"   },
	{ "5FB7F034-2C63-45e9-BE91-3D44E2C707E4", "hp-wmi",	"HP"   },
	{ "551A1F84-FBDD-4125-91DB-3EA8F44F1D45", "msi-wmi",	"MSI"  },
	{ "B6F3EEF2-3D2F-49DC-9DE3-85BCE18C62F2", "msi-wmi",	"MSI"  },
	{ "C364AC71-36DB-495A-8494-B439D472A505", "tc110-wmi",	"HP Compaq" },
	{ NULL, NULL, NULL }
};

static fwts_wmi_known_guid *wmi_find_guid(char *guid)
{
	fwts_wmi_known_guid *info = fwts_wmi_known_guids;

	for (info = fwts_wmi_known_guids; info->guid != NULL; info++)
		if (strcmp(info->guid, guid) == 0)
			return info;

	return NULL;
}

#define CONSUME_WHITESPACE(str)		\
	while (*str && isspace(*str))	\
		str++;			\
	if (*str == '\0') return;	\

static char *wmi_wdg_flags_to_text(const fwts_wmi_flags flags)
{
	static char buffer[256];

	*buffer = 0;

	if (flags & FWTS_WMI_EXPENSIVE)
		strcat(buffer, "WMI_EXPENSIVE ");
	if (flags & FWTS_WMI_METHOD)
		strcat(buffer, "WMI_METHOD");
	if (flags & FWTS_WMI_STRING)
		strcat(buffer, "WMI_STRING");
	if (flags & FWTS_WMI_EVENT)
		strcat(buffer, "WMI_EVENT ");

	return buffer;
}

static void wmi_parse_wdg_data(fwts_framework *fw,
	const int size, const uint8_t *wdg_data, bool *result)
{
	int i;
	int advice_given = 0;

	fwts_guid_info *info = (fwts_guid_info *)wdg_data;

	for (i=0; i<(size / sizeof(fwts_guid_info)); i++) {
		uint8_t *guid = info->guid;
		char guidstr[37];
		fwts_wmi_known_guid *known;

		fwts_guid_buf_to_str(guid, guidstr, sizeof(guidstr));

		known = wmi_find_guid(guidstr);

		if (info->flags & FWTS_WMI_METHOD) {
			fwts_log_info(fw,
				"Found WMI Method WM%c%c with GUID: %s, "
				"Instance 0x%2.2x",
				info->obj_id[0], info->obj_id[1],
				guidstr, info->instance);
		} else if (info->flags & FWTS_WMI_EVENT) {
			fwts_log_info(fw,
				"Found WMI Event, Notifier ID: 0x%2.2x, "
				"GUID: %s, Instance 0x%2.2x",
				info->notify_id, guidstr, info->instance);
			if (known == NULL) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"WMIUnknownGUID",
					"GUID %s is unknown to the kernel, "
					"a driver may need to be implemented "
					"for this GUID.", guidstr);
				*result = true;
				if (!advice_given) {
					advice_given = 1;
					fwts_log_nl(fw);
					fwts_log_advice(fw,
						"ADVICE: A WMI driver probably "
						"needs to be written for this "
						"event.");
					fwts_log_advice(fw,
						"It can checked for using: "
						"wmi_has_guid(\"%s\").",
						guidstr);
					fwts_log_advice(fw,
						"One can install a notify "
						"handler using "
						"wmi_install_notify_handler"
						"(\"%s\", handler, NULL).  ",
						guidstr);
					fwts_log_advice(fw,
						"http://lwn.net/Articles/391230"
						" describes how to write an "
						"appropriate driver.");
					fwts_log_nl(fw);
				}
			}
		} else {
			char *flags = wmi_wdg_flags_to_text(info->flags);
			fwts_log_info(fw,
				"Found WMI Object, Object ID %c%c, "
				"GUID: %s, Instance 0x%2.2x, Flags: %2.2x %s",
				info->obj_id[0], info->obj_id[1], guidstr,
				info->instance, info->flags, flags);
		}

		if (known) {
			fwts_passed(fw,
				"GUID %s is handled by driver %s (Vendor: %s).",
				guidstr, known->driver, known->vendor);
			*result = true;
		}

		info++;
	}
}

static void wmi_get_wdg_data(fwts_framework *fw,
	fwts_list_link *item, const int size, uint8_t *wdg_data)
{
	char *str;
	uint8_t *data = wdg_data;

	for (;item != NULL; item=item->next) {
		int i;
		str = fwts_text_list_text(item);
		CONSUME_WHITESPACE(str);

		if (*str == '}') break;

		if (strncmp(str, "/*", 2) == 0) {
			str = strstr(str, "*/");
			if (str)
				str += 2;
			else
				continue;
		}
		CONSUME_WHITESPACE(str);

		for (i=0;i<8;i++) {
			if (strncmp(str, "0x", 2))
				break;
			*data = strtoul(str, NULL, 16);
			str+=4;
			if (*str != ',') break;
			str++;
			if (!isspace(*str)) break;
			str++;
			data++;
			if (data > wdg_data + size) {
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"WMI_WDGBufferBad",
					"_WDG buffer was more than %d bytes "
					"long!", size);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_BAD_LENGTH);
				return;
			}
		}
	}
	return;
}

static void wmi_parse_for_wdg(fwts_framework *fw,
	fwts_list_link *item, int *count, bool *result)
{
	uint8_t *wdg_data;
	int size;
	char *str = fwts_text_list_text(item);

	/* Parse Name(_WDG, Buffer, (0xXX) */

	CONSUME_WHITESPACE(str);

	if (strncmp(str, "Name", 4))
		return;
	str += 4;

	CONSUME_WHITESPACE(str);

	if (*str != '(') return;
	str++;

	CONSUME_WHITESPACE(str);

	if (strncmp(str, "_WDG",4))
		return;
	str+=4;

	CONSUME_WHITESPACE(str);

	if (*str != ',') return;
	str++;

	CONSUME_WHITESPACE(str);

	if (strncmp(str, "Buffer",6))
		return;
	str+=6;

	CONSUME_WHITESPACE(str);

	if (*str != '(') return;
	str++;

	CONSUME_WHITESPACE(str);

	size = strtoul(str, NULL, 16);

	item = item->next;
	if (item == NULL) return;

	str = fwts_text_list_text(item);
	CONSUME_WHITESPACE(str);
	if (*str != '{') return;

	item = item->next;
	if (item == NULL) return;

	if ((wdg_data = calloc(1, size)) != NULL) {
		(*count)++ ;
		wmi_get_wdg_data(fw, item, size, wdg_data);
		wmi_parse_wdg_data(fw, size, wdg_data, result);
		free(wdg_data);
	}
}

static int wmi_table(fwts_framework *fw,
	const char *table, const int which, const char *name, bool *result)
{
	fwts_list_link *item;
	fwts_list* iasl_output;
	int count = 0;
	int ret;

	ret = fwts_iasl_disassemble(fw, table, which, &iasl_output);
	if (ret == FWTS_NO_TABLE)	/* Nothing more to do */
		return ret;
	if (ret != FWTS_OK) {
		fwts_aborted(fw, "Cannot disassemble and parse for "
			"WMI information.");
		return FWTS_ERROR;
	}

	fwts_list_foreach(item, iasl_output)
		wmi_parse_for_wdg(fw, item, &count, result);

	if (count == 0)
		fwts_log_info(fw, "No WMI data found in table  %s.", name);

	fwts_text_list_free(iasl_output);

	return FWTS_OK;
}

static int wmi_DSDT(fwts_framework *fw)
{
	bool result = false;
	int ret;

	ret = wmi_table(fw, "DSDT", 0, "DSDT", &result);

	if (!result)
		fwts_infoonly(fw);

	return ret;
}

static int wmi_SSDT(fwts_framework *fw)
{
	int i;
	bool result = false;
	int ret = FWTS_OK;

	for (i=0; i < 16; i++) {
		char buffer[10];
		snprintf(buffer, sizeof(buffer), "SSDT%d", i+1);

		ret = wmi_table(fw, "SSDT", i, buffer, &result);
		if (ret == FWTS_NO_TABLE) {
			ret = FWTS_OK; /* Hit the last table */
			break;
		}
		if (ret != FWTS_OK) {
			ret = FWTS_ERROR;
			break;
		}
	}
	if (!result)
		fwts_infoonly(fw);

	return ret;
}

static fwts_framework_minor_test wmi_tests[] = {
	{ wmi_DSDT, "Check Windows Management Instrumentation in DSDT" },
	{ wmi_SSDT, "Check Windows Management Instrumentation in SSDT" },
	{ NULL, NULL }
};

static fwts_framework_ops wmi_ops = {
	.description = "Extract and analyse Windows Management "
		       "Instrumentation (WMI).",
	.minor_tests = wmi_tests
};

FWTS_REGISTER(wmi, &wmi_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
