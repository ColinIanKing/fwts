#
# Copyright (C) 2010-2021 Canonical, Ltd.
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

# Nb. the top-level Makefile builds source/compiler for us now...

#
# -DACPI_EXEC_APP is specific to making ACPICA for the fwts method test
#
AM_CPPFLAGS = 						\
	-D_LINUX -DNDEBUG -D_CONSOLE -DACPI_EXEC_APP	\
	-I$(top_srcdir)/src/lib/include			\
	-I../acpica/source/include			\
	-I../acpica/source/tools/acpiexec		\
	`pkg-config --silence-errors --cflags json`	\
	`pkg-config --silence-errors --cflags json-c`	\
	-Wall -fno-strict-aliasing			\
	-Wno-address-of-packed-member

#
#  We rename some of the functions so we can override them in fwts. This
#  is a hack, I feel dirty now.
#
osunixxf_munged.c: ../acpica/source/os_specific/service_layers/osunixxf.c
	cat  $^ |							\
	sed 's/^AcpiOsPrintf/__AcpiOsPrintf/' |				\
	sed 's/^AcpiOsReadPort/__AcpiOsReadPort/' |			\
	sed 's/^AcpiOsReadPciConfiguration/__AcpiOsReadPciConfiguration/' | \
	sed 's/^AcpiOsSignalSemaphore/__AcpiOsSignalSemaphore/' |	\
	sed 's/^AcpiOsWaitSemaphore/__AcpiOsWaitSemaphore/' |		\
	sed 's/^AcpiOsCreateSemaphore/__AcpiOsCreateSemaphore/' |	\
	sed 's/^AcpiOsDeleteSemaphore/__AcpiOsDeleteSemaphore/' |	\
	sed 's/^AcpiOsVprintf/__AcpiOsVprintf/' |			\
	sed 's/^AcpiOsSignal/__AcpiOsSignal/' |				\
	sed 's/^AcpiOsSleep/__AcpiOsSleep/' |				\
	sed 's/^AcpiOsExecute/__AcpiOsExecute/'				\
	> $@
#
#  Force maximum loop iterations to be just 128 instead of 0xffff
#  because we really don't want to wait until the sun turns into
#  a lump of coal before we complete a blocked operation
#
dscontrol_munged.c: ../acpica/source/components/dispatcher/dscontrol.c
	cat $^ |					\
	sed 's/ACPI_MAX_LOOP_ITERATIONS/0x0080/'	\
	> $@

BUILT_SOURCES = osunixxf_munged.c dscontrol_munged.c

#
#  Source files that are generated on-the fly and need cleaning
#
CLEANFILES = osunixxf_munged.c				\
	dscontrol_munged.c				\
	../acpica/source/compiler/aslcompiler.output	\
	../acpica/source/compiler/dtparser.output	\
	../acpica/source/compiler/dtparser.y.h		\
	../acpica/source/compiler/prparser.output	\
	../acpica/source/compiler/prparser.y.h		\
	../acpica/source/compiler/prparserlex.c		\
	../acpica/source/compiler/prparserparse.c

pkglib_LTLIBRARIES = libfwtsacpica.la

libfwtsacpica_la_LDFLAGS = -lpthread -version-info 1:0:0
libfwtsacpica_la_CPPFLAGS = $(AM_CPPFLAGS)

#
#  fwts acpica library sources
#
libfwtsacpica_la_SOURCES =					\
	fwts_acpica.c						\
	osunixxf_munged.c					\
	dscontrol_munged.c					\
	../acpica/source/components/debugger/dbcmds.c		\
	../acpica/source/components/debugger/dbdisply.c		\
	../acpica/source/components/debugger/dbexec.c		\
	../acpica/source/components/debugger/dbfileio.c		\
	../acpica/source/components/debugger/dbhistry.c		\
	../acpica/source/components/debugger/dbinput.c		\
	../acpica/source/components/debugger/dbstats.c		\
	../acpica/source/components/debugger/dbobject.c		\
	../acpica/source/components/debugger/dbutils.c		\
	../acpica/source/components/debugger/dbxface.c		\
	../acpica/source/components/debugger/dbmethod.c		\
	../acpica/source/components/debugger/dbnames.c		\
	../acpica/source/components/debugger/dbconvert.c	\
	../acpica/source/components/debugger/dbtest.c		\
	../acpica/source/components/disassembler/dmbuffer.c	\
	../acpica/source/components/disassembler/dmcstyle.c	\
	../acpica/source/components/disassembler/dmnames.c	\
	../acpica/source/components/disassembler/dmopcode.c	\
	../acpica/source/components/disassembler/dmresrc.c	\
	../acpica/source/components/disassembler/dmresrcl.c	\
	../acpica/source/components/disassembler/dmresrcs.c	\
	../acpica/source/components/disassembler/dmutils.c	\
	../acpica/source/components/disassembler/dmwalk.c	\
	../acpica/source/components/disassembler/dmresrcl2.c	\
	../acpica/source/components/disassembler/dmdeferred.c 	\
	../acpica/source/components/dispatcher/dsdebug.c	\
	../acpica/source/components/dispatcher/dsfield.c	\
	../acpica/source/components/dispatcher/dsinit.c		\
	../acpica/source/components/dispatcher/dsmethod.c	\
	../acpica/source/components/dispatcher/dsmthdat.c	\
	../acpica/source/components/dispatcher/dsobject.c	\
	../acpica/source/components/dispatcher/dspkginit.c	\
	../acpica/source/components/dispatcher/dsutils.c	\
	../acpica/source/components/dispatcher/dswexec.c	\
	../acpica/source/components/dispatcher/dswload.c	\
	../acpica/source/components/dispatcher/dswscope.c	\
	../acpica/source/components/dispatcher/dswstate.c	\
	../acpica/source/components/dispatcher/dsargs.c		\
	../acpica/source/components/dispatcher/dswload2.c	\
	../acpica/source/components/dispatcher/dsopcode.c	\
	../acpica/source/components/events/evevent.c		\
	../acpica/source/components/events/evgpe.c		\
	../acpica/source/components/events/evgpeblk.c		\
	../acpica/source/components/events/evgpeinit.c		\
	../acpica/source/components/events/evgpeutil.c		\
	../acpica/source/components/events/evxfgpe.c		\
	../acpica/source/components/events/evmisc.c		\
	../acpica/source/components/events/evregion.c		\
	../acpica/source/components/events/evrgnini.c		\
	../acpica/source/components/events/evsci.c		\
	../acpica/source/components/events/evxface.c		\
	../acpica/source/components/events/evxfevnt.c		\
	../acpica/source/components/events/evxfregn.c		\
	../acpica/source/components/events/evglock.c		\
	../acpica/source/components/events/evhandler.c		\
	../acpica/source/components/executer/exconcat.c		\
	../acpica/source/components/executer/exfield.c		\
	../acpica/source/components/executer/exfldio.c		\
	../acpica/source/components/executer/exmisc.c		\
	../acpica/source/components/executer/exmutex.c		\
	../acpica/source/components/executer/exnames.c		\
	../acpica/source/components/executer/exoparg1.c		\
	../acpica/source/components/executer/exoparg2.c		\
	../acpica/source/components/executer/exoparg3.c		\
	../acpica/source/components/executer/exoparg6.c		\
	../acpica/source/components/executer/exprep.c		\
	../acpica/source/components/executer/exregion.c		\
	../acpica/source/components/executer/exresnte.c		\
	../acpica/source/components/executer/exresolv.c		\
	../acpica/source/components/executer/exresop.c		\
	../acpica/source/components/executer/exserial.c		\
	../acpica/source/components/executer/exstore.c		\
	../acpica/source/components/executer/exstoren.c		\
	../acpica/source/components/executer/exstorob.c		\
	../acpica/source/components/executer/exsystem.c		\
	../acpica/source/components/executer/extrace.c		\
	../acpica/source/components/executer/exutils.c		\
	../acpica/source/components/executer/exconvrt.c		\
	../acpica/source/components/executer/excreate.c		\
	../acpica/source/components/executer/exdump.c		\
	../acpica/source/components/executer/exdebug.c		\
	../acpica/source/components/executer/exconfig.c		\
	../acpica/source/components/hardware/hwacpi.c		\
	../acpica/source/components/hardware/hwgpe.c		\
	../acpica/source/components/hardware/hwpci.c		\
	../acpica/source/components/hardware/hwregs.c		\
	../acpica/source/components/hardware/hwsleep.c		\
	../acpica/source/components/hardware/hwvalid.c		\
	../acpica/source/components/hardware/hwxface.c		\
	../acpica/source/components/hardware/hwxfsleep.c	\
	../acpica/source/components/hardware/hwesleep.c		\
	../acpica/source/components/namespace/nsaccess.c	\
	../acpica/source/components/namespace/nsalloc.c		\
	../acpica/source/components/namespace/nsdump.c		\
	../acpica/source/components/namespace/nsdumpdv.c	\
	../acpica/source/components/namespace/nseval.c		\
	../acpica/source/components/namespace/nsinit.c		\
	../acpica/source/components/namespace/nsload.c		\
	../acpica/source/components/namespace/nsnames.c		\
	../acpica/source/components/namespace/nsobject.c	\
	../acpica/source/components/namespace/nsparse.c		\
	../acpica/source/components/namespace/nspredef.c	\
	../acpica/source/components/namespace/nsrepair.c	\
	../acpica/source/components/namespace/nsrepair2.c	\
	../acpica/source/components/namespace/nssearch.c	\
	../acpica/source/components/namespace/nsutils.c		\
	../acpica/source/components/namespace/nswalk.c		\
	../acpica/source/components/namespace/nsxfeval.c	\
	../acpica/source/components/namespace/nsxfname.c	\
	../acpica/source/components/namespace/nsxfobj.c		\
	../acpica/source/components/namespace/nsconvert.c	\
	../acpica/source/components/namespace/nsprepkg.c	\
	../acpica/source/components/namespace/nsarguments.c	\
	../acpica/source/components/parser/psargs.c		\
	../acpica/source/components/parser/psloop.c		\
	../acpica/source/components/parser/psopcode.c		\
	../acpica/source/components/parser/psparse.c		\
	../acpica/source/components/parser/psscope.c		\
	../acpica/source/components/parser/pstree.c		\
	../acpica/source/components/parser/psutils.c		\
	../acpica/source/components/parser/pswalk.c		\
	../acpica/source/components/parser/psxface.c		\
	../acpica/source/components/parser/psopinfo.c		\
	../acpica/source/components/parser/psobject.c		\
	../acpica/source/components/resources/rsaddr.c		\
	../acpica/source/components/resources/rscalc.c		\
	../acpica/source/components/resources/rscreate.c	\
	../acpica/source/components/resources/rsdump.c		\
	../acpica/source/components/resources/rsio.c		\
	../acpica/source/components/resources/rsinfo.c		\
	../acpica/source/components/resources/rsirq.c		\
	../acpica/source/components/resources/rslist.c		\
	../acpica/source/components/resources/rsmemory.c	\
	../acpica/source/components/resources/rsmisc.c		\
	../acpica/source/components/resources/rsutils.c		\
	../acpica/source/components/resources/rsxface.c		\
	../acpica/source/components/resources/rsserial.c	\
	../acpica/source/components/resources/rsdumpinfo.c	\
	../acpica/source/components/tables/tbdata.c		\
	../acpica/source/components/tables/tbfadt.c		\
	../acpica/source/components/tables/tbfind.c		\
	../acpica/source/components/tables/tbinstal.c		\
	../acpica/source/components/tables/tbutils.c		\
	../acpica/source/components/tables/tbxface.c		\
	../acpica/source/components/tables/tbxfroot.c		\
	../acpica/source/components/tables/tbxfload.c		\
	../acpica/source/components/tables/tbprint.c		\
	../acpica/source/components/utilities/utaddress.c	\
	../acpica/source/components/utilities/utalloc.c		\
	../acpica/source/components/utilities/utascii.c		\
	../acpica/source/components/utilities/utcache.c		\
	../acpica/source/components/utilities/utcopy.c		\
	../acpica/source/components/utilities/utdebug.c		\
	../acpica/source/components/utilities/utdelete.c	\
	../acpica/source/components/utilities/uteval.c		\
	../acpica/source/components/utilities/utglobal.c	\
	../acpica/source/components/utilities/uthex.c		\
	../acpica/source/components/utilities/utids.c		\
	../acpica/source/components/utilities/utinit.c		\
	../acpica/source/components/utilities/utlock.c		\
	../acpica/source/components/utilities/utmath.c		\
	../acpica/source/components/utilities/utmisc.c		\
	../acpica/source/components/utilities/utmutex.c		\
	../acpica/source/components/utilities/utnonansi.c	\
	../acpica/source/components/utilities/utobject.c	\
	../acpica/source/components/utilities/utresdecode.c	\
	../acpica/source/components/utilities/utresrc.c		\
	../acpica/source/components/utilities/utstate.c		\
	../acpica/source/components/utilities/utstrtoul64.c	\
	../acpica/source/components/utilities/uttrack.c		\
	../acpica/source/components/utilities/utosi.c		\
	../acpica/source/components/utilities/utxferror.c	\
	../acpica/source/components/utilities/utxface.c		\
	../acpica/source/components/utilities/utdecode.c	\
	../acpica/source/components/utilities/utexcep.c		\
	../acpica/source/components/utilities/utpredef.c	\
	../acpica/source/components/utilities/utstring.c	\
	../acpica/source/components/utilities/utstrsuppt.c	\
	../acpica/source/components/utilities/utownerid.c	\
	../acpica/source/components/utilities/utxfinit.c	\
	../acpica/source/components/utilities/uterror.c		\
	../acpica/source/components/utilities/utbuffer.c 	\
	../acpica/source/components/utilities/utuuid.c		\
	../acpica/source/common/acfileio.c			\
	../acpica/source/common/acgetline.c            		\
	../acpica/source/common/ahids.c				\
	../acpica/source/common/cmfsize.c            		\
	../acpica/source/common/ahtable.c			\
	../acpica/source/common/ahuuids.c			\
	../acpica/source/tools/acpiexec/aeinitfile.c		\
	../acpica/source/tools/acpiexec/aehandlers.c		\
	../acpica/source/tools/acpiexec/aeexception.c		\
	../acpica/source/tools/acpiexec/aeregion.c		\
	../acpica/source/tools/acpiexec/aeinstall.c		\
	../acpica/source/os_specific/service_layers/osgendbg.c

libfwtsacpica_la_LIBADD = \
	-L$(top_builddir)/src/lib/src -lfwts -lrt


-include $(top_srcdir)/git.mk
