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
	-I$(srcdir)/../acpica/source/include		\
	-I$(srcdir)/../acpica/source/tools/acpiexec	\
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
dscontrol_munged.c: $(srcdir)/../acpica/source/components/dispatcher/dscontrol.c
	cat $^ |					\
	sed 's/ACPI_MAX_LOOP_ITERATIONS/0x0080/'	\
	> $@

BUILT_SOURCES = osunixxf_munged.c dscontrol_munged.c

#
#  Source files that are generated on-the fly and need cleaning
#
CLEANFILES = osunixxf_munged.c					\
	dscontrol_munged.c					\
	$(srcdir)/../acpica/source/compiler/aslcompiler.output	\
	$(srcdir)/../acpica/source/compiler/dtparser.output	\
	$(srcdir)/../acpica/source/compiler/dtparser.y.h	\
	$(srcdir)/../acpica/source/compiler/prparser.output	\
	$(srcdir)/../acpica/source/compiler/prparser.y.h	\
	$(srcdir)/../acpica/source/compiler/prparserlex.c	\
	$(srcdir)/../acpica/source/compiler/prparserparse.c

pkglib_LTLIBRARIES = libfwtsacpica.la

libfwtsacpica_la_LDFLAGS = -lpthread -version-info 1:0:0
libfwtsacpica_la_CPPFLAGS = $(AM_CPPFLAGS)

#
#  fwts acpica library sources
#
libfwtsacpica_la_SOURCES =						\
	fwts_acpica.c							\
	osunixxf_munged.c						\
	dscontrol_munged.c						\
	$(srcdir)/../acpica/source/components/debugger/dbcmds.c		\
	$(srcdir)/../acpica/source/components/debugger/dbdisply.c	\
	$(srcdir)/../acpica/source/components/debugger/dbexec.c		\
	$(srcdir)/../acpica/source/components/debugger/dbfileio.c	\
	$(srcdir)/../acpica/source/components/debugger/dbhistry.c	\
	$(srcdir)/../acpica/source/components/debugger/dbinput.c	\
	$(srcdir)/../acpica/source/components/debugger/dbstats.c	\
	$(srcdir)/../acpica/source/components/debugger/dbobject.c	\
	$(srcdir)/../acpica/source/components/debugger/dbutils.c	\
	$(srcdir)/../acpica/source/components/debugger/dbxface.c	\
	$(srcdir)/../acpica/source/components/debugger/dbmethod.c	\
	$(srcdir)/../acpica/source/components/debugger/dbnames.c	\
	$(srcdir)/../acpica/source/components/debugger/dbconvert.c	\
	$(srcdir)/../acpica/source/components/debugger/dbtest.c		\
	$(srcdir)/../acpica/source/components/disassembler/dmbuffer.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmcstyle.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmnames.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmopcode.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmresrc.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmresrcl.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmresrcs.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmutils.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmwalk.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmresrcl2.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmdeferred.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsdebug.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsfield.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsinit.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsmethod.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsmthdat.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsobject.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dspkginit.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsutils.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dswexec.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dswload.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dswscope.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dswstate.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsargs.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dswload2.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsopcode.c	\
	$(srcdir)/../acpica/source/components/events/evevent.c		\
	$(srcdir)/../acpica/source/components/events/evgpe.c		\
	$(srcdir)/../acpica/source/components/events/evgpeblk.c		\
	$(srcdir)/../acpica/source/components/events/evgpeinit.c	\
	$(srcdir)/../acpica/source/components/events/evgpeutil.c	\
	$(srcdir)/../acpica/source/components/events/evxfgpe.c		\
	$(srcdir)/../acpica/source/components/events/evmisc.c		\
	$(srcdir)/../acpica/source/components/events/evregion.c		\
	$(srcdir)/../acpica/source/components/events/evrgnini.c		\
	$(srcdir)/../acpica/source/components/events/evsci.c		\
	$(srcdir)/../acpica/source/components/events/evxface.c		\
	$(srcdir)/../acpica/source/components/events/evxfevnt.c		\
	$(srcdir)/../acpica/source/components/events/evxfregn.c		\
	$(srcdir)/../acpica/source/components/events/evglock.c		\
	$(srcdir)/../acpica/source/components/events/evhandler.c	\
	$(srcdir)/../acpica/source/components/executer/exconcat.c	\
	$(srcdir)/../acpica/source/components/executer/exfield.c	\
	$(srcdir)/../acpica/source/components/executer/exfldio.c	\
	$(srcdir)/../acpica/source/components/executer/exmisc.c		\
	$(srcdir)/../acpica/source/components/executer/exmutex.c	\
	$(srcdir)/../acpica/source/components/executer/exnames.c	\
	$(srcdir)/../acpica/source/components/executer/exoparg1.c	\
	$(srcdir)/../acpica/source/components/executer/exoparg2.c	\
	$(srcdir)/../acpica/source/components/executer/exoparg3.c	\
	$(srcdir)/../acpica/source/components/executer/exoparg6.c	\
	$(srcdir)/../acpica/source/components/executer/exprep.c		\
	$(srcdir)/../acpica/source/components/executer/exregion.c	\
	$(srcdir)/../acpica/source/components/executer/exresnte.c	\
	$(srcdir)/../acpica/source/components/executer/exresolv.c	\
	$(srcdir)/../acpica/source/components/executer/exresop.c	\
	$(srcdir)/../acpica/source/components/executer/exserial.c	\
	$(srcdir)/../acpica/source/components/executer/exstore.c	\
	$(srcdir)/../acpica/source/components/executer/exstoren.c	\
	$(srcdir)/../acpica/source/components/executer/exstorob.c	\
	$(srcdir)/../acpica/source/components/executer/exsystem.c	\
	$(srcdir)/../acpica/source/components/executer/extrace.c	\
	$(srcdir)/../acpica/source/components/executer/exutils.c	\
	$(srcdir)/../acpica/source/components/executer/exconvrt.c	\
	$(srcdir)/../acpica/source/components/executer/excreate.c	\
	$(srcdir)/../acpica/source/components/executer/exdump.c		\
	$(srcdir)/../acpica/source/components/executer/exdebug.c	\
	$(srcdir)/../acpica/source/components/executer/exconfig.c	\
	$(srcdir)/../acpica/source/components/hardware/hwacpi.c		\
	$(srcdir)/../acpica/source/components/hardware/hwgpe.c		\
	$(srcdir)/../acpica/source/components/hardware/hwpci.c		\
	$(srcdir)/../acpica/source/components/hardware/hwregs.c		\
	$(srcdir)/../acpica/source/components/hardware/hwsleep.c	\
	$(srcdir)/../acpica/source/components/hardware/hwvalid.c	\
	$(srcdir)/../acpica/source/components/hardware/hwxface.c	\
	$(srcdir)/../acpica/source/components/hardware/hwxfsleep.c	\
	$(srcdir)/../acpica/source/components/hardware/hwesleep.c	\
	$(srcdir)/../acpica/source/components/namespace/nsaccess.c	\
	$(srcdir)/../acpica/source/components/namespace/nsalloc.c	\
	$(srcdir)/../acpica/source/components/namespace/nsdump.c	\
	$(srcdir)/../acpica/source/components/namespace/nsdumpdv.c	\
	$(srcdir)/../acpica/source/components/namespace/nseval.c	\
	$(srcdir)/../acpica/source/components/namespace/nsinit.c	\
	$(srcdir)/../acpica/source/components/namespace/nsload.c	\
	$(srcdir)/../acpica/source/components/namespace/nsnames.c	\
	$(srcdir)/../acpica/source/components/namespace/nsobject.c	\
	$(srcdir)/../acpica/source/components/namespace/nsparse.c	\
	$(srcdir)/../acpica/source/components/namespace/nspredef.c	\
	$(srcdir)/../acpica/source/components/namespace/nsrepair.c	\
	$(srcdir)/../acpica/source/components/namespace/nsrepair2.c	\
	$(srcdir)/../acpica/source/components/namespace/nssearch.c	\
	$(srcdir)/../acpica/source/components/namespace/nsutils.c	\
	$(srcdir)/../acpica/source/components/namespace/nswalk.c	\
	$(srcdir)/../acpica/source/components/namespace/nsxfeval.c	\
	$(srcdir)/../acpica/source/components/namespace/nsxfname.c	\
	$(srcdir)/../acpica/source/components/namespace/nsxfobj.c	\
	$(srcdir)/../acpica/source/components/namespace/nsconvert.c	\
	$(srcdir)/../acpica/source/components/namespace/nsprepkg.c	\
	$(srcdir)/../acpica/source/components/namespace/nsarguments.c	\
	$(srcdir)/../acpica/source/components/parser/psargs.c		\
	$(srcdir)/../acpica/source/components/parser/psloop.c		\
	$(srcdir)/../acpica/source/components/parser/psopcode.c		\
	$(srcdir)/../acpica/source/components/parser/psparse.c		\
	$(srcdir)/../acpica/source/components/parser/psscope.c		\
	$(srcdir)/../acpica/source/components/parser/pstree.c		\
	$(srcdir)/../acpica/source/components/parser/psutils.c		\
	$(srcdir)/../acpica/source/components/parser/pswalk.c		\
	$(srcdir)/../acpica/source/components/parser/psxface.c		\
	$(srcdir)/../acpica/source/components/parser/psopinfo.c		\
	$(srcdir)/../acpica/source/components/parser/psobject.c		\
	$(srcdir)/../acpica/source/components/resources/rsaddr.c	\
	$(srcdir)/../acpica/source/components/resources/rscalc.c	\
	$(srcdir)/../acpica/source/components/resources/rscreate.c	\
	$(srcdir)/../acpica/source/components/resources/rsdump.c	\
	$(srcdir)/../acpica/source/components/resources/rsio.c		\
	$(srcdir)/../acpica/source/components/resources/rsinfo.c	\
	$(srcdir)/../acpica/source/components/resources/rsirq.c		\
	$(srcdir)/../acpica/source/components/resources/rslist.c	\
	$(srcdir)/../acpica/source/components/resources/rsmemory.c	\
	$(srcdir)/../acpica/source/components/resources/rsmisc.c	\
	$(srcdir)/../acpica/source/components/resources/rsutils.c	\
	$(srcdir)/../acpica/source/components/resources/rsxface.c	\
	$(srcdir)/../acpica/source/components/resources/rsserial.c	\
	$(srcdir)/../acpica/source/components/resources/rsdumpinfo.c	\
	$(srcdir)/../acpica/source/components/tables/tbdata.c		\
	$(srcdir)/../acpica/source/components/tables/tbfadt.c		\
	$(srcdir)/../acpica/source/components/tables/tbfind.c		\
	$(srcdir)/../acpica/source/components/tables/tbinstal.c		\
	$(srcdir)/../acpica/source/components/tables/tbutils.c		\
	$(srcdir)/../acpica/source/components/tables/tbxface.c		\
	$(srcdir)/../acpica/source/components/tables/tbxfroot.c		\
	$(srcdir)/../acpica/source/components/tables/tbxfload.c		\
	$(srcdir)/../acpica/source/components/tables/tbprint.c		\
	$(srcdir)/../acpica/source/components/utilities/utaddress.c	\
	$(srcdir)/../acpica/source/components/utilities/utalloc.c	\
	$(srcdir)/../acpica/source/components/utilities/utascii.c	\
	$(srcdir)/../acpica/source/components/utilities/utcache.c	\
	$(srcdir)/../acpica/source/components/utilities/utcopy.c	\
	$(srcdir)/../acpica/source/components/utilities/utdebug.c	\
	$(srcdir)/../acpica/source/components/utilities/utdelete.c	\
	$(srcdir)/../acpica/source/components/utilities/uteval.c	\
	$(srcdir)/../acpica/source/components/utilities/utglobal.c	\
	$(srcdir)/../acpica/source/components/utilities/uthex.c		\
	$(srcdir)/../acpica/source/components/utilities/utids.c		\
	$(srcdir)/../acpica/source/components/utilities/utinit.c	\
	$(srcdir)/../acpica/source/components/utilities/utlock.c	\
	$(srcdir)/../acpica/source/components/utilities/utmath.c	\
	$(srcdir)/../acpica/source/components/utilities/utmisc.c	\
	$(srcdir)/../acpica/source/components/utilities/utmutex.c	\
	$(srcdir)/../acpica/source/components/utilities/utnonansi.c	\
	$(srcdir)/../acpica/source/components/utilities/utobject.c	\
	$(srcdir)/../acpica/source/components/utilities/utresdecode.c	\
	$(srcdir)/../acpica/source/components/utilities/utresrc.c	\
	$(srcdir)/../acpica/source/components/utilities/utstate.c	\
	$(srcdir)/../acpica/source/components/utilities/utstrtoul64.c	\
	$(srcdir)/../acpica/source/components/utilities/uttrack.c	\
	$(srcdir)/../acpica/source/components/utilities/utosi.c		\
	$(srcdir)/../acpica/source/components/utilities/utxferror.c	\
	$(srcdir)/../acpica/source/components/utilities/utxface.c	\
	$(srcdir)/../acpica/source/components/utilities/utdecode.c	\
	$(srcdir)/../acpica/source/components/utilities/utexcep.c	\
	$(srcdir)/../acpica/source/components/utilities/utpredef.c	\
	$(srcdir)/../acpica/source/components/utilities/utstring.c	\
	$(srcdir)/../acpica/source/components/utilities/utstrsuppt.c	\
	$(srcdir)/../acpica/source/components/utilities/utownerid.c	\
	$(srcdir)/../acpica/source/components/utilities/utxfinit.c	\
	$(srcdir)/../acpica/source/components/utilities/uterror.c	\
	$(srcdir)/../acpica/source/components/utilities/utbuffer.c 	\
	$(srcdir)/../acpica/source/components/utilities/utuuid.c	\
	$(srcdir)/../acpica/source/common/acfileio.c			\
	$(srcdir)/../acpica/source/common/acgetline.c            	\
	$(srcdir)/../acpica/source/common/ahids.c			\
	$(srcdir)/../acpica/source/common/cmfsize.c            		\
	$(srcdir)/../acpica/source/common/ahtable.c			\
	$(srcdir)/../acpica/source/common/ahuuids.c			\
	$(srcdir)/../acpica/source/tools/acpiexec/aeinitfile.c		\
	$(srcdir)/../acpica/source/tools/acpiexec/aehandlers.c		\
	$(srcdir)/../acpica/source/tools/acpiexec/aeexception.c		\
	$(srcdir)/../acpica/source/tools/acpiexec/aeregion.c		\
	$(srcdir)/../acpica/source/tools/acpiexec/aeinstall.c		\
	$(srcdir)/../acpica/source/os_specific/service_layers/osgendbg.c

libfwtsacpica_la_LIBADD = \
	-L$(top_builddir)/src/lib/src -lfwts -lrt


-include $(top_srcdir)/git.mk
