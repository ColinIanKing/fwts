#!/bin/bash
#
# Copyright (C) 2010-2012 Canonical
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

CONFIGS="-DCONFIG_ACPI_HOTPLUG_CPU -DCONFIG_ACPI_PROC_EVENT \
	 -DCONFIG_ACPI_PROCFS_POWER -DCONFIG_ACPI_SLEEP \
	 -DCONFIG_CPU_FREQ -DCONFIG_DMI -DCONFIG_HIBERNATION \
	 -DCONFIG_HOTPLUG_CPU -DCONFIG_KEXEC -DCONFIG_NET \
	 -DCONFIG_PM -DCONFIG_PM_SLEEP -DCONFIG_SMP \
	 -DCONFIG_SUSPEND -DCONFIG_X86 -DCONFIG_X86_IO_APIC"

KERNELSCAN=./kernelscan
TMP=/tmp/kernelscan_$$.txt

if [ $# -lt 1 ]; then
	echo "Usage: $0 path-to-kernel-source"
	exit 1
fi

src=$1

if [ ! -d  $1 ]; then
	echo "Path '$1' not found"
	exit 1
fi

scan_source_file()
{
	$KERNELSCAN < $1 -E | gcc -E $CONFIGS - | $KERNELSCAN -P > $TMP
	if [ $(stat -c%s $TMP) -gt 0 ]; then
		echo "Source: $1"
		cat $TMP
	fi
	rm $TMP
}

scan_source_tree()
{
	tree=$1

	echo "Scanning $tree"
	for I in $(find $tree -name "*.c")
	do
		scan_source_file $I
	done
}

scan_source_tree $src/drivers/acpi
scan_source_tree $src/arch/x86/kernel/acpi
