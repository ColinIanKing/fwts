#!/bin/bash

#
# Hacky script to fetch latest ACPICA core and upgrade fwts
#
FWTS=.

echo "Removing old ACPICA.."
if [ -e acpica ]; then
	mv acpica acpica.old
fi
echo "Cloning new ACPICA.."
git clone git://github.com/acpica/acpica

FWTS_ACPICA_PATH=${FWTS}/src/acpica/source 
SAVE_FILES=acpica-upgrade-save

files="include/acpi.h \
       include/platform/acenv.h \
       include/platform/aclinux.h \
       include/platform/acgcc.h \
       include/acnames.h \
       include/actypes.h \
       include/acexcep.h \
       include/actbl.h \
       include/actbl1.h \
       include/actbl2.h \
       include/actbl3.h \
       include/acoutput.h \
       include/acrestyp.h \
       include/acpiosxf.h \
       include/acpixf.h \
       include/accommon.h \
       include/acconfig.h \
       include/acmacros.h \
       include/aclocal.h \
       include/acobject.h \
       include/acstruct.h \
       include/acglobal.h \
       include/achware.h \
       include/acutils.h \
       include/acparser.h \
       include/amlcode.h \
       include/acnamesp.h \
       include/acdebug.h \
       include/actables.h \
       include/acinterp.h \
       include/acapps.h \
       include/acdispat.h \
       include/acevents.h \
       include/acresrc.h \
       include/amlresrc.h \
       include/acdisasm.h \
       include/acpredef.h \
       include/acopcode.h \
       components/debugger/dbcmds.c \
       components/debugger/dbdisply.c \
       components/debugger/dbexec.c \
       components/debugger/dbfileio.c \
       components/debugger/dbhistry.c \
       components/debugger/dbinput.c \
       components/debugger/dbstats.c \
       components/debugger/dbutils.c \
       components/debugger/dbxface.c \
       components/debugger/dbmethod.c \
       components/debugger/dbnames.c \
       components/disassembler/dmbuffer.c \
       components/disassembler/dmnames.c \
       components/disassembler/dmobject.c \
       components/disassembler/dmopcode.c \
       components/disassembler/dmresrc.c \
       components/disassembler/dmresrcl.c \
       components/disassembler/dmresrcs.c \
       components/disassembler/dmutils.c \
       components/disassembler/dmwalk.c \
       components/disassembler/dmresrcl2.c \
       components/dispatcher/dsfield.c \
       components/dispatcher/dsinit.c \
       components/dispatcher/dsmethod.c \
       components/dispatcher/dsmthdat.c \
       components/dispatcher/dsobject.c \
       components/dispatcher/dsopcode.c \
       components/dispatcher/dsutils.c \
       components/dispatcher/dswexec.c \
       components/dispatcher/dswload.c \
       components/dispatcher/dswscope.c \
       components/dispatcher/dswstate.c \
       components/dispatcher/dscontrol.c \
       components/dispatcher/dsargs.c \
       components/dispatcher/dswload2.c \
       components/events/evevent.c \
       components/events/evgpe.c \
       components/events/evgpeblk.c \
       components/events/evgpeinit.c \
       components/events/evgpeutil.c \
       components/events/evmisc.c \
       components/events/evregion.c \
       components/events/evrgnini.c \
       components/events/evsci.c \
       components/events/evxface.c \
       components/events/evxfevnt.c \
       components/events/evxfgpe.c \
       components/events/evxfregn.c \
       components/events/evglock.c \
       components/executer/exfield.c \
       components/executer/exfldio.c \
       components/executer/exmisc.c \
       components/executer/exmutex.c \
       components/executer/exnames.c \
       components/executer/exoparg1.c \
       components/executer/exoparg2.c \
       components/executer/exoparg3.c \
       components/executer/exoparg6.c \
       components/executer/exprep.c \
       components/executer/exregion.c \
       components/executer/exresnte.c \
       components/executer/exresolv.c \
       components/executer/exresop.c \
       components/executer/exstore.c \
       components/executer/exstoren.c \
       components/executer/exstorob.c \
       components/executer/exsystem.c \
       components/executer/exutils.c \
       components/executer/exconvrt.c \
       components/executer/excreate.c \
       components/executer/exdump.c \
       components/executer/exdebug.c \
       components/executer/exconfig.c \
       components/hardware/hwacpi.c \
       components/hardware/hwgpe.c \
       components/hardware/hwpci.c \
       components/hardware/hwregs.c \
       components/hardware/hwsleep.c \
       components/hardware/hwvalid.c \
       components/hardware/hwxface.c \
       components/hardware/hwxfsleep.c \
       components/hardware/hwesleep.c \
       components/namespace/nsaccess.c \
       components/namespace/nsalloc.c \
       components/namespace/nsdump.c \
       components/namespace/nsdumpdv.c \
       components/namespace/nseval.c \
       components/namespace/nsinit.c \
       components/namespace/nsload.c \
       components/namespace/nsnames.c \
       components/namespace/nsobject.c \
       components/namespace/nsparse.c \
       components/namespace/nspredef.c \
       components/namespace/nsrepair.c \
       components/namespace/nsrepair2.c \
       components/namespace/nssearch.c \
       components/namespace/nsutils.c \
       components/namespace/nswalk.c \
       components/namespace/nsxfeval.c \
       components/namespace/nsxfname.c \
       components/namespace/nsxfobj.c \
       components/parser/psargs.c \
       components/parser/psloop.c \
       components/parser/psopcode.c \
       components/parser/psparse.c \
       components/parser/psscope.c \
       components/parser/pstree.c \
       components/parser/psutils.c \
       components/parser/pswalk.c \
       components/parser/psxface.c \
       components/resources/rsaddr.c \
       components/resources/rscalc.c \
       components/resources/rscreate.c \
       components/resources/rsdump.c \
       components/resources/rsio.c \
       components/resources/rsinfo.c \
       components/resources/rsirq.c \
       components/resources/rslist.c \
       components/resources/rsmemory.c \
       components/resources/rsmisc.c \
       components/resources/rsutils.c \
       components/resources/rsxface.c \
       components/resources/rsserial.c \
       components/tables/tbfadt.c \
       components/tables/tbfind.c \
       components/tables/tbinstal.c \
       components/tables/tbutils.c \
       components/tables/tbxface.c \
       components/tables/tbxfroot.c \
       components/tables/tbxfload.c \
       components/utilities/utaddress.c \
       components/utilities/utalloc.c \
       components/utilities/utcache.c \
       components/utilities/utcopy.c \
       components/utilities/utdebug.c \
       components/utilities/utdelete.c \
       components/utilities/uteval.c \
       components/utilities/utglobal.c \
       components/utilities/utids.c \
       components/utilities/utinit.c \
       components/utilities/utlock.c \
       components/utilities/utmath.c \
       components/utilities/utmisc.c \
       components/utilities/utmutex.c \
       components/utilities/utobject.c \
       components/utilities/utresrc.c \
       components/utilities/utstate.c \
       components/utilities/uttrack.c \
       components/utilities/utosi.c \
       components/utilities/utxferror.c \
       components/utilities/utxface.c \
       components/utilities/utdecode.c \
       components/utilities/utexcep.c \
       tools/acpiexec/aehandlers.c \
       tools/acpiexec/aecommon.h \
       compiler/aslanalyze.c \
       compiler/aslcodegen.c \
       compiler/aslcompile.c \
       compiler/aslcompiler.h \
       compiler/aslcompiler.l \
       compiler/aslcompiler.y \
       compiler/aslsupport.l \
       compiler/asldefine.h \
       compiler/aslerror.c \
       compiler/aslfiles.c \
       compiler/aslfold.c \
       compiler/aslglobal.h \
       compiler/asllength.c \
       compiler/asllisting.c \
       compiler/aslload.c \
       compiler/asllookup.c \
       compiler/aslmain.c \
       compiler/aslmap.c \
       compiler/aslmessages.h \
       compiler/aslopcodes.c \
       compiler/asloperands.c \
       compiler/aslopt.c \
       compiler/aslpredef.c \
       compiler/aslresource.c \
       compiler/aslrestype1.c \
       compiler/aslrestype1i.c \
       compiler/aslrestype2.c \
       compiler/aslrestype2d.c \
       compiler/aslrestype2e.c \
       compiler/aslrestype2q.c \
       compiler/aslrestype2w.c \
       compiler/aslstartup.c \
       compiler/aslstubs.c \
       compiler/asltransform.c \
       compiler/asltree.c \
       compiler/asltypes.h \
       compiler/aslutils.c \
       compiler/dtcompile.c \
       compiler/dtcompiler.h \
       compiler/dtfield.c \
       compiler/dtio.c \
       compiler/dtsubtable.c \
       compiler/dttable.c \
       compiler/dttemplate.c \
       compiler/dttemplate.h \
       compiler/dtutils.c \
       compiler/dtexpress.c \
       compiler/dtparser.y \
       compiler/dtparser.l \
       compiler/aslbtypes.c \
       compiler/aslwalks.c \
       compiler/asluuid.c \
       compiler/preprocess.h \
       compiler/prscan.c \
       compiler/prmacros.c \
       compiler/prutils.c \
       compiler/prexpress.c \
       compiler/prparser.y \
       compiler/prparser.l \
       compiler/aslrestype2s.c \
       common/adfile.c \
       common/adisasm.c \
       common/adwalk.c \
       common/ahpredef.c \
       common/dmextern.c \
       common/dmrestag.c \
       common/dmtable.c \
       common/dmtbinfo.c \
       common/dmtbdump.c \
       os_specific/service_layers/osunixxf.c"

echo "Saving makefiles etc...."
rm -rf ${SAVE_FILES}
mkdir ${SAVE_FILES}
cp ${FWTS_ACPICA_PATH}/compiler/Makefile* ${SAVE_FILES}
cp ${FWTS_ACPICA_PATH}/compiler/fwts_iasl_interface.c ${SAVE_FILES}
cp ${FWTS_ACPICA_PATH}/compiler/fwts_iasl_interface.h ${SAVE_FILES}

echo "Removing old ACPICA sources"
rm -rf ${FWTS_ACPICA_PATH}
mkdir ${FWTS_ACPICA_PATH} 

echo "Copying new ACPICA sources"
for i in ${files}
do
	mkdir -p `dirname ${FWTS_ACPICA_PATH}/$i`
	cp -p acpica/source/$i ${FWTS}/src/acpica/source/$i
done

cp ${SAVE_FILES}/* ${FWTS}/src/acpica/source/compiler
