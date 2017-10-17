# Must build in this order:
#   1. acpica/source/compiler
#   2. lib
#   3. acpica
# ... because the libs in this bundled acpica/ depend on lib/ (libfwts.so),
# but libfwts.so depends on libraries produced by acpica/source/compiler.
SUBDIRS = acpica/source/compiler lib acpica

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/lib/include \
	-I$(top_srcdir)/src/acpica/source/include \
	-I$(top_srcdir)/src/acpica/source/compiler \
	-I$(top_srcdir)/efi_runtime \
	`pkg-config --silence-errors --cflags json` \
	`pkg-config --silence-errors --cflags json-c` \
	-pthread `pkg-config --cflags glib-2.0 gio-2.0` \
	-Wall -Werror -Wextra

bin_PROGRAMS = fwts

fwts_CPPFLAGS = $(AM_CPPFLAGS) -DACPI_DEBUG_OUTPUT

if HAVE_LIBFDT
dt_tests = \
	devicetree/dt_base/dt_base.c		\
	devicetree/dt_sysinfo/dt_sysinfo.c
mem_tests = \
	opal/mem_info.c				\
	opal/reserv_mem.c
cpu_tests = \
	opal/cpu_info.c
power_mgmt_tests = \
	opal/power_mgmt_info.c
endif

if HAVE_LIBFDT
if HAVE_LIBPCI
pci_tests = \
	opal/pci_info.c
endif
endif

#
#  fwts main + tests
#
fwts_SOURCES = main.c 				\
	acpi/ac_adapter/ac_adapter.c 		\
	acpi/devices/ac_adapter/ac.c		\
	acpi/devices/battery/battery.c		\
	acpi/devices/battery/smart_battery.c	\
	acpi/devices/ec/ec.c			\
	acpi/devices/buttons/power_button.c	\
	acpi/devices/buttons/sleep_button.c	\
	acpi/devices/lid/lid.c			\
	acpi/devices/sensor/als.c		\
	acpi/acpidump/acpidump.c 		\
	acpi/acpiinfo/acpiinfo.c 		\
	acpi/acpitables/acpitables.c 		\
	sbbr/acpitables/acpitables.c 		\
	acpi/apicinstance/apicinstance.c 	\
	acpi/asf/asf.c				\
	acpi/aspt/aspt.c			\
	acpi/battery/battery.c 			\
	acpi/bert/bert.c			\
	acpi/bgrt/bgrt.c			\
	acpi/boot/boot.c			\
	acpi/brightness/brightness-helper.c	\
	acpi/brightness/brightness.c 		\
	acpi/brightness/autobrightness.c 	\
	acpi/checksum/checksum.c 		\
	acpi/cpep/cpep.c			\
	acpi/crsdump/crsdump.c			\
	acpi/crsdump/prsdump.c			\
	acpi/csrt/csrt.c			\
	acpi/cstates/cstates.c 			\
	acpi/dbgp/dbgp.c 			\
	acpi/dbg2/dbg2.c 			\
	acpi/dmar/dmar.c 			\
	acpi/dppt/dppt.c 			\
	acpi/drtm/drtm.c 			\
	acpi/ecdt/ecdt.c			\
	acpi/einj/einj.c			\
	acpi/erst/erst.c			\
	acpi/facs/facs.c 			\
	acpi/fadt/fadt.c 			\
	sbbr/fadt/fadt.c 			\
	acpi/fan/fan.c 				\
	acpi/fpdt/fpdt.c 			\
	acpi/gpedump/gpedump.c			\
	acpi/gtdt/gtdt.c			\
	acpi/hest/hest.c			\
	acpi/hpet/hpet.c 			\
	acpi/hmat/hmat.c 			\
	acpi/iort/iort.c			\
	acpi/lid/lid.c 				\
	acpi/lpit/lpit.c 			\
	acpi/madt/madt.c			\
	acpi/mcfg/mcfg.c 			\
	acpi/mchi/mchi.c 			\
	acpi/mpst/mpst.c 			\
	acpi/msct/msct.c 			\
	acpi/msdm/msdm.c 			\
	acpi/method/method.c 			\
	acpi/nfit/nfit.c 			\
	acpi/osilinux/osilinux.c 		\
	acpi/pcc/pcc.c 				\
	acpi/pcct/pcct.c			\
	acpi/pdtt/pdtt.c			\
	acpi/powerbutton/powerbutton.c 		\
	acpi/plddump/plddump.c			\
	acpi/pmtt/pmtt.c 			\
	acpi/pptt/pptt.c 			\
	acpi/rasf/rasf.c			\
	acpi/rsdp/rsdp.c			\
	sbbr/rsdp/rsdp.c			\
	acpi/rsdt/rsdt.c			\
	acpi/s3/s3.c 				\
	acpi/s3power/s3power.c 			\
	acpi/s4/s4.c 				\
	acpi/sbst/sbst.c			\
	acpi/sdei/sdei.c			\
	acpi/sdev/sdev.c			\
	acpi/slic/slic.c 			\
	acpi/slit/slit.c 			\
	acpi/spcr/spcr.c 			\
	acpi/spmi/spmi.c 			\
	acpi/srat/srat.c 			\
	acpi/stao/stao.c			\
	acpi/syntaxcheck/syntaxcheck.c 		\
	acpi/tcpa/tcpa.c 			\
	acpi/tpm2/tpm2.c 			\
	acpi/uefi/uefi.c			\
	acpi/waet/waet.c			\
	acpi/wakealarm/wakealarm.c 		\
	acpi/wdat/wdat.c			\
	acpi/wmi/wmi.c 				\
	acpi/wpbt/wpbt.c			\
	acpi/wsmt/wsmt.c			\
	acpi/xsdt/xsdt.c			\
	acpi/xenv/xenv.c			\
	apic/apicedge/apicedge.c 		\
	bios/bios_info/bios_info.c 		\
	bios/bios32/bios32.c 			\
	bios/ebda_region/ebda_region.c 		\
	bios/ebdadump/ebdadump.c 		\
	bios/hdaaudio/hdaaudio.c 		\
	bios/memmapdump/memmapdump.c 		\
	bios/mtrr/mtrr.c 			\
	bios/multiproc/mpcheck.c 		\
	bios/multiproc/mpdump.c 		\
	bios/pciirq/pciirq.c 			\
	bios/pnp/pnp.c 				\
	bios/romdump/romdump.c 			\
	cmos/cmosdump/cmosdump.c 		\
	cpu/virt/virt.c 			\
	cpu/virt/virt_svm.c 			\
	cpu/virt/virt_vmx.c			\
	cpu/maxfreq/maxfreq.c 			\
	cpu/cpufreq/cpufreq.c 			\
	cpu/nx/nx.c 				\
	cpu/msr/msr.c 				\
	cpu/microcode/microcode.c 		\
	dmi/dmicheck/dmicheck.c 		\
	hotkey/hotkey/hotkey.c 			\
	ipmi/bmc/bmc_info.c			\
	kernel/klog/klog.c 			\
	kernel/olog/olog.c			\
	kernel/oops/oops.c 			\
	kernel/version/version.c 		\
	opal/mtd_info.c				\
	opal/prd_info.c				\
	pci/aspm/aspm.c 			\
	pci/crs/crs.c 				\
	pci/maxreadreq/maxreadreq.c 		\
	uefi/csm/csm.c 				\
	uefi/uefidump/uefidump.c 		\
	uefi/uefirttime/uefirttime.c		\
	uefi/uefirtvariable/uefirtvariable.c	\
	uefi/uefirtmisc/uefirtmisc.c		\
	uefi/securebootcert/securebootcert.c	\
	uefi/uefivarinfo/uefivarinfo.c		\
	uefi/uefibootpath/uefibootpath.c	\
	uefi/uefirtauthvar/uefirtauthvar.c	\
	uefi/esrtdump/esrtdump.c		\
	uefi/esrt/esrt.c			\
	$(pci_tests)				\
	$(mem_tests)				\
	$(cpu_tests)				\
	$(power_mgmt_tests)			\
	$(dt_tests)

fwts_LDFLAGS = -lm `pkg-config --libs glib-2.0 gio-2.0`

fwts_LDADD = \
	-L$(top_builddir)/src/acpica -lfwtsacpica \
	-Llib/src -lfwts

man_MANS = ../doc/fwts.1 ../doc/fwts-collect.1 ../doc/fwts-frontend-text.1
