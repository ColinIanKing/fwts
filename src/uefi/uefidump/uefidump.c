/*
 * Copyright (C) 2010-2013 Canonical
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
#include <stddef.h>
#include <inttypes.h>

#include "fwts.h"
#include "fwts_uefi.h"


typedef void (*uefidump_func)(fwts_framework *fw, fwts_uefi_var *var);

typedef struct {
	char *description;		/* UEFI var */
	uefidump_func	func;		/* Function to dump this variable */
} uefidump_info;

static void uefidump_var_hexdump(fwts_framework *fw, fwts_uefi_var *var)
{
	int i;
	uint8_t *data = (uint8_t*)var->data;

	fwts_log_info_verbatum(fw,  "  Size: %zd bytes of data.", var->datalen);

	for (i = 0; i < (int)var->datalen; i+= 16) {
		char buffer[128];
		int left = (int)var->datalen - i;

		fwts_dump_raw_data(buffer, sizeof(buffer), data + i, i, left > 16 ? 16 : left);
		fwts_log_info_verbatum(fw,  "  Data: %s", buffer+2);
	}
}

static char *uefidump_vprintf(char *str, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/*
 *  uefidump_vprintf()
 *	printf() to str: if str NULL - allocate buffer and print into it,
 *	otherwise allocate more space and append new text to end of existing
 *	string.  Return new string, or NULL if failed.
 */
static char *uefidump_vprintf(char *str, const char *fmt, ...)
{
	va_list args;
	char buffer[4096];

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (str == NULL)
		str = strdup(buffer);
	else {
		str = realloc(str, strlen(str) + strlen(buffer) + 1);
		if (str == NULL)
			return NULL;
		strcat(str, buffer);
	}

	return str;
}

/*
 *  uefidump_build_dev_path()
 *	recursively scan dev_path and build up a human readable path name
 */
static char *uefidump_build_dev_path(char *path, fwts_uefi_dev_path *dev_path)
{
	switch (dev_path->type & 0x7f) {
	case FWTS_UEFI_END_DEV_PATH_TYPE:
		switch (dev_path->subtype) {
		case FWTS_UEFI_END_ENTIRE_DEV_PATH_SUBTYPE:
		case FWTS_UEFI_END_THIS_DEV_PATH_SUBTYPE:
			break;
		default:
			return uefidump_vprintf(path, "\\Unknown-End(0x%" PRIx8 ")", dev_path->subtype);
		}
		break;
	case FWTS_UEFI_HARDWARE_DEV_PATH_TYPE:
		switch (dev_path->subtype) {
		case FWTS_UEFI_PCI_DEV_PATH_SUBTYPE:
			{
				fwts_uefi_pci_dev_path *p = (fwts_uefi_pci_dev_path *)dev_path;
				path = uefidump_vprintf(path, "\\PCI(0x%" PRIx8 ",0x%" PRIx8 ")",
					p->function, p->device);
			}
			break;
		case FWTS_UEFI_PCCARD_DEV_PATH_SUBTYPE:
			{
				fwts_uefi_pccard_dev_path *p = (fwts_uefi_pccard_dev_path *)dev_path;
				path = uefidump_vprintf(path, "\\PCCARD(0x%" PRIx8 ")",
					p->function);
			}
			break;
		case FWTS_UEFI_MEMORY_MAPPED_DEV_PATH_SUBTYPE:
			{
				fwts_uefi_mem_mapped_dev_path *m = (fwts_uefi_mem_mapped_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\Memmap(0x%" PRIx32 ",0x%" PRIx64 ",0x%" PRIx64 ")",
					m->memory_type,
					m->start_addr,
					m->end_addr);
			}
			break;
		case FWTS_UEFI_VENDOR_DEV_PATH_SUBTYPE:
			{
				fwts_uefi_vendor_dev_path *v = (fwts_uefi_vendor_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\VENDOR(%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x)",
					v->guid.info1, v->guid.info2, v->guid.info3,
					v->guid.info4[0], v->guid.info4[1], v->guid.info4[2], v->guid.info4[3],
					v->guid.info4[4], v->guid.info4[5], v->guid.info4[6], v->guid.info4[7]);
			}
			break;
		case FWTS_UEFI_CONTROLLER_DEV_PATH_SUBTYPE:
			{
				fwts_uefi_controller_dev_path *c = (fwts_uefi_controller_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\Controller(0x%" PRIx32 ")",
					c->controller);
			}
			break;
		default:
			path = uefidump_vprintf(path, "\\Unknown-HW-DEV-PATH(0x%" PRIx8 ")", dev_path->subtype);
			break;
		}
		break;

	case FWTS_UEFI_ACPI_DEVICE_PATH_TYPE:
		switch (dev_path->subtype) {
		case FWTS_UEFI_ACPI_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_acpi_dev_path *a = (fwts_uefi_acpi_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\ACPI(0x%" PRIx32 ",0x%" PRIx32 ")",
					a->hid, a->uid);
			}
			break;
		case FWTS_UEFI_EXPANDED_ACPI_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_expanded_acpi_dev_path *a = (fwts_uefi_expanded_acpi_dev_path*)dev_path;
				char *hidstr= a->hidstr;
				path = uefidump_vprintf(path, "\\ACPI(");
				if (hidstr[0] == '\0')
					path = uefidump_vprintf(path, "0x%" PRIx32 ",", a->hid);
				else
					path = uefidump_vprintf(path, "%s,", hidstr);
				hidstr += strlen(hidstr) + 1;
				if (hidstr[0] == '\0')
					path = uefidump_vprintf(path, "0x%" PRIx32 ",", a->uid);
				else
					path = uefidump_vprintf(path, "%s,", hidstr);
				hidstr += strlen(hidstr) + 1;
				if (hidstr[0] == '\0')
					path = uefidump_vprintf(path, "0x%" PRIx32 ",", a->cid);
				else
					path = uefidump_vprintf(path, "%s,", hidstr);
			}
			break;
		default:
			path = uefidump_vprintf(path, "\\Unknown-ACPI-DEV-PATH(0x%" PRIx8 ")", dev_path->subtype);
			break;
		}
		break;

	case FWTS_UEFI_MESSAGING_DEVICE_PATH_TYPE:
		switch (dev_path->subtype) {
			case FWTS_UEFI_ATAPI_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_atapi_dev_path *a = (fwts_uefi_atapi_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\ATAPI(0x%" PRIx8 ",0x%" PRIx8 ",0x%" PRIx16 ")",
					a->primary_secondary, a->slave_master, a->lun);
			}
			break;
		case FWTS_UEFI_SCSI_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_scsi_dev_path *s = (fwts_uefi_scsi_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\SCSI(0x%" PRIx16 ",0x%" PRIx16 ")",
					s->pun, s->lun);
			}
			break;
		case FWTS_UEFI_FIBRE_CHANNEL_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_fibre_channel_dev_path *f = (fwts_uefi_fibre_channel_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\FIBRECHANNEL(0x%" PRIx64 ",0x%" PRIx64 ")",
					f->wwn, f->lun);
			}
			break;
		case FWTS_UEFI_1394_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_1394_dev_path *fw = (fwts_uefi_1394_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\1394(0x%" PRIx64 ")",
					fw->guid);
			}
			break;
		case FWTS_UEFI_USB_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_usb_dev_path *u = (fwts_uefi_usb_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\USB(0x%" PRIx8 ",0x%" PRIx8 ")",
					u->parent_port_number, u->interface);
			}
			break;
		case FWTS_UEFI_USB_CLASS_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_usb_class_dev_path *u = (fwts_uefi_usb_class_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\USBCLASS(0x%" PRIx16 ",0x%" PRIx16
					",0x%" PRIx8 ",0x%" PRIx8 ",0x%" PRIx8 ")",
					u->vendor_id, u->product_id,
					u->device_class, u->device_subclass,
					u->device_protocol);
			}
			break;
		case FWTS_UEFI_I2O_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_i2o_dev_path *i2o = (fwts_uefi_i2o_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\I2O(0x%" PRIx32 ")", i2o->tid);
			}
			break;
		case FWTS_UEFI_MAC_ADDRESS_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_mac_addr_dev_path *m = (fwts_uefi_mac_addr_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\MACADDR(%" PRIx8 ":%" PRIx8 ":%" PRIx8
					":%" PRIx8 ":%" PRIx8 ":%" PRIx8 ",0x%" PRIx8 ")",
					m->mac_addr[0], m->mac_addr[1],
					m->mac_addr[2], m->mac_addr[3],
					m->mac_addr[4], m->mac_addr[5],
					m->if_type);
			}
			break;
		case FWTS_UEFI_IPV4_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_ipv4_dev_path *i = (fwts_uefi_ipv4_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\IPv4("
					"%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 ","
					"%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 ","
					"%" PRIu16 ",%" PRIu16 ",%" PRIx16 ",%" PRIx8 ")",
					i->local_ip_addr[0], i->local_ip_addr[1],
					i->local_ip_addr[2], i->local_ip_addr[3],
					i->remote_ip_addr[0], i->remote_ip_addr[1],
					i->remote_ip_addr[2], i->remote_ip_addr[3],
					i->local_port, i->remote_port,
					i->protocol, i->static_ip_address);
			}
			break;
		case FWTS_UEFI_IPV6_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_ipv6_dev_path *i = (fwts_uefi_ipv6_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\IPv6("
					"%" PRIx8 ":%" PRIx8 ":%" PRIx8 ":%" PRIx8
					":%" PRIx8 ":%" PRIx8 ":%" PRIx8 ":%" PRIx8 ","
					"%" PRIx8 ":%" PRIx8 ":%" PRIx8 ":%" PRIx8
					":%" PRIx8 ":%" PRIx8 ":%" PRIx8 ":%" PRIx8 ","
					"%" PRIu16 ",%" PRIu16 ",%" PRIx16 ",%" PRIx8 ")",
					i->local_ip_addr[0], i->local_ip_addr[1],
					i->local_ip_addr[2], i->local_ip_addr[3],
					i->local_ip_addr[4], i->local_ip_addr[5],
					i->local_ip_addr[6], i->local_ip_addr[7],
					i->remote_ip_addr[0], i->remote_ip_addr[1],
					i->remote_ip_addr[2], i->remote_ip_addr[3],
					i->remote_ip_addr[4], i->remote_ip_addr[5],
					i->remote_ip_addr[6], i->remote_ip_addr[7],
					i->local_port, i->remote_port,
					i->protocol, i->static_ip_address);
			}
			break;
		case FWTS_UEFI_INFINIBAND_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_infiniband_dev_path *i = (fwts_uefi_infiniband_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\InfiniBand("
					"%" PRIx8 ",%" PRIx64 ",%" PRIx64 ",%" PRIx64 ")",
					i->port_gid[0], i->remote_id,
					i->target_port_id, i->device_id);
			}
			break;
		case FWTS_UEFI_UART_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_uart_dev_path *u = (fwts_uefi_uart_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\UART("
					"%" PRIu64 ",%" PRIu8 ",%" PRIu8 ",%" PRIu8 ")",
					u->baud_rate, u->data_bits, u->parity, u->stop_bits);
			}
			break;
		case FWTS_UEFI_VENDOR_MESSAGING_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_vendor_messaging_dev_path *v = (fwts_uefi_vendor_messaging_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\VENDOR("
					"%08" PRIx32 "-%04" PRIx16 "-%04" PRIx16 "-"
					"%02" PRIx8 "-%02" PRIx8 "-"
					"%02" PRIx8 "-%02" PRIx8 "-%02" PRIx8 "-%02" PRIx8 "-%02" PRIx8 "-%02" PRIx8 ")",
					v->guid.info1, v->guid.info2, v->guid.info3,
					v->guid.info4[0], v->guid.info4[1], v->guid.info4[2], v->guid.info4[3],
					v->guid.info4[4], v->guid.info4[5], v->guid.info4[6], v->guid.info4[7]);
			}
			break;
		default:
			path = uefidump_vprintf(path, "\\Unknown-MESSAGING-DEV-PATH(0x%" PRIx8 ")", dev_path->subtype);
			break;
		}
		break;

	case FWTS_UEFI_MEDIA_DEVICE_PATH_TYPE:
		switch (dev_path->subtype) {
		case FWTS_UEFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_hard_drive_dev_path *h = (fwts_uefi_hard_drive_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\HARDDRIVE("
				"%" PRIu32 ",%" PRIx64 ",%" PRIx64 ","
				"%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 ","
				"%" PRIx8 ",%" PRIx8 ")",
				h->partition_number,
				h->partition_start,
				h->partition_size,
				h->partition_signature[0], h->partition_signature[1],
				h->partition_signature[2], h->partition_signature[3],
				h->partition_signature[4], h->partition_signature[5],
				h->partition_signature[6], h->partition_signature[7],
				h->mbr_type, h->signature_type);
			}
			break;
		case FWTS_UEFI_CDROM_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_cdrom_dev_path *c = (fwts_uefi_cdrom_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\CDROM(%" PRIu32 ",%" PRIx64 ",%" PRIx64 ")",
					c->boot_entry, c->partition_start, c->partition_size);
			}
			break;
		case FWTS_UEFI_VENDOR_MEDIA_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_vendor_media_dev_path *v = (fwts_uefi_vendor_media_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\VENDOR("
					"%08" PRIx32 "-%04" PRIx16 "-%04" PRIx16 "-"
					"%02" PRIx8 "-%02" PRIx8 "-"
					"%02" PRIx8 "-%02" PRIx8 "-%02" PRIx8 "-%02" PRIx8 "-%02" PRIx8 "-%02" PRIx8 ")",
					v->guid.info1, v->guid.info2, v->guid.info3,
					v->guid.info4[0], v->guid.info4[1], v->guid.info4[2], v->guid.info4[3],
					v->guid.info4[4], v->guid.info4[5], v->guid.info4[6], v->guid.info4[7]);
			}
			break;
		case FWTS_UEFI_FILE_PATH_DEVICE_PATH_SUBTYPE:
			{
				char tmp[4096];
				fwts_uefi_file_path_dev_path *f = (fwts_uefi_file_path_dev_path*)dev_path;
				fwts_uefi_str16_to_str(tmp, sizeof(tmp), f->path_name);
				path = uefidump_vprintf(path, "\\FILE('%s')", tmp);
			}
			break;
		case FWTS_UEFI_PROTOCOL_DEVICE_PATH_SUBTYPE:
		default:
			path = uefidump_vprintf(path, "\\Unknown-MEDIA-DEV-PATH(0x%" PRIx8 ")", dev_path->subtype);
			break;
		}
		break;

	case FWTS_UEFI_BIOS_DEVICE_PATH_TYPE:
		switch (dev_path->subtype) {
		case FWTS_UEFI_BIOS_DEVICE_PATH_SUBTYPE:
			{
				fwts_uefi_bios_dev_path *b = (fwts_uefi_bios_dev_path*)dev_path;
				path = uefidump_vprintf(path, "\\BIOS(%" PRIx16 ",%" PRIx16 ",%s)",
					b->device_type, b->status_flags, b->description);
			}
			break;
		default:
			path = uefidump_vprintf(path, "\\Unknown-BIOS-DEV-PATH(0x%" PRIx8 ")", dev_path->subtype);
			break;
		}
		break;
	default:
		path = uefidump_vprintf(path, "\\Unknown-TYPE(0x%" PRIx8 ")", dev_path->type);
		break;
	}

	/* Not end? - collect more */
	if (!((dev_path->type & 0x7f) == (FWTS_UEFI_END_DEV_PATH_TYPE) &&
	      (dev_path->subtype == FWTS_UEFI_END_ENTIRE_DEV_PATH_SUBTYPE))) {
		uint16_t len = dev_path->length[0] | (((uint16_t)dev_path->length[1])<<8);
		if (len > 0) {
			dev_path = (fwts_uefi_dev_path*)((char *)dev_path + len);
			path = uefidump_build_dev_path(path, dev_path);
		}
	}

	return path;
}

static void uefidump_info_dev_path(fwts_framework *fw, fwts_uefi_var *var)
{
	char *path;

	path = uefidump_build_dev_path(NULL, (fwts_uefi_dev_path*)var->data);

	fwts_log_info_verbatum(fw, "  Device Path: %s.", path);

	free(path);
}

static void uefidump_info_lang(fwts_framework *fw, fwts_uefi_var *var)
{
	uint8_t *data = (uint8_t*)var->data;
	fwts_log_info_verbatum(fw, "  Language: %c%c%c%c.", data[0], data[1], data[2], data[3]);
}

static void uefidump_info_langcodes(fwts_framework *fw, fwts_uefi_var *var)
{
	char buffer[2048];
	char *dst = buffer;
	char *data = (char*)var->data;

	for (;;) {
		*dst++ = *data++;
		*dst++ = *data++;
		*dst++ = *data++;
		if (*data < ' ')
			break;
		*dst++ = ',';
	}
	*dst = '\0';

	fwts_log_info_verbatum(fw, "  Language Codes: %s.", buffer);
}

static void uefidump_info_platform_lang(fwts_framework *fw, fwts_uefi_var *var)
{
	uint8_t *data = (uint8_t*)var->data;
	fwts_log_info_verbatum(fw, "  Platform Language: %c%c%c%c%c%c.", data[0], data[1], data[2], data[3], data[4], data[5]);
}

static void uefidump_info_platform_langcodes(fwts_framework *fw, fwts_uefi_var *var)
{
	char buffer[2048];

	char *dst = buffer;
	char *data = (char*)var->data;

	for (;;) {
		if (*data < ' ')
			break;
		if (*data == ';')
			*dst++ = ',';
		else
			*dst++ = *data;
		data++;
	}
	*dst = '\0';

	fwts_log_info_verbatum(fw, "  Platform Language Codes: %s.", buffer);
}

static void uefidump_info_timeout(fwts_framework *fw, fwts_uefi_var *var)
{
	uint16_t *data = (uint16_t*)var->data;
	fwts_log_info_verbatum(fw, "Timeout: %" PRId16 " seconds.", *data);
}

static void uefidump_info_bootcurrent(fwts_framework *fw, fwts_uefi_var *var)
{
	uint16_t *data = (uint16_t *)var->data;

	fwts_log_info_verbatum(fw, "  BootCurrent: 0x%4.4" PRIx16 ".", *data);
}

static void uefidump_info_bootnext(fwts_framework *fw, fwts_uefi_var *var)
{
	uint16_t *data = (uint16_t *)var->data;

	fwts_log_info_verbatum(fw, "  BootNext: 0x%4.4" PRIx16 ".", *data);
}

static void uefidump_info_bootoptionsupport(fwts_framework *fw, fwts_uefi_var *var)
{
	uint16_t *data = (uint16_t *)var->data;

	fwts_log_info_verbatum(fw, "  BootOptionSupport: 0x%4.4" PRIx16 ".", *data);
}

static void uefidump_info_bootorder(fwts_framework *fw, fwts_uefi_var *var)
{
	uint16_t *data = (uint16_t*)var->data;
	int i;
	int n = (int)var->datalen / sizeof(uint16_t);
	char *str = NULL;

	for (i = 0; i<n; i++) {
		str = uefidump_vprintf(str, "0x%04" PRIx16 "%s",
			*data++, i < (n - 1) ? "," : "");
	}
	fwts_log_info_verbatum(fw, "  Boot Order: %s.", str);
	free(str);
}

static void uefidump_info_bootdev(fwts_framework *fw, fwts_uefi_var *var)
{
	fwts_uefi_load_option * load_option = (fwts_uefi_load_option *)var->data;
	char tmp[2048];
	char *path;
	int len;

	fwts_log_info_verbatum(fw, "  Active: %s\n",
		(load_option->attributes & FWTS_UEFI_LOAD_ACTIVE) ? "Yes" : "No");
	fwts_uefi_str16_to_str(tmp, sizeof(tmp), load_option->description);
	len = fwts_uefi_str16len(load_option->description);
	fwts_log_info_verbatum(fw, "  Info: %s\n", tmp);

	/* Skip over description to get to packed path, unpack path and print */
	path = (char *)var->data + sizeof(load_option->attributes) +
		sizeof(load_option->file_path_list_length) +
		(sizeof(uint16_t) * (len + 1));
	path = uefidump_build_dev_path(NULL, (fwts_uefi_dev_path *)path);
	fwts_log_info_verbatum(fw, "  Path: %s.", path);
	free(path);
}

/*
 *  Dump kernel oops log messages
 */
static void uefidump_info_dump_type0(fwts_framework *fw, fwts_uefi_var *var)
{
	char *ptr = (char*)var->data;

	while (*ptr) {
		char *start = ptr;
		while (*ptr && *ptr != '\n')
			ptr++;

		if (*ptr == '\n') {
			*ptr++ = 0;
			fwts_log_info_verbatum(fw, "  KLog: %s.", start);
		}
	}
}

static void uefidump_info_secure_boot(fwts_framework *fw, fwts_uefi_var *var)
{
	if (var->datalen != 1) {
		/* Should be 1 byte, of not, dump it out as a hex dump */
		uefidump_var_hexdump(fw, var);
	} else {
		char *mode;
		uint8_t value = (uint8_t)var->data[0];

		switch (value) {
		case 0:
			mode = " (Secure Boot Mode Off)";
			break;
		case 1:
			mode = " (Secure Boot Mode On)";
			break;
		default:
			mode = "";
			break;
		}
		fwts_log_info_verbatum(fw, "  Value: 0x%2.2x%s.", value, mode);
	}
}

static void uefidump_info_setup_mode(fwts_framework *fw, fwts_uefi_var *var)
{
	if (var->datalen != 1) {
		/* Should be 1 byte, of not, dump it out as a hex dump */
		uefidump_var_hexdump(fw, var);
	} else {
		char *mode;
		uint8_t value = (uint8_t)var->data[0];

		switch (value) {
		case 0:
			mode = " (User Mode)";
			break;
		case 1:
			mode = " (Setup Mode)";
			break;
		default:
			mode = "";
			break;
		}
		fwts_log_info_verbatum(fw, "  Value: 0x%2.2" PRIx8 "%s.", value, mode);
	}
}

/*
 *  See TCG Platform Reset Attack Mitigation Specification, Revision 1.0,
 *  section 5.
 */
static void uefidump_info_morc(fwts_framework *fw, fwts_uefi_var *var)
{
	if (var->datalen != 1) {
		/* Should be 1 byte, of not, dump it out as a hex dump */
		uefidump_var_hexdump(fw, var);
	} else {
		char *mode;
		uint8_t value = (uint8_t)var->data[0];

		switch (value & 1) {
		case 0:
			mode = " (Firmware should not clear memory on reboot)";
			break;
		case 1:
			mode = " (Firmware should clear memory on reboot)";
			break;
		default:
			mode = "";
			break;
		}
		fwts_log_info_verbatum(fw, "  Value: 0x%2.2" PRIx8 "%s.", value, mode);
	}
}

/*
 *  Dump ACPI global variable address
 */
static void uefidump_info_acpi_global_variable(fwts_framework *fw, fwts_uefi_var *var)
{
	if (var->datalen != 8) {
		/* Should be 8 bytes, of not, dump it out as a hex dump */
		uefidump_var_hexdump(fw, var);
	} else {
		uint64_t value;

		memcpy(&value, var->data, sizeof(uint64_t));
		fwts_log_info_verbatum(fw, "  ACPI Global Variable Address: 0x%16.16" PRIx64 ".", value);
	}
}

/*
 *  Dump Supported Signature GUIDs
 */
static void uefidump_info_signature_support(fwts_framework *fw, fwts_uefi_var *var)
{
	if (var->datalen % 16) {
		/* Should be multiple of 16 bytes, of not, dump it out as a hex dump */
		uefidump_var_hexdump(fw, var);
	} else {
		/* Signatures are an array of GUIDs */
		uint8_t *data = var->data;
		char guid_str[37];

		if (var->datalen)
			fwts_log_info_verbatum(fw, "  Signature GUIDs:");

		while (data - var->data < (ptrdiff_t)var->datalen) {
			fwts_guid_buf_to_str(data, guid_str, sizeof(guid_str));
			fwts_log_info_verbatum(fw, "    %s", guid_str);
			data += 16;
		}
	}
}

static void uefidump_info_hwerrrec_support(fwts_framework *fw, fwts_uefi_var *var)
{
	if (var->datalen != 2) {
		/* Should be 2 byte, of not, dump it out as a hex dump */
		uefidump_var_hexdump(fw, var);
	} else {
		char *support;
		uint16_t *value = (uint16_t *)var->data;

		switch (*value) {
		case 0:
			support = " (Not support for Hardware Error Record Persistence)";
			break;
		case 1:
			support = " (Support for Hardware Error Record Persistence)";
			break;
		default:
			support = " (reserved value)";
			break;
		}
		fwts_log_info_verbatum(fw, "  Value: 0x%4.4" PRIx16 "%s.", *value, support);
	}
}

static uefidump_info uefidump_info_table[] = {
	{ "PlatformLangCodes",	uefidump_info_platform_langcodes },
	{ "PlatformLang",	uefidump_info_platform_lang },
	{ "BootOptionSupport", 	uefidump_info_bootoptionsupport },
	{ "BootCurrent", 	uefidump_info_bootcurrent },
	{ "BootOrder",		uefidump_info_bootorder },
	{ "BootNext", 		uefidump_info_bootnext },
	{ "ConInDev",		uefidump_info_dev_path },
	{ "ConIn",		uefidump_info_dev_path },
	{ "ConOutDev",		uefidump_info_dev_path },
	{ "ConOut",		uefidump_info_dev_path },
	{ "ConErrDev",		uefidump_info_dev_path },
	{ "ErrOutDev",		uefidump_info_dev_path },
	{ "ErrOut",		uefidump_info_dev_path },
	{ "LangCodes",		uefidump_info_langcodes },
	{ "Lang",		uefidump_info_lang },
	{ "Timeout",		uefidump_info_timeout },
	{ "Boot0",		uefidump_info_bootdev },
	{ "dump-type0-",	uefidump_info_dump_type0 },
	{ "SecureBoot",		uefidump_info_secure_boot },
	{ "SetupMode",		uefidump_info_setup_mode },
	{ "MemoryOverwriteRequestControl",	uefidump_info_morc },
	{ "AcpiGlobalVariable",	uefidump_info_acpi_global_variable },
	{ "SignatureSupport",	uefidump_info_signature_support },
	{ "HwErrRecSupport",	uefidump_info_hwerrrec_support },
	{ NULL, NULL }
};

static void uefidump_var(fwts_framework *fw, fwts_uefi_var *var)
{
	char varname[512];
	char guid_str[37];
	uefidump_info *info;

	fwts_uefi_get_varname(varname, sizeof(varname), var);

	fwts_log_info_verbatum(fw, "Name: %s.", varname);
	fwts_guid_buf_to_str(var->guid, guid_str, sizeof(guid_str));
	fwts_log_info_verbatum(fw, "  GUID: %s", guid_str);
	fwts_log_info_verbatum(fw, "  Attr: 0x%x (%s).", var->attributes, fwts_uefi_attribute_info(var->attributes));

	/* If we've got an appropriate per variable dump mechanism, use this */
	for (info = uefidump_info_table; info->description != NULL; info++) {
		if (strncmp(varname, info->description, strlen(info->description)) == 0) {
			info->func(fw, var);
			return;
		}
	}

	/* otherwise just do a plain old hex dump */
	uefidump_var_hexdump(fw, var);
}

static int uefidump_init(fwts_framework *fw)
{
	if (fwts_firmware_detect() != FWTS_FIRMWARE_UEFI) {
		fwts_log_info(fw, "Cannot detect any UEFI firmware. Aborted.");
		return FWTS_ABORTED;
	}

	return FWTS_OK;
}

static int uefidump_test1(fwts_framework *fw)
{
	fwts_list name_list;

	if (fwts_uefi_get_variable_names(&name_list) == FWTS_ERROR) {
		fwts_log_info(fw, "Cannot find any UEFI variables.");
	} else {
		fwts_list_link *item;

		fwts_list_foreach(item, &name_list) {
			fwts_uefi_var var;
			char *name = fwts_list_data(char *, item);

			if (fwts_uefi_get_variable(name, &var) == FWTS_OK) {
				uefidump_var(fw, &var);
				fwts_uefi_free_variable(&var);
				fwts_log_nl(fw);
			}
		}
	}

	fwts_uefi_free_variable_names(&name_list);

	return FWTS_OK;
}

static fwts_framework_minor_test uefidump_tests[] = {
	{ uefidump_test1, "Dump UEFI Variables." },
	{ NULL, NULL }
};

static fwts_framework_ops uefidump_ops = {
	.description = "Dump UEFI variables.",
	.init        = uefidump_init,
	.minor_tests = uefidump_tests
};

FWTS_REGISTER("uefidump", &uefidump_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_UTILS | FWTS_FLAG_ROOT_PRIV);
