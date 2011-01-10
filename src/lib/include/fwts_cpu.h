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

#ifndef __FWTS_CPU_H__
#define __FWTS_CPU_H__

typedef struct cpuinfo_x86 {
	char *vendor_id;	/* Vendor ID */
	int x86;		/* CPU family */
	int x86_model;		/* Model */
	char *model_name;	/* Model name */
	int stepping;		/* Stepping */
	char *flags;		/* String containing flags */
} fwts_cpuinfo_x86;

int fwts_cpu_readmsr(const int cpu, const uint32_t reg, uint64_t *val);

int fwts_cpu_has_c1e(void);
fwts_cpuinfo_x86 *fwts_cpu_get_info(const int which_cpu);
void fwts_cpu_free_info(fwts_cpuinfo_x86 *cpu);

int fwts_cpu_enumerate(void);
int fwts_cpu_consume(const int seconds);
int fwts_cpu_consume_start(void);
void fwts_cpu_consume_complete(void);

#endif
