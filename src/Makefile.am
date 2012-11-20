# Must build in this order:
#   1. acpica/source/compiler
#   2. lib
#   3. acpica
# ... because the libs in this bundled acpica/ depend on lib/ (libfwts.so),
# but libfwts.so depends on libraries produced by acpica/source/compiler.
SUBDIRS = acpica/source/compiler lib acpica
 
AM_CPPFLAGS = -I$(top_srcdir)/src/lib/include -I$(top_srcdir)/src/acpica/source/include -I$(top_srcdir)/efi_runtime -Wall -Werror -Wextra

bin_PROGRAMS = fwts
fwts_SOURCES = main.c \
	acpi/klog/klog.c \
	acpi/wakealarm/wakealarm.c \
	acpi/syntaxcheck/syntaxcheck.c \
	acpi/s3/s3.c \
	acpi/s3power/s3power.c \
	acpi/s4/s4.c \
	acpi/ac_adapter/ac_adapter.c \
	acpi/acpiinfo/acpiinfo.c \
	acpi/acpidump/acpidump.c \
	acpi/acpitables/acpitables.c \
	acpi/apicinstance/apicinstance.c \
	acpi/battery/battery.c \
	acpi/brightness/brightness.c \
	acpi/checksum/checksum.c \
	acpi/common/common.c \
	acpi/cstates/cstates.c \
	acpi/dmar/dmar.c \
	acpi/fadt/fadt.c \
	acpi/fan/fan.c \
	acpi/mcfg/mcfg.c \
	acpi/method/method.c \
	apic/apicedge/apicedge.c \
	acpi/osilinux/osilinux.c \
	acpi/lid/lid.c \
	acpi/powerbutton/powerbutton.c \
	acpi/wmi/wmi.c \
	acpi/pcc/pcc.c \
	bios/ebda_region/ebda_region.c \
	bios/ebdadump/ebdadump.c \
	bios/mtrr/mtrr.c \
	bios/bios_info/bios_info.c \
	bios/bios32/bios32.c \
	bios/hdaaudio/hdaaudio.c \
	bios/romdump/romdump.c \
	bios/os2gap/os2gap.c \
	bios/memmapdump/memmapdump.c \
	bios/multiproc/mpdump.c \
	bios/multiproc/mpcheck.c \
	bios/pciirq/pciirq.c \
	bios/pnp/pnp.c \
	bios/smbios/smbios.c \
	cmos/cmosdump/cmosdump.c \
	dmi/dmi_decode/dmi_decode.c \
	hpet/hpet_check/hpet_check.c \
	pci/aspm/aspm.c \
	pci/crs/crs.c \
	pci/maxreadreq/maxreadreq.c \
	cpu/virt/virt.c cpu/virt/virt_svm.c cpu/virt/virt_vmx.c \
	cpu/maxfreq/maxfreq.c \
	cpu/cpufreq/cpufreq.c \
	cpu/nx/nx.c \
	cpu/msr/msr.c \
	cpu/microcode/microcode.c \
	hotkey/hotkey/hotkey.c \
	kernel/version/version.c \
	kernel/oops/oops.c \
	uefi/csm/csm.c \
	uefi/uefidump/uefidump.c \
	uefi/uefirttime/uefirttime.c

fwts_LDFLAGS = -ljson -lm

fwts_LDADD = \
	-Llib/src -lfwts \
	-L$(top_srcdir)/src/acpica -lfwtsacpica

man_MANS = ../doc/fwts.1 ../doc/fwts-collect.1 ../doc/fwts-frontend-text.1
