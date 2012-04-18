# Nb. the top-level Makefile builds source/compiler for us now...
## SUBDIRS = source/compiler

ACPICA_SRC =    source
ACPICA_COMMON = $(ACPICA_SRC)/common
ACPICA_CORE =   $(ACPICA_SRC)
ACPICA_TOOLS =  $(ACPICA_SRC)/tools
ACPICA_OSL =    $(ACPICA_SRC)/os_specific/service_layers
ACPICA_COMP =   $(ACPICA_SRC)/components

#
# -DACPI_EXEC_APP is specific to making ACPICA for the fwts method test
#
AM_CPPFLAGS =  -D_LINUX -DNDEBUG -D_CONSOLE -DACPI_EXEC_APP -I$(top_srcdir)/src/lib/include -I$(ACPICA_SRC)/include -Wall -fno-strict-aliasing

osunixxf_munged.c: $(ACPICA_OSL)/osunixxf.c
	cat  $(ACPICA_OSL)/osunixxf.c | \
	sed 's/^AcpiOsPrintf/__AcpiOsPrintf/' | \
	sed 's/^AcpiOsReadPort/__AcpiOsReadPort/' | \
	sed 's/^AcpiOsReadPciConfiguration/__AcpiOsReadPciConfiguration/' | \
	sed 's/^AcpiOsSignalSemaphore/__AcpiOsSignalSemaphore/' | \
	sed 's/^AcpiOsWaitSemaphore/__AcpiOsWaitSemaphore/' | \
	sed 's/^AcpiOsVprintf/__AcpiOsVprintf/' | \
	sed 's/^AcpiOsSignal/__AcpiOsSignal/' | \
	sed 's/^AcpiOsSleep/__AcpiOsSleep/' \
	> osunixxf_munged.c

#
#  Force maximum loop iterations to be just 128 instead of 0xffff
#  because we really don't want to wait until the sun turns into
#  a lump of coal before we complete a blocked operation
#
dscontrol_munged.c: $(ACPICA_COMP)/dispatcher/dscontrol.c
	cat $(ACPICA_COMP)/dispatcher/dscontrol.c | \
	sed 's/ACPI_MAX_LOOP_ITERATIONS/0x0080/' \
	> dscontrol_munged.c

BUILT_SOURCES = osunixxf_munged.c dscontrol_munged.c

CLEANFILES = osunixxf_munged.c

lib_LTLIBRARIES = libfwtsacpica.la
libfwtsacpica_la_LDFLAGS = -lpthread -version-info 1:0:0
libfwtsacpica_la_SOURCES = \
	fwts_acpica.c \
	osunixxf_munged.c \
	dscontrol_munged.c \
	$(ACPICA_COMP)/debugger/dbcmds.c \
	$(ACPICA_COMP)/debugger/dbdisply.c \
	$(ACPICA_COMP)/debugger/dbexec.c \
	$(ACPICA_COMP)/debugger/dbfileio.c \
	$(ACPICA_COMP)/debugger/dbhistry.c \
	$(ACPICA_COMP)/debugger/dbinput.c \
	$(ACPICA_COMP)/debugger/dbstats.c \
	$(ACPICA_COMP)/debugger/dbutils.c \
	$(ACPICA_COMP)/debugger/dbxface.c \
	$(ACPICA_COMP)/debugger/dbmethod.c \
	$(ACPICA_COMP)/debugger/dbnames.c \
	$(ACPICA_COMP)/disassembler/dmbuffer.c \
	$(ACPICA_COMP)/disassembler/dmnames.c \
	$(ACPICA_COMP)/disassembler/dmobject.c \
	$(ACPICA_COMP)/disassembler/dmopcode.c \
	$(ACPICA_COMP)/disassembler/dmresrc.c \
	$(ACPICA_COMP)/disassembler/dmresrcl.c \
	$(ACPICA_COMP)/disassembler/dmresrcs.c \
	$(ACPICA_COMP)/disassembler/dmutils.c \
	$(ACPICA_COMP)/disassembler/dmwalk.c \
	$(ACPICA_COMP)/disassembler/dmresrcl2.c \
	$(ACPICA_COMP)/dispatcher/dsfield.c \
	$(ACPICA_COMP)/dispatcher/dsinit.c \
	$(ACPICA_COMP)/dispatcher/dsmethod.c \
	$(ACPICA_COMP)/dispatcher/dsmthdat.c \
	$(ACPICA_COMP)/dispatcher/dsobject.c \
	$(ACPICA_COMP)/dispatcher/dsutils.c \
	$(ACPICA_COMP)/dispatcher/dswexec.c \
	$(ACPICA_COMP)/dispatcher/dswload.c \
	$(ACPICA_COMP)/dispatcher/dswscope.c \
	$(ACPICA_COMP)/dispatcher/dswstate.c \
	$(ACPICA_COMP)/dispatcher/dsargs.c \
	$(ACPICA_COMP)/dispatcher/dswload2.c \
	$(ACPICA_COMP)/dispatcher/dsopcode.c \
	$(ACPICA_COMP)/events/evevent.c \
	$(ACPICA_COMP)/events/evgpe.c \
	$(ACPICA_COMP)/events/evgpeblk.c \
	$(ACPICA_COMP)/events/evgpeinit.c \
	$(ACPICA_COMP)/events/evgpeutil.c \
	$(ACPICA_COMP)/events/evxfgpe.c \
	$(ACPICA_COMP)/events/evmisc.c \
	$(ACPICA_COMP)/events/evregion.c \
	$(ACPICA_COMP)/events/evrgnini.c \
	$(ACPICA_COMP)/events/evsci.c \
	$(ACPICA_COMP)/events/evxface.c \
	$(ACPICA_COMP)/events/evxfevnt.c \
	$(ACPICA_COMP)/events/evxfregn.c \
	$(ACPICA_COMP)/events/evglock.c \
	$(ACPICA_COMP)/executer/exfield.c \
	$(ACPICA_COMP)/executer/exfldio.c \
	$(ACPICA_COMP)/executer/exmisc.c \
	$(ACPICA_COMP)/executer/exmutex.c \
	$(ACPICA_COMP)/executer/exnames.c \
	$(ACPICA_COMP)/executer/exoparg1.c \
	$(ACPICA_COMP)/executer/exoparg2.c \
	$(ACPICA_COMP)/executer/exoparg3.c \
	$(ACPICA_COMP)/executer/exoparg6.c \
	$(ACPICA_COMP)/executer/exprep.c \
	$(ACPICA_COMP)/executer/exregion.c \
	$(ACPICA_COMP)/executer/exresnte.c \
	$(ACPICA_COMP)/executer/exresolv.c \
	$(ACPICA_COMP)/executer/exresop.c \
	$(ACPICA_COMP)/executer/exstore.c \
	$(ACPICA_COMP)/executer/exstoren.c \
	$(ACPICA_COMP)/executer/exstorob.c \
	$(ACPICA_COMP)/executer/exsystem.c \
	$(ACPICA_COMP)/executer/exutils.c \
	$(ACPICA_COMP)/executer/exconvrt.c \
	$(ACPICA_COMP)/executer/excreate.c \
	$(ACPICA_COMP)/executer/exdump.c \
	$(ACPICA_COMP)/executer/exdebug.c \
	$(ACPICA_COMP)/executer/exconfig.c \
	$(ACPICA_COMP)/hardware/hwacpi.c \
	$(ACPICA_COMP)/hardware/hwgpe.c \
	$(ACPICA_COMP)/hardware/hwpci.c \
	$(ACPICA_COMP)/hardware/hwregs.c \
	$(ACPICA_COMP)/hardware/hwsleep.c \
	$(ACPICA_COMP)/hardware/hwvalid.c \
	$(ACPICA_COMP)/hardware/hwxface.c \
	$(ACPICA_COMP)/hardware/hwxfsleep.c \
	$(ACPICA_COMP)/hardware/hwesleep.c \
	$(ACPICA_COMP)/namespace/nsaccess.c \
	$(ACPICA_COMP)/namespace/nsalloc.c \
	$(ACPICA_COMP)/namespace/nsdump.c \
	$(ACPICA_COMP)/namespace/nsdumpdv.c \
	$(ACPICA_COMP)/namespace/nseval.c \
	$(ACPICA_COMP)/namespace/nsinit.c \
	$(ACPICA_COMP)/namespace/nsload.c \
	$(ACPICA_COMP)/namespace/nsnames.c \
	$(ACPICA_COMP)/namespace/nsobject.c \
	$(ACPICA_COMP)/namespace/nsparse.c \
	$(ACPICA_COMP)/namespace/nspredef.c \
	$(ACPICA_COMP)/namespace/nsrepair.c \
	$(ACPICA_COMP)/namespace/nsrepair2.c \
	$(ACPICA_COMP)/namespace/nssearch.c \
	$(ACPICA_COMP)/namespace/nsutils.c \
	$(ACPICA_COMP)/namespace/nswalk.c \
	$(ACPICA_COMP)/namespace/nsxfeval.c \
	$(ACPICA_COMP)/namespace/nsxfname.c \
	$(ACPICA_COMP)/namespace/nsxfobj.c \
	$(ACPICA_COMP)/parser/psargs.c \
	$(ACPICA_COMP)/parser/psloop.c \
	$(ACPICA_COMP)/parser/psopcode.c \
	$(ACPICA_COMP)/parser/psparse.c \
	$(ACPICA_COMP)/parser/psscope.c \
	$(ACPICA_COMP)/parser/pstree.c \
	$(ACPICA_COMP)/parser/psutils.c \
	$(ACPICA_COMP)/parser/pswalk.c \
	$(ACPICA_COMP)/parser/psxface.c \
	$(ACPICA_COMP)/resources/rsaddr.c \
	$(ACPICA_COMP)/resources/rscalc.c \
	$(ACPICA_COMP)/resources/rscreate.c \
	$(ACPICA_COMP)/resources/rsdump.c \
	$(ACPICA_COMP)/resources/rsio.c \
	$(ACPICA_COMP)/resources/rsinfo.c \
	$(ACPICA_COMP)/resources/rsirq.c \
	$(ACPICA_COMP)/resources/rslist.c \
	$(ACPICA_COMP)/resources/rsmemory.c \
	$(ACPICA_COMP)/resources/rsmisc.c \
	$(ACPICA_COMP)/resources/rsutils.c \
	$(ACPICA_COMP)/resources/rsxface.c \
	$(ACPICA_COMP)/resources/rsserial.c \
	$(ACPICA_COMP)/tables/tbfadt.c \
	$(ACPICA_COMP)/tables/tbfind.c \
	$(ACPICA_COMP)/tables/tbinstal.c \
	$(ACPICA_COMP)/tables/tbutils.c \
	$(ACPICA_COMP)/tables/tbxface.c \
	$(ACPICA_COMP)/tables/tbxfroot.c \
	$(ACPICA_COMP)/utilities/utaddress.c \
	$(ACPICA_COMP)/utilities/utalloc.c \
	$(ACPICA_COMP)/utilities/utcache.c \
	$(ACPICA_COMP)/utilities/utcopy.c \
	$(ACPICA_COMP)/utilities/utdebug.c \
	$(ACPICA_COMP)/utilities/utdelete.c \
	$(ACPICA_COMP)/utilities/uteval.c \
	$(ACPICA_COMP)/utilities/utglobal.c \
	$(ACPICA_COMP)/utilities/utids.c \
	$(ACPICA_COMP)/utilities/utinit.c \
	$(ACPICA_COMP)/utilities/utlock.c \
	$(ACPICA_COMP)/utilities/utmath.c \
	$(ACPICA_COMP)/utilities/utmisc.c \
	$(ACPICA_COMP)/utilities/utmutex.c \
	$(ACPICA_COMP)/utilities/utobject.c \
	$(ACPICA_COMP)/utilities/utresrc.c \
	$(ACPICA_COMP)/utilities/utstate.c \
	$(ACPICA_COMP)/utilities/uttrack.c \
	$(ACPICA_COMP)/utilities/utosi.c \
	$(ACPICA_COMP)/utilities/utxferror.c \
	$(ACPICA_COMP)/utilities/utxface.c \
	$(ACPICA_COMP)/utilities/utdecode.c \
	$(ACPICA_SRC)/tools/acpiexec/aehandlers.c

libfwtsacpica_la_LIBADD = \
	-L$(top_srcdir)/src/lib/src -lfwts

