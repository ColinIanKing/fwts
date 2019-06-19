#
# Copyright (C) 2010-2019 Canonical, Ltd.
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

ACPICA = $(top_srcdir)/src/acpica/source

#
# -DACPI_EXEC_APP is specific to making ACPICA for the fwts method test
#
AM_CPPFLAGS = 						\
	-D_LINUX -DNDEBUG -D_CONSOLE -DACPI_EXEC_APP	\
	-I$(top_srcdir)/src/lib/include			\
	-I$(ACPICA)/include				\
	-I$(ACPICA)/tools/acpiexec			\
	`pkg-config --silence-errors --cflags json`	\
	`pkg-config --silence-errors --cflags json-c`	\
	-Wall -fno-strict-aliasing			\
	-Wno-address-of-packed-member

#
#  We rename some of the functions so we can override them in fwts. This
#  is a hack, I feel dirty now.
#
osunixxf_munged.c: $(ACPICA)/os_specific/service_layers/osunixxf.c
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
dscontrol_munged.c: $(ACPICA)/components/dispatcher/dscontrol.c
	cat $^ |					\
	sed 's/ACPI_MAX_LOOP_ITERATIONS/0x0080/'	\
	> $@

BUILT_SOURCES = osunixxf_munged.c dscontrol_munged.c

#
#  Source files that are generated on-the fly and need cleaning
#
CLEANFILES = osunixxf_munged.c			\
	dscontrol_munged.c			\
	$(ACPICA)/compiler/aslcompiler.output	\
	$(ACPICA)/compiler/dtparser.output	\
	$(ACPICA)/compiler/dtparser.y.h		\
	$(ACPICA)/compiler/prparser.output	\
	$(ACPICA)/compiler/prparser.y.h		\
	$(ACPICA)/compiler/prparserlex.c	\
	$(ACPICA)/compiler/prparserparse.c

pkglib_LTLIBRARIES = libfwtsacpica.la

libfwtsacpica_la_LDFLAGS = -lpthread -version-info 1:0:0
libfwtsacpica_la_CPPFLAGS = $(AM_CPPFLAGS)

#
#  fwts acpica library sources
#
libfwtsacpica_la_SOURCES =				\
	fwts_acpica.c					\
	osunixxf_munged.c				\
	dscontrol_munged.c				\
	$(ACPICA)/components/debugger/dbcmds.c		\
	$(ACPICA)/components/debugger/dbdisply.c	\
	$(ACPICA)/components/debugger/dbexec.c		\
	$(ACPICA)/components/debugger/dbfileio.c	\
	$(ACPICA)/components/debugger/dbhistry.c	\
	$(ACPICA)/components/debugger/dbinput.c		\
	$(ACPICA)/components/debugger/dbstats.c		\
	$(ACPICA)/components/debugger/dbobject.c	\
	$(ACPICA)/components/debugger/dbutils.c		\
	$(ACPICA)/components/debugger/dbxface.c		\
	$(ACPICA)/components/debugger/dbmethod.c	\
	$(ACPICA)/components/debugger/dbnames.c		\
	$(ACPICA)/components/debugger/dbconvert.c	\
	$(ACPICA)/components/debugger/dbtest.c		\
	$(ACPICA)/components/disassembler/dmbuffer.c	\
	$(ACPICA)/components/disassembler/dmcstyle.c	\
	$(ACPICA)/components/disassembler/dmnames.c	\
	$(ACPICA)/components/disassembler/dmopcode.c	\
	$(ACPICA)/components/disassembler/dmresrc.c	\
	$(ACPICA)/components/disassembler/dmresrcl.c	\
	$(ACPICA)/components/disassembler/dmresrcs.c	\
	$(ACPICA)/components/disassembler/dmutils.c	\
	$(ACPICA)/components/disassembler/dmwalk.c	\
	$(ACPICA)/components/disassembler/dmresrcl2.c	\
	$(ACPICA)/components/disassembler/dmdeferred.c 	\
	$(ACPICA)/components/dispatcher/dsdebug.c	\
	$(ACPICA)/components/dispatcher/dsfield.c	\
	$(ACPICA)/components/dispatcher/dsinit.c	\
	$(ACPICA)/components/dispatcher/dsmethod.c	\
	$(ACPICA)/components/dispatcher/dsmthdat.c	\
	$(ACPICA)/components/dispatcher/dsobject.c	\
	$(ACPICA)/components/dispatcher/dspkginit.c	\
	$(ACPICA)/components/dispatcher/dsutils.c	\
	$(ACPICA)/components/dispatcher/dswexec.c	\
	$(ACPICA)/components/dispatcher/dswload.c	\
	$(ACPICA)/components/dispatcher/dswscope.c	\
	$(ACPICA)/components/dispatcher/dswstate.c	\
	$(ACPICA)/components/dispatcher/dsargs.c	\
	$(ACPICA)/components/dispatcher/dswload2.c	\
	$(ACPICA)/components/dispatcher/dsopcode.c	\
	$(ACPICA)/components/events/evevent.c		\
	$(ACPICA)/components/events/evgpe.c		\
	$(ACPICA)/components/events/evgpeblk.c		\
	$(ACPICA)/components/events/evgpeinit.c		\
	$(ACPICA)/components/events/evgpeutil.c		\
	$(ACPICA)/components/events/evxfgpe.c		\
	$(ACPICA)/components/events/evmisc.c		\
	$(ACPICA)/components/events/evregion.c		\
	$(ACPICA)/components/events/evrgnini.c		\
	$(ACPICA)/components/events/evsci.c		\
	$(ACPICA)/components/events/evxface.c		\
	$(ACPICA)/components/events/evxfevnt.c		\
	$(ACPICA)/components/events/evxfregn.c		\
	$(ACPICA)/components/events/evglock.c		\
	$(ACPICA)/components/events/evhandler.c		\
	$(ACPICA)/components/executer/exconcat.c	\
	$(ACPICA)/components/executer/exfield.c		\
	$(ACPICA)/components/executer/exfldio.c		\
	$(ACPICA)/components/executer/exmisc.c		\
	$(ACPICA)/components/executer/exmutex.c		\
	$(ACPICA)/components/executer/exnames.c		\
	$(ACPICA)/components/executer/exoparg1.c	\
	$(ACPICA)/components/executer/exoparg2.c	\
	$(ACPICA)/components/executer/exoparg3.c	\
	$(ACPICA)/components/executer/exoparg6.c	\
	$(ACPICA)/components/executer/exprep.c		\
	$(ACPICA)/components/executer/exregion.c	\
	$(ACPICA)/components/executer/exresnte.c	\
	$(ACPICA)/components/executer/exresolv.c	\
	$(ACPICA)/components/executer/exresop.c		\
	$(ACPICA)/components/executer/exserial.c	\
	$(ACPICA)/components/executer/exstore.c		\
	$(ACPICA)/components/executer/exstoren.c	\
	$(ACPICA)/components/executer/exstorob.c	\
	$(ACPICA)/components/executer/exsystem.c	\
	$(ACPICA)/components/executer/extrace.c		\
	$(ACPICA)/components/executer/exutils.c		\
	$(ACPICA)/components/executer/exconvrt.c	\
	$(ACPICA)/components/executer/excreate.c	\
	$(ACPICA)/components/executer/exdump.c		\
	$(ACPICA)/components/executer/exdebug.c		\
	$(ACPICA)/components/executer/exconfig.c	\
	$(ACPICA)/components/hardware/hwacpi.c		\
	$(ACPICA)/components/hardware/hwgpe.c		\
	$(ACPICA)/components/hardware/hwpci.c		\
	$(ACPICA)/components/hardware/hwregs.c		\
	$(ACPICA)/components/hardware/hwsleep.c		\
	$(ACPICA)/components/hardware/hwvalid.c		\
	$(ACPICA)/components/hardware/hwxface.c		\
	$(ACPICA)/components/hardware/hwxfsleep.c	\
	$(ACPICA)/components/hardware/hwesleep.c	\
	$(ACPICA)/components/namespace/nsaccess.c	\
	$(ACPICA)/components/namespace/nsalloc.c	\
	$(ACPICA)/components/namespace/nsdump.c		\
	$(ACPICA)/components/namespace/nsdumpdv.c	\
	$(ACPICA)/components/namespace/nseval.c		\
	$(ACPICA)/components/namespace/nsinit.c		\
	$(ACPICA)/components/namespace/nsload.c		\
	$(ACPICA)/components/namespace/nsnames.c	\
	$(ACPICA)/components/namespace/nsobject.c	\
	$(ACPICA)/components/namespace/nsparse.c	\
	$(ACPICA)/components/namespace/nspredef.c	\
	$(ACPICA)/components/namespace/nsrepair.c	\
	$(ACPICA)/components/namespace/nsrepair2.c	\
	$(ACPICA)/components/namespace/nssearch.c	\
	$(ACPICA)/components/namespace/nsutils.c	\
	$(ACPICA)/components/namespace/nswalk.c		\
	$(ACPICA)/components/namespace/nsxfeval.c	\
	$(ACPICA)/components/namespace/nsxfname.c	\
	$(ACPICA)/components/namespace/nsxfobj.c	\
	$(ACPICA)/components/namespace/nsconvert.c	\
	$(ACPICA)/components/namespace/nsprepkg.c	\
	$(ACPICA)/components/namespace/nsarguments.c	\
	$(ACPICA)/components/parser/psargs.c		\
	$(ACPICA)/components/parser/psloop.c		\
	$(ACPICA)/components/parser/psopcode.c		\
	$(ACPICA)/components/parser/psparse.c		\
	$(ACPICA)/components/parser/psscope.c		\
	$(ACPICA)/components/parser/pstree.c		\
	$(ACPICA)/components/parser/psutils.c		\
	$(ACPICA)/components/parser/pswalk.c		\
	$(ACPICA)/components/parser/psxface.c		\
	$(ACPICA)/components/parser/psopinfo.c		\
	$(ACPICA)/components/parser/psobject.c		\
	$(ACPICA)/components/resources/rsaddr.c		\
	$(ACPICA)/components/resources/rscalc.c		\
	$(ACPICA)/components/resources/rscreate.c	\
	$(ACPICA)/components/resources/rsdump.c		\
	$(ACPICA)/components/resources/rsio.c		\
	$(ACPICA)/components/resources/rsinfo.c		\
	$(ACPICA)/components/resources/rsirq.c		\
	$(ACPICA)/components/resources/rslist.c		\
	$(ACPICA)/components/resources/rsmemory.c	\
	$(ACPICA)/components/resources/rsmisc.c		\
	$(ACPICA)/components/resources/rsutils.c	\
	$(ACPICA)/components/resources/rsxface.c	\
	$(ACPICA)/components/resources/rsserial.c	\
	$(ACPICA)/components/resources/rsdumpinfo.c	\
	$(ACPICA)/components/tables/tbdata.c		\
	$(ACPICA)/components/tables/tbfadt.c		\
	$(ACPICA)/components/tables/tbfind.c		\
	$(ACPICA)/components/tables/tbinstal.c		\
	$(ACPICA)/components/tables/tbutils.c		\
	$(ACPICA)/components/tables/tbxface.c		\
	$(ACPICA)/components/tables/tbxfroot.c		\
	$(ACPICA)/components/tables/tbxfload.c		\
	$(ACPICA)/components/tables/tbprint.c		\
	$(ACPICA)/components/utilities/utaddress.c	\
	$(ACPICA)/components/utilities/utalloc.c	\
	$(ACPICA)/components/utilities/utascii.c	\
	$(ACPICA)/components/utilities/utcache.c	\
	$(ACPICA)/components/utilities/utcopy.c		\
	$(ACPICA)/components/utilities/utdebug.c	\
	$(ACPICA)/components/utilities/utdelete.c	\
	$(ACPICA)/components/utilities/uteval.c		\
	$(ACPICA)/components/utilities/utglobal.c	\
	$(ACPICA)/components/utilities/uthex.c		\
	$(ACPICA)/components/utilities/utids.c		\
	$(ACPICA)/components/utilities/utinit.c		\
	$(ACPICA)/components/utilities/utlock.c		\
	$(ACPICA)/components/utilities/utmath.c		\
	$(ACPICA)/components/utilities/utmisc.c		\
	$(ACPICA)/components/utilities/utmutex.c	\
	$(ACPICA)/components/utilities/utnonansi.c	\
	$(ACPICA)/components/utilities/utobject.c	\
	$(ACPICA)/components/utilities/utresdecode.c	\
	$(ACPICA)/components/utilities/utresrc.c	\
	$(ACPICA)/components/utilities/utstate.c	\
	$(ACPICA)/components/utilities/utstrtoul64.c	\
	$(ACPICA)/components/utilities/uttrack.c	\
	$(ACPICA)/components/utilities/utosi.c		\
	$(ACPICA)/components/utilities/utxferror.c	\
	$(ACPICA)/components/utilities/utxface.c	\
	$(ACPICA)/components/utilities/utdecode.c	\
	$(ACPICA)/components/utilities/utexcep.c	\
	$(ACPICA)/components/utilities/utpredef.c	\
	$(ACPICA)/components/utilities/utstring.c	\
	$(ACPICA)/components/utilities/utstrsuppt.c	\
	$(ACPICA)/components/utilities/utownerid.c	\
	$(ACPICA)/components/utilities/utxfinit.c	\
	$(ACPICA)/components/utilities/uterror.c	\
	$(ACPICA)/components/utilities/utbuffer.c 	\
	$(ACPICA)/components/utilities/utuuid.c		\
	$(ACPICA)/common/acfileio.c			\
	$(ACPICA)/common/acgetline.c            	\
	$(ACPICA)/common/ahids.c			\
	$(ACPICA)/common/cmfsize.c            		\
	$(ACPICA)/common/ahtable.c			\
	$(ACPICA)/common/ahuuids.c			\
	$(ACPICA)/tools/acpiexec/aeinitfile.c		\
	$(ACPICA)/tools/acpiexec/aehandlers.c		\
	$(ACPICA)/tools/acpiexec/aeexception.c		\
	$(ACPICA)/tools/acpiexec/aeregion.c		\
	$(ACPICA)/tools/acpiexec/aeinstall.c		\
	$(ACPICA)/os_specific/service_layers/osgendbg.c

libfwtsacpica_la_LIBADD = \
	-L$(top_builddir)/src/lib/src -lfwts -lrt

