/*
 * Copyright (C) 2011 Canonical
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

static fwts_mp_data mp_data;

static uint8_t last_cpu_apic_id = 0;

static bool mpcheck_find_bus(uint8_t id, int depth)
{
	fwts_list_link	*entry;

	if (depth > 16)
		return false;		/* too deep? */

	fwts_list_foreach(entry, &mp_data.entries) {	
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		if (*data == FWTS_MP_BUS_ENTRY) {
			fwts_mp_bus_entry *bus_entry =
				fwts_list_data(fwts_mp_bus_entry *, entry);
			if (id == bus_entry->bus_id)
				return true;
		}
		if (*data == FWTS_MP_BUS_HIERARCHY_ENTRY) {
			fwts_mp_bus_hierarchy_entry *bus_hierarchy_entry = 
				fwts_list_data(fwts_mp_bus_hierarchy_entry *, entry);
			if (id == bus_hierarchy_entry->bus_id)
				return mpcheck_find_bus(bus_hierarchy_entry->parent_bus, depth+1);
		}
	}
	return false;
}


static int mpcheck_test_cpu_entries(fwts_framework *fw)
{
	bool failed = false;
	int n = 0;
	fwts_list_link	*entry;
	int bootstrap_cpu = -1;
	bool usable_cpu_found = false;
	int first_io_apic_id = -1;

	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		if (*data == FWTS_MP_CPU_ENTRY) {
			uint32_t phys_addr = mp_data.phys_addr + ((void *)data - (void *)mp_data.header);
			fwts_mp_processor_entry *cpu_entry = 
				fwts_list_data(fwts_mp_processor_entry *, entry);

			if (last_cpu_apic_id < cpu_entry->local_apic_id)
				last_cpu_apic_id = cpu_entry->local_apic_id;
		
			if (first_io_apic_id == -1) {
				first_io_apic_id = cpu_entry->local_apic_id;
				if (first_io_apic_id != 0) {
					fwts_failed_high(fw, "CPU Entry %d (@0x%8.8x) has a Local APIC ID 0x%2.2x and should be 0x00.", n, phys_addr, first_io_apic_id);
					failed = true;
				}
			} else {
				if (cpu_entry->local_apic_id != (first_io_apic_id + n)) {
					fwts_failed_high(fw, "CPU Entry %d (@0x%8.8x) has a Local APIC ID 0x%2.2x and should be 0x%2.2x.", n, phys_addr, cpu_entry->local_apic_id, first_io_apic_id + n);
					failed = true;
				}
			}
			/*
			if ((cpu_entry->local_apic_version != 0x11) &&
			    (cpu_entry->local_apic_version != 0x14)) {
				fwts_failed_high(fw, "CPU Entry %d (@0x%8.8x) has an invalid Local APIC Version %2.2x, should be 0x11 or 0x14.", n, phys_addr, cpu_entry->local_apic_version);
				failed = true;
			}
			*/
			if (cpu_entry->cpu_flags & 1)
				usable_cpu_found = true;

			if ((cpu_entry->cpu_flags >> 1) & 1) {
				if (bootstrap_cpu != -1) {
					fwts_failed_high(fw, "CPU Entry %d (@0x%8.8x) is marked as a boot CPU but CPU entry %d is the first boot CPU.", n, phys_addr, bootstrap_cpu);
					failed = true;
				} else 
					bootstrap_cpu = n;
			}
			n++;
		}
	}

	if (!usable_cpu_found) {
		fwts_failed_high(fw, "CPU entries 0..%d were not marked as usable. There should be at least one usable CPU.",
			n-1);
		failed = true;
	}

	if (!failed)
		fwts_passed(fw, "All %d CPU entries look sane.", n);

	return FWTS_OK;
}

static char *bus_types[] = {
	"CBUS",
	"CBUSII",
	"EISA",
	"FUTURE",
	"INTERN",
	"ISA",
	"MBI",
	"MBII",
	"MCA",
	"MPI",
	"MPSA",
	"NUBUS",
	"PCI",
	"PCMCIA",
	"TC",
	"VL",
	"VME",
	"XPRESS",
	NULL,
};

static int mpcheck_test_bus_entries(fwts_framework *fw)
{
	bool failed = false;
	int n = 0;
	fwts_list_link	*entry;
	int prev_bus_id = -1;

	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		if (*data == FWTS_MP_BUS_ENTRY) {
			int i;
			uint32_t phys_addr = mp_data.phys_addr + ((void *)data - (void *)mp_data.header);
			fwts_mp_bus_entry *bus_entry =
				fwts_list_data(fwts_mp_bus_entry *, entry);

			for (i=0; bus_types[i] != NULL; i++) {
				if (strncmp(bus_types[i], (char*)bus_entry->bus_type, strlen(bus_types[i])) == 0) 
					break;
			}
			if (bus_types[i] == NULL) {
				fwts_failed_high(fw, "Bus Entry %d (@0x%8.8x) has an unrecognised bus type: %6.6s",
					n, phys_addr, bus_entry->bus_type);
			}
			if (prev_bus_id == -1) {
				prev_bus_id = bus_entry->bus_id;
				if (prev_bus_id != 0) {
					fwts_failed_high(fw, "Bus Entry %d (@0x%8.8x) has a Local APIC ID 0x%2.2x and should be 0x00.", n, phys_addr, prev_bus_id);
					failed = true;
				}
			} else {
				if (bus_entry->bus_id < prev_bus_id) {
					fwts_failed_high(fw, "Bus Entry %d (@0x%8.8x) has a Bus ID 0x%2.2x and should be greater than 0x%2.2x.", n, phys_addr, bus_entry->bus_id, prev_bus_id);
					failed = true;
				}
			}
			n++;
		}
	}
	if (!failed)
		fwts_passed(fw, "All %d Bus Entries looked sane.", n);

	return FWTS_OK;
}

static int mpcheck_test_io_apic_entries(fwts_framework *fw)
{
	bool failed = false;
	bool enabled = false;
	int n = 0;
	fwts_list_link	*entry;
	int first_io_apic_id = -1;

	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		uint32_t phys_addr = mp_data.phys_addr + ((void *)data - (void *)mp_data.header);
		if (*data == FWTS_MP_IO_APIC_ENTRY) {
			fwts_mp_io_apic_entry *io_apic_entry = fwts_list_data(fwts_mp_io_apic_entry *, entry);

			if (io_apic_entry->address == 0) {
				fwts_failed_high(fw, "IO APIC Entry %d (@0x%8.8x) has an invalid NULL address, should be non-zero.", n, phys_addr);
				failed = true;
			}
			if (io_apic_entry->flags & 1) {
				enabled = true;
			}
			if (first_io_apic_id == -1) {
				first_io_apic_id = io_apic_entry->id;
				if (first_io_apic_id != (last_cpu_apic_id + 1)) {
					fwts_failed_high(fw, "IO APIC Entry %d (@0x%8.8x) has a Local APIC ID 0x%2.2x and should be 0x%2.2x.", n, phys_addr, io_apic_entry->id, last_cpu_apic_id + 1);
					failed = true;
				}
			} else {
				if (io_apic_entry->id != (first_io_apic_id + n)) {
					fwts_failed_high(fw, "IO APIC Entry %d (@0x%8.8x) has a Local APIC ID 0x%2.2x and should be 0x%2.2x than the previous entry.", n, phys_addr, io_apic_entry->id, first_io_apic_id + n);
					failed = true;
				}
			}
			n++;
		}
	}

	if (!enabled) {
		fwts_failed_high(fw, "None of the %d IO APIC entries were enabled, at least one must be enabled.", n);
		failed = true;
	}

	if (!failed)
		fwts_passed(fw, "All %d IO APIC Entries look sane.", n);

	return FWTS_OK;
}

static bool mpcheck_find_io_apic(uint8_t id)
{
	fwts_list_link	*entry;

	if (id == 0xff)
		return true;	/* match all */

	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
#if 0
		if (*data == FWTS_MP_CPU_ENTRY) {
			fwts_mp_processor_entry *cpu_entry = 
				fwts_list_data(fwts_mp_processor_entry *, entry);
			if (id == cpu_entry->local_apic_id)
				return true;
		}
#endif
		if (*data == FWTS_MP_IO_APIC_ENTRY) {
			fwts_mp_io_apic_entry *io_apic_entry =
				fwts_list_data(fwts_mp_io_apic_entry *, entry);
			if (id == io_apic_entry->id)
				return true;
		}
	}
	return false;
}

static int mpcheck_test_io_interrupt_entries(fwts_framework *fw)
{
	bool failed = false;
	int n = 0;
	fwts_list_link	*entry;

	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		uint32_t phys_addr = mp_data.phys_addr + ((void *)data - (void *)mp_data.header);
		if (*data == FWTS_MP_IO_INTERRUPT_ENTRY) {
			fwts_mp_io_interrupt_entry *io_interrupt_entry =
				fwts_list_data(fwts_mp_io_interrupt_entry *, entry);
				
			if (io_interrupt_entry->type > 3) {
				fwts_failed_high(fw, "IO Interrupt Entry %d (@0x%8.8x) has a Type 0x%2.2x and should be 0x00..0x03.", n, phys_addr, io_interrupt_entry->type);
				failed = true;
			}
			if (!mpcheck_find_io_apic(io_interrupt_entry->destination_io_apic_id)) {
				fwts_failed_high(fw, "IO Interrupt Entry %d (@0x%8.8x) has a Destination IO APIC ID 0x%2.2x which has not been defined.", n, phys_addr, io_interrupt_entry->destination_io_apic_id);
				failed = true;
			}
			n++;
		}
	}
		
	if (!failed)
		fwts_passed(fw, "All %d IO Interrupt Entries look sane.", n);

	return FWTS_OK;
}

static int mpcheck_test_local_interrupt_entries(fwts_framework *fw)
{
	bool failed = false;
	int n = 0;
	fwts_list_link	*entry;

	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		uint32_t phys_addr = mp_data.phys_addr + ((void *)data - (void *)mp_data.header);
		if (*data == FWTS_MP_LOCAL_INTERRUPT_ENTRY) {
			fwts_mp_local_interrupt_entry *local_interrupt_entry =
				fwts_list_data(fwts_mp_local_interrupt_entry *, entry);
				
			if (local_interrupt_entry->type > 3) {
				fwts_failed_high(fw, "Local Interrupt Entry %d (@0x%8.8x) has a Type 0x%2.2x and should be 0x00..0x03.", n, phys_addr, local_interrupt_entry->type);
				failed = true;
			}
#if 0
			if (!mpcheck_find_io_apic(local_interrupt_entry->destination_local_apic_id)) {
				fwts_failed_high(fw, "Local Interrupt Entry %d (@0x%8.8x) has a Destination IO APIC ID 0x%2.2x which has not been defined.", n, phys_addr, local_interrupt_entry->destination_local_apic_id);
				failed = true;
			}
#endif
			n++;
		}
	}
		
	if (!failed)
		fwts_passed(fw, "All %d Local Interrupt Entries look sane.", n);

	return FWTS_OK;
}

static int mpcheck_test_sys_addr_entries(fwts_framework *fw)
{
	bool failed = false;
	int n = 0;
	fwts_list_link	*entry;

	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		if (*data == FWTS_MP_SYS_ADDR_ENTRY) {
			uint32_t phys_addr = mp_data.phys_addr + ((void *)data - (void *)mp_data.header);
			fwts_mp_system_address_space_entry *sys_addr_entry = 
				fwts_list_data(fwts_mp_system_address_space_entry *, entry);

			if (!mpcheck_find_bus(sys_addr_entry->bus_id, 0)) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM, 
					"System Address Space Mapping Entry %d (@0x%8.8x) has an Bus ID 0x%2.2x that is not defined in any of the Bus Entries.", n, phys_addr, sys_addr_entry->bus_id);
				failed = true;
			}
			if (sys_addr_entry->address_type > 3) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"System Address Space Mapping Entry %d (@0x%8.8x) has an incorrect Address Type: %d, should be 0..3.", n, phys_addr, sys_addr_entry->address_type);
				failed = true;
			}
			if (sys_addr_entry->address_length == 0) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"System Address Space Mapping Entry %d (@0x%8.8x) has an zero sized Address Length.", n, phys_addr);
				failed = true;
			}
			n++;
		}	
	}
	
	if (!failed)
		fwts_passed(fw, "All %d System Address Space Mapping Entries looks sane.", n);

	return FWTS_OK;
}

static int mpcheck_test_bus_hierarchy_entries(fwts_framework *fw)
{
	bool failed = false;
	int n = 0;
	fwts_list_link	*entry;

	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		if (*data == FWTS_MP_BUS_HIERARCHY_ENTRY) {
			uint32_t phys_addr = mp_data.phys_addr + ((void *)data - (void *)mp_data.header);
			fwts_mp_bus_hierarchy_entry *bus_hierarchy_entry = 
				fwts_list_data(fwts_mp_bus_hierarchy_entry *, entry);
			if (bus_hierarchy_entry->length != 8) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Bus Hierarchy Entry %d (@x%8.8x) length was 0x%2.2x, it should be 0x08.",
					n, phys_addr, bus_hierarchy_entry->length);
				failed = true;
			}
			if (!mpcheck_find_bus(bus_hierarchy_entry->parent_bus, 0)) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Bus Hierarchy Entry %d (@x%8.8x) did not have parents that connected to a top level Bus entry.",
					n, phys_addr);
				failed = true;
			}
			n++;
		}
	}

	if (!failed)
		fwts_passed(fw, "All %d Bus Hierarchy Entries look sane.", n);

	return FWTS_OK;
}

static int mpcheck_test_compat_bus_address_space_entries(fwts_framework *fw)
{
	bool failed = false;
	int n = 0;
	fwts_list_link	*entry;

	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		if (*data == FWTS_MP_COMPAT_BUS_ADDRESS_SPACE_ENTRY) {
			uint32_t phys_addr = mp_data.phys_addr + ((void *)data - (void *)mp_data.header);
			fwts_mp_compat_bus_address_space_entry *compat_bus_entry =
				fwts_list_data(fwts_mp_compat_bus_address_space_entry*, entry);

			if (compat_bus_entry->length != 8) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Compatible Bus Address Space Entry %d (@x%8.8x) length was 0x%2.2x, it should be 0x08.",
					n, phys_addr, compat_bus_entry->length);
				failed = true;
			}
			if (compat_bus_entry->range_list > 1) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Compatible Bus Address Space Entry %d (@x%8.8x) Range List was 0x%2.2x, it should be 0x00 or 0x01.",
					n, phys_addr, compat_bus_entry->range_list);
				failed = true;
			}
			n++;
		}
	}

	if (!failed)
		fwts_passed(fw, "All %d Compatible Bus Address Space Entries look sane.", n);

	return FWTS_OK;
}


static int mpcheck_init(fwts_framework *fw)
{	
	if (fwts_mp_data_get(&mp_data) != FWTS_OK) {
		fwts_log_error(fw, "Failed to get _MP_ data from firmware.");
		return FWTS_SKIP;
	}
	return FWTS_OK;
}

static int mpcheck_deinit(fwts_framework *fw)
{
	return fwts_mp_data_free(&mp_data);
}

uint8_t mpcheck_get_apic_id(void *data)
{
	uint8_t *which = (uint8_t*)data;
	
	if (*which == FWTS_MP_CPU_ENTRY) {
		fwts_mp_processor_entry *cpu_entry = (fwts_mp_processor_entry *)data;
		return cpu_entry->local_apic_id;
	}
	if (*which == FWTS_MP_IO_APIC_ENTRY) {
		fwts_mp_io_apic_entry *io_apic_entry = (fwts_mp_io_apic_entry *)data;
		return io_apic_entry->id;
	}
	return 0xff;
}

static int mpcheck_test_header(fwts_framework *fw)
{
	bool failed = false;

	if (strncmp((char*)mp_data.header->signature, FWTS_MP_HEADER_SIGNATURE, 4)) {
		fwts_failed_medium(fw, "MP header signature should be %s, got %4.4s.",
				FWTS_MP_HEADER_SIGNATURE, mp_data.header->signature);
		failed = true;
	}

	if ((mp_data.header->spec_rev != 1) && (mp_data.header->spec_rev != 4)) {
		fwts_failed_medium(fw, "MP header spec revision should be 1 or 4, got %d.", mp_data.header->spec_rev);
		failed = true;
	}
	if (mp_data.header->lapic_address == 0) {
		fwts_failed_medium(fw, "MP header LAPIC address is NULL.");
		failed = true;
	}
	if (!failed)
		fwts_passed(fw, "MP header looks sane.");

	return FWTS_OK;
}


static fwts_framework_minor_test mpcheck_tests[] = {
	{ mpcheck_test_header,  		"Check MP header." },
	{ mpcheck_test_cpu_entries,		"Check MP CPU entries." },
	{ mpcheck_test_bus_entries,		"Check MP Bus entries." },
	{ mpcheck_test_io_apic_entries, 	"Check MP IO APIC entries." },
	{ mpcheck_test_io_interrupt_entries,	"Check MP IO Interrupt entries." },
	{ mpcheck_test_local_interrupt_entries, "Check MP Local Interrupt entries." },
	{ mpcheck_test_sys_addr_entries,	"Check MP System Address entries." },
	{ mpcheck_test_bus_hierarchy_entries,	"Check MP Bus Hierarchy entries." },
	{ mpcheck_test_compat_bus_address_space_entries, "Check MP Compatible Bus Address Space entries." },
	{ NULL, NULL }
};

static fwts_framework_ops mpcheck_ops = {
	.description = "Check MultiProcessor Tables.",
	.init        = mpcheck_init,
	.deinit      = mpcheck_deinit,
	.minor_tests = mpcheck_tests,
};

FWTS_REGISTER(mpcheck, &mpcheck_ops, FWTS_TEST_ANYTIME, FWTS_BATCH | FWTS_ROOT_PRIV);

#endif
