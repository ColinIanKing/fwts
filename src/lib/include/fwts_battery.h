/*
 * Copyright (C) 2011-2015 Canonical
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
#ifndef __FWTS_BATTERY_H__
#define __FWTS_BATTERY_H__

#define FWTS_PROC_ACPI_BATTERY          "/proc/acpi/battery"

#define FWTS_BATTERY_DESIGN_CAPACITY	(0x00)
#define FWTS_BATTERY_REMAINING_CAPACITY	(0x01)
#define FWTS_BATTERY_ALL		(-1)

int fwts_battery_get_count(fwts_framework *fw, int *count);
int fwts_battery_get_cycle_count(fwts_framework *fw, const int index, int *cycle_count);
bool fwts_battery_check_trip_point_support(fwts_framework *fw, const int index);
int fwts_battery_set_trip_point(fwts_framework *fw, const int index, const int trip_point);
int fwts_battery_get_trip_point(fwts_framework *fw, const int index, int *trip_point);
int fwts_battery_get_capacity(fwts_framework *fw, const int type, const int index, uint32_t *capacity_mAh, uint32_t *capacity_mWh);
int fwts_battery_get_name(fwts_framework *fw, const int index, char *name);

#endif
