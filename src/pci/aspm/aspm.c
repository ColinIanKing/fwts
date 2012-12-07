/*
 * Copyright (C) 2011-2012 Canonical
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
#include <fcntl.h>
#include <limits.h>
#include "fwts.h"

/* PCI Confiiguration Space for Type 0 & 1 */
#define FWTS_PCI_HEADER_TYPE		0x0E
#define FWTS_PCI_CAPABILITIES_POINTER	0x34

/* PCI Confiiguration Space for Type 1 */
#define FWTS_PCI_SECONDARD_BUS_NUMBER	0x19

/* PCI Capability IDs  */
#define FWTS_PCI_LAST_ID		0x00
#define FWTS_PCI_EXPRESS_CAP_ID		0x10

/* PCI Express Capability Structure Fields */
#define FWTS_PCIE_ASPM_SUPPORT_L0_FIELD	0x0400
#define FWTS_PCIE_ASPM_SUPPORT_L1_FIELD	0x0800
#define FWTS_PCIE_ASPM_SUPPORT_FIELD	(FWTS_PCIE_ASPM_SUPPORT_L0_FIELD | FWTS_PCIE_ASPM_SUPPORT_L1_FIELD)

#define FWTS_PCIE_ASPM_CONTROL_L0_FIELD	0x0001
#define FWTS_PCIE_ASPM_CONTROL_L1_FIELD	0x0002
#define FWTS_PCIE_ASPM_CONTROL_FIELD	(FWTS_PCIE_ASPM_CONTROL_L0_FIELD | FWTS_PCIE_ASPM_CONTROL_L1_FIELD)


struct pci_device {
	uint8_t segment;
	uint8_t bus;
	uint8_t dev;
	uint8_t func;
	uint8_t config[256];
	struct pci_device *next;
};

/* PCI Express Capability Structure is defined in Section 7.8
 * of PCI Express®i Base Specification Revision 2.0
 */
struct pcie_capability {
	uint8_t pcie_cap_id;
	uint8_t next_cap_point;
	uint16_t pcie_cap_reg;
	uint32_t device_cap;
	uint16_t device_contrl;
	uint16_t device_status;
	uint32_t link_cap;
	uint16_t link_contrl;
	uint16_t link_status;
	uint32_t slot_cap;
	uint16_t slot_contrl;
	uint16_t slot_status;
	uint16_t root_contrl;
	uint16_t root_cap;
	uint32_t root_status;
	uint32_t device_cap2;
	uint16_t device_contrl2;
	uint16_t device_status2;
	uint32_t link_cap2;
	uint16_t link_contrl2;
	uint16_t link_status2;
	uint32_t slot_cap2;
	uint16_t slot_contrl2;
	uint16_t slot_status2;
};

static int facp_get_aspm_control(fwts_framework *fw, int *aspm)
{
	fwts_acpi_table_info *table;
	fwts_acpi_table_fadt *fadt;

	if (fwts_acpi_find_table(fw, "FACP", 0, &table) != FWTS_OK) {
		return FWTS_ERROR;
	}
	fadt = (fwts_acpi_table_fadt*)table->data;

	if ((fadt->iapc_boot_arch & FWTS_FACP_IAPC_BOOT_ARCH_PCIE_ASPM_CONTROLS) == 0) {
		*aspm = 1;
		fwts_log_info(fw, "PCIE ASPM is controlled by Linux kernel.");
	} else {
		*aspm = 0;
		fwts_log_info(fw, "PCIE ASPM is not controlled by Linux kernel.");
		fwts_advice(fw,
			"BIOS reports that Linux kernel should not modify ASPM "
			"settings that BIOS configured. It can be intentional "
			"because hardware vendors identified some capability "
			"bugs between the motherboard and the add-on cards.");
	}

	return FWTS_OK;
}

static int pcie_compare_rp_dev_aspm_registers(fwts_framework *fw,
	struct pci_device *rp,
	struct pci_device *dev)
{
	struct pcie_capability *rp_cap, *device_cap;
	uint8_t rp_aspm_cntrl, device_aspm_cntrl;
	uint8_t next_cap;
	int ret = FWTS_OK;
	bool l0s_disabled = false, l1_disabled = false;

	next_cap = rp->config[FWTS_PCI_CAPABILITIES_POINTER];
	rp_cap = (struct pcie_capability *) &rp->config[next_cap];
	while (rp_cap->pcie_cap_id != FWTS_PCI_EXPRESS_CAP_ID) {
		if (rp_cap->next_cap_point == FWTS_PCI_LAST_ID)
			break;
		next_cap = rp_cap->next_cap_point;
		rp_cap = (struct pcie_capability *) &rp->config[next_cap];
	}

	next_cap = dev->config[FWTS_PCI_CAPABILITIES_POINTER];
	device_cap = (struct pcie_capability *)	&dev->config[next_cap];
	while (device_cap->pcie_cap_id != FWTS_PCI_EXPRESS_CAP_ID) {
		if (device_cap->next_cap_point == FWTS_PCI_LAST_ID)
			break;
		next_cap = device_cap->next_cap_point;
		device_cap = (struct pcie_capability *)	&dev->config[next_cap];
	}


	if (((rp_cap->link_cap & FWTS_PCIE_ASPM_SUPPORT_L0_FIELD) >> 10) !=
		(rp_cap->link_contrl & FWTS_PCIE_ASPM_CONTROL_L0_FIELD)) {
		fwts_warning(fw, "RP %02Xh:%02Xh.%02Xh L0s not enabled.",
			 rp->bus, rp->dev, rp->func);
		l0s_disabled = true;
	}

	if (((rp_cap->link_cap & FWTS_PCIE_ASPM_SUPPORT_L1_FIELD) >> 10) !=
		(rp_cap->link_contrl & FWTS_PCIE_ASPM_CONTROL_L1_FIELD)) {
		fwts_warning(fw, "RP %02Xh:%02Xh.%02Xh L1 not enabled.",
			rp->bus, rp->dev, rp->func);
		l1_disabled = true;
	}

	if (((device_cap->link_cap & FWTS_PCIE_ASPM_SUPPORT_L0_FIELD) >> 10) !=
		(device_cap->link_contrl & FWTS_PCIE_ASPM_CONTROL_L0_FIELD)) {
		fwts_warning(fw, "Device %02Xh:%02Xh.%02Xh L0s not enabled.",
			dev->bus, dev->dev, dev->func);
		l0s_disabled = true;
	}

	if (((device_cap->link_cap & FWTS_PCIE_ASPM_SUPPORT_L1_FIELD) >> 10) !=
		(device_cap->link_contrl & FWTS_PCIE_ASPM_CONTROL_L1_FIELD)) {
		fwts_warning(fw, "Device %02Xh:%02Xh.%02Xh L1 not enabled.",
			dev->bus, dev->dev, dev->func);
		l1_disabled = true;
	}

	if (l0s_disabled) {
		fwts_advice(fw,
			"The ASPM L0s low power Link state is optimized for "
			"short entry and exit latencies, while providing "
			"substantial power savings. Disabling L0s of a PCIe "
			"device may increases power consumption, and will  "
			"impact the battery life of a mobile system.");
	}

	if (l1_disabled) {
		fwts_advice(fw,
			"The ASPM L1 low power Link state is optimized for "
			"maximum power savings with longer entry and exit "
			"latencies. Disabling L1 of a PCIe device may "
			"increases power consumption, and will impact the "
			"battery life of a mobile system significantly.");
	}

	rp_aspm_cntrl = rp_cap->link_contrl & FWTS_PCIE_ASPM_CONTROL_FIELD;
	device_aspm_cntrl = device_cap->link_contrl & FWTS_PCIE_ASPM_CONTROL_FIELD;
	if (rp_aspm_cntrl != device_aspm_cntrl) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "PCIEASPM_UNMATCHED",
			"PCIE aspm setting was not matched.");
		fwts_log_error(fw, "RP %02Xh:%02Xh.%02Xh has aspm = %02Xh."
			, rp->bus, rp->dev, rp->func, rp_aspm_cntrl);
		fwts_log_error(fw, "Device %02Xh:%02Xh.%02Xh has aspm = %02Xh.",
			dev->bus, dev->dev, dev->func, device_aspm_cntrl);
		fwts_advice(fw,
			"ASPM control registers between root port and device "
			"must match in order for ASPM to be active. Unmatched "
			"configuration indicates software did not configure "
			"ASPM correctly and the system is not saving power "
			"at its full potential.");
	} else {
		fwts_passed(fw, "PCIE aspm setting matched was matched.");
	}

	return ret;
}

static void pci_device_list_free(struct pci_device *head)
{
	struct pci_device *next;

	while (head) {
		next = head->next;
		free(head);
		head = next;
	}
}

static int pcie_check_aspm_registers(fwts_framework *fw)
{
	fwts_list *lspci_output;
	fwts_list_link *item;
	struct pci_device *head = NULL, *cur = NULL, *device = NULL;
	char command[PATH_MAX];
	int ret = FWTS_OK;
	int status;

	snprintf(command, sizeof(command), "%s", fw->lspci);

	if (fwts_pipe_exec(command, &lspci_output, &status) != FWTS_OK) {
		fwts_log_warning(fw, "Could not execute %s", command);
		return FWTS_ERROR;
	}
	if (lspci_output == NULL)
		return FWTS_ERROR;

	/* Get the list of pci devices and their configuration */
	fwts_list_foreach(item, lspci_output) {
		char *line = fwts_text_list_text(item);
		char *pEnd;

		device = (struct pci_device *) calloc(1, sizeof(*device));
		if (device == NULL) {
			pci_device_list_free(head);
			fwts_text_list_free(lspci_output);
			return FWTS_ERROR;
		}

		device->bus = strtol(line, &pEnd, 16);
		device->dev = strtol(pEnd + 1, &pEnd, 16);
		device->func = strtol(pEnd + 1, &pEnd, 16);

		if (head == NULL)
			head = device;
		else
			cur->next = device;

		cur = device;
	}
	fwts_text_list_free(lspci_output);

	for (cur = head; cur; cur = cur->next) {
		int reg_loc = 0, reg_val = 0;
		int i;
		int status;

		snprintf(command, sizeof(command), "%s -s %02X:%02X.%02X -xxx",
			fw->lspci, cur->bus, cur->dev, cur->func);
		if (fwts_pipe_exec(command, &lspci_output, &status) != FWTS_OK) {
			fwts_log_warning(fw, "Could not execute %s", command);
			pci_device_list_free(head);
			return FWTS_ERROR;
		}
		if (lspci_output == NULL) {
			pci_device_list_free(head);
			return FWTS_ERROR;
		}

		fwts_list_foreach(item, lspci_output) {
			char *line = fwts_text_list_text(item);
			char *pEnd;

			if (strlen(line) >= 3 && line[3] == ' ') {
				reg_val = strtol(line, &pEnd, 16);
				for (i = 0; reg_loc < 256 && i < 16; i++) {
					reg_val = strtol(pEnd + 1, &pEnd, 16);
					cur->config[reg_loc] = reg_val;
					reg_loc++;
				}
			}
		}
		fwts_text_list_free(lspci_output);
	}

	/* Check aspm registers from the list of pci devices */
	for (cur = head; cur; cur = cur->next) {
		struct pci_device *target;

		/* Find PCI Bridge (PCIE Root Port) and the attached device  */
		if (cur->config[FWTS_PCI_HEADER_TYPE] & 0x01) {
			target = head;
			while (target != NULL) {
				if (target->bus == cur->config[FWTS_PCI_SECONDARD_BUS_NUMBER])
					break;
				target = target->next;
			}
			if (target == NULL) {
				continue;
			}

			pcie_compare_rp_dev_aspm_registers(fw, cur, target);
		}
	}

	pci_device_list_free(head);

	return ret;
}

static int aspm_check_configuration(fwts_framework *fw)
{
	int ret;
	int aspm_facp;

	ret = facp_get_aspm_control(fw, &aspm_facp);
	if (ret == FWTS_ERROR) {
		fwts_log_info(fw, "No valid FACP information present: cannot test aspm.");
		return FWTS_ERROR;
	}

	return ret;
}

static int aspm_pcie_register_configuration(fwts_framework *fw)
{
	return pcie_check_aspm_registers(fw);
}

static fwts_framework_minor_test aspm_tests[] = {
	{ aspm_check_configuration, "PCIe ASPM ACPI test." },
	{ aspm_pcie_register_configuration, "PCIe ASPM registers test." },
	{ NULL, NULL }
};

static fwts_framework_ops aspm_ops = {
	.description = "PCIe ASPM check.",
	.minor_tests = aspm_tests
};

FWTS_REGISTER(aspm, &aspm_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ROOT_PRIV);
