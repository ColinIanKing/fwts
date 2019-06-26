#
# Copyright (C) 2011-2019 Canonical, Ltd.
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

#
# We need to build this as a separate shared library with the iasl functionality built in. We
# just export the fwts_* interface.
#
AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -Wstrict-prototypes		\
	      -fno-strict-aliasing 		\
	      -D_LINUX				\
	      -DACPI_ASL_COMPILER $(CFLAGS)	\
	      -I../acpica/source/include	\
	      -I../acpica/source/compiler

AM_YFLAGS = -d

#
# Case in-sensitive scanning
#
AM_LFLAGS = -i

ASL_PARSER = 						\
	../acpica/source/compiler/aslcstyle.y		\
	../acpica/source/compiler/aslhelpers.y		\
	../acpica/source/compiler/aslparser.y		\
	../acpica/source/compiler/aslprimaries.y	\
	../acpica/source/compiler/aslresources.y	\
	../acpica/source/compiler/aslrules.y		\
	../acpica/source/compiler/aslsupport.y		\
	../acpica/source/compiler/asltokens.y		\
	../acpica/source/compiler/asltypes.y

ASL_LEXER =
	../acpica/source/compiler/aslcompiler.l	\
	$(srcdir)/aslsupport.l			\
	$(srcdir)/aslcompiler.y.h

$(srcdir)/aslcompiler.y: $(ASL_PARSER)
	m4 -P -I../acpica/source/compiler ../acpica/source/compiler/aslparser.y > $@

$(srcdir)/aslcompilerlex.c: $(ASL_LEXER)
	${LEX} ${AM_LFLAGS} -PAslCompiler -o$@ ../acpica/source/compiler/aslcompiler.l

.NOTPARALLEL: $(srcdir)/aslcompiler.c
$(srcdir)/aslcompiler.c $(srcdir)/aslcompiler.y.h: $(srcdir)/aslcompiler.y
	${YACC} ${AM_YFLAGS} -d -baslcompiler -pAslCompiler $^
	mv aslcompiler.tab.c aslcompiler.c
	mv aslcompiler.tab.h aslcompiler.y.h

.NOTPARALLEL: $(srcdir)/dtparserlex.c
$(srcdir)/dtparserlex.c $(srcdir)/dtparser.c $(srcdir)/dtparser.y.h: ../acpica/source/compiler/dtparser.l ../acpica/source/compiler/dtparser.y
	${LEX} ${AM_LFLAGS} -PDtParser -o$(srcdir)/dtparserlex.c $<
	${YACC} ${AM_YFLAGS} -bdtparser -pDtParser ../acpica/source/compiler/dtparser.y
	mv dtparser.tab.c dtparser.c
	mv dtparser.tab.h dtparser.y.h

.NOTPARALLEL: $(srcdir)/prparserlex.c
$(srcdir)/prparserlex.c $(srcdir)/prparser.c $(srcdir)/prparser.y.h: ../acpica/source/compiler/prparser.l ../acpica/source/compiler/prparser.y
	${LEX} ${AM_LFLAGS} -PPrParser -o$(srcdir)/prparserlex.c $<
	${YACC} ${AM_YFLAGS} -bprparser -pPrParser ../acpica/source/compiler/prparser.y
	mv prparser.tab.c prparser.c
	mv prparser.tab.h prparser.y.h

pkglib_LTLIBRARIES = libfwtsiasl.la

BUILT_SOURCES = aslcompiler.y		\
		aslcompiler.y.h		\
		aslcompilerlex.c	\
		aslcompiler.c		\
		dtparser.y.h		\
		dtparserlex.c 		\
		dtparser.c 		\
		prparser.y.h		\
		prparserlex.c		\
		prparser.c

#
# Just export fwts specific API so we don't clash with core ACPICA library
#
libfwtsiasl_la_LDFLAGS = -export-symbols-regex "fwts_.*" -lpthread -version-info 1:0:0

CLEANFILES = $(BUILT_SOURCES)

libfwtsiasl_la_CPPFLAGS = $(AM_CPPFLAGS)
libfwtsiasl_la_SOURCES = 					\
	fwts_iasl_interface.c 					\
	aslcompilerlex.c					\
	aslcompiler.c						\
	dtparserlex.c						\
	dtparser.c						\
	prparserlex.c						\
	prparser.c						\
	../acpica/source/common/adisasm.c			\
	../acpica/source/common/acfileio.c			\
	../acpica/source/common/adfile.c			\
	../acpica/source/common/adwalk.c			\
	../acpica/source/common/ahids.c				\
	../acpica/source/common/ahpredef.c			\
	../acpica/source/common/ahtable.c			\
	../acpica/source/common/ahuuids.c			\
	../acpica/source/compiler/aslallocate.c			\
	../acpica/source/compiler/aslanalyze.c			\
	../acpica/source/compiler/aslascii.c			\
	../acpica/source/compiler/aslbtypes.c			\
	../acpica/source/compiler/aslcache.c			\
	../acpica/source/compiler/aslcodegen.c			\
	../acpica/source/compiler/aslcompile.c			\
	../acpica/source/compiler/asldebug.c			\
	../acpica/source/compiler/aslerror.c			\
	../acpica/source/compiler/aslexternal.c			\
	../acpica/source/compiler/aslfiles.c			\
	../acpica/source/compiler/aslfileio.c			\
	../acpica/source/compiler/aslfold.c			\
	../acpica/source/compiler/aslhelp.c			\
	../acpica/source/compiler/aslhex.c			\
	../acpica/source/compiler/asllength.c			\
	../acpica/source/compiler/asllisting.c			\
	../acpica/source/compiler/asllistsup.c			\
	../acpica/source/compiler/aslload.c			\
	../acpica/source/compiler/asllookup.c			\
	../acpica/source/compiler/aslmain.c			\
	../acpica/source/compiler/aslmap.c			\
	../acpica/source/compiler/aslmapenter.c			\
	../acpica/source/compiler/aslmapoutput.c		\
	../acpica/source/compiler/aslmaputils.c			\
	../acpica/source/compiler/aslmessages.c			\
	../acpica/source/compiler/aslmethod.c			\
	../acpica/source/compiler/aslnamesp.c			\
	../acpica/source/compiler/asloffset.c			\
	../acpica/source/compiler/aslopcodes.c			\
	../acpica/source/compiler/asloperands.c			\
	../acpica/source/compiler/aslopt.c			\
	../acpica/source/compiler/asloptions.c			\
	../acpica/source/compiler/aslparseop.c			\
	../acpica/source/compiler/aslpredef.c			\
	../acpica/source/compiler/aslprepkg.c			\
	../acpica/source/compiler/aslprintf.c			\
	../acpica/source/compiler/aslprune.c			\
	../acpica/source/compiler/aslresource.c			\
	../acpica/source/compiler/aslrestype1.c			\
	../acpica/source/compiler/aslrestype1i.c		\
	../acpica/source/compiler/aslrestype2.c			\
	../acpica/source/compiler/aslrestype2d.c		\
	../acpica/source/compiler/aslrestype2e.c		\
	../acpica/source/compiler/aslrestype2q.c		\
	../acpica/source/compiler/aslrestype2s.c		\
	../acpica/source/compiler/aslrestype2w.c		\
	../acpica/source/compiler/aslstartup.c			\
	../acpica/source/compiler/aslstubs.c			\
	../acpica/source/compiler/aslpld.c			\
	../acpica/source/compiler/asltransform.c		\
	../acpica/source/compiler/asltree.c			\
	../acpica/source/compiler/aslutils.c			\
	../acpica/source/compiler/asluuid.c			\
	../acpica/source/compiler/aslwalks.c			\
	../acpica/source/compiler/aslxref.c			\
	../acpica/source/compiler/aslxrefout.c			\
	../acpica/source/compiler/cvcompiler.c			\
	../acpica/source/compiler/cvdisasm.c			\
	../acpica/source/compiler/cvparser.c			\
	../acpica/source/common/cmfsize.c			\
	../acpica/source/components/debugger/dbfileio.c		\
	../acpica/source/components/disassembler/dmbuffer.c	\
	../acpica/source/components/disassembler/dmcstyle.c	\
	../acpica/source/components/disassembler/dmdeferred.c	\
	../acpica/source/common/dmextern.c			\
	../acpica/source/components/disassembler/dmnames.c	\
	../acpica/source/components/disassembler/dmopcode.c	\
	../acpica/source/components/disassembler/dmresrc.c	\
	../acpica/source/components/disassembler/dmresrcl.c	\
	../acpica/source/components/disassembler/dmresrcl2.c	\
	../acpica/source/components/disassembler/dmresrcs.c	\
	../acpica/source/common/dmrestag.c			\
	../acpica/source/common/dmswitch.c			\
	../acpica/source/common/dmtable.c			\
	../acpica/source/common/dmtables.c			\
	../acpica/source/common/dmtbdump.c			\
	../acpica/source/common/dmtbdump1.c			\
	../acpica/source/common/dmtbdump2.c			\
	../acpica/source/common/dmtbdump3.c			\
	../acpica/source/common/dmtbinfo.c			\
	../acpica/source/common/dmtbinfo1.c			\
	../acpica/source/common/dmtbinfo2.c			\
	../acpica/source/common/dmtbinfo3.c			\
	../acpica/source/components/disassembler/dmutils.c	\
	../acpica/source/components/disassembler/dmwalk.c	\
	../acpica/source/components/dispatcher/dsargs.c		\
	../acpica/source/components/dispatcher/dscontrol.c	\
	../acpica/source/components/dispatcher/dsfield.c	\
	../acpica/source/components/dispatcher/dsobject.c	\
	../acpica/source/components/dispatcher/dsopcode.c	\
	../acpica/source/components/dispatcher/dspkginit.c	\
	../acpica/source/components/dispatcher/dsutils.c	\
	../acpica/source/components/dispatcher/dswexec.c	\
	../acpica/source/components/dispatcher/dswload.c	\
	../acpica/source/components/dispatcher/dswload2.c	\
	../acpica/source/components/dispatcher/dswscope.c	\
	../acpica/source/components/dispatcher/dswstate.c	\
	../acpica/source/compiler/dtcompile.c			\
	../acpica/source/compiler/dtexpress.c			\
	../acpica/source/compiler/dtfield.c			\
	../acpica/source/compiler/dtio.c			\
	../acpica/source/compiler/dtsubtable.c			\
	../acpica/source/compiler/dttable.c			\
	../acpica/source/compiler/dttable1.c			\
	../acpica/source/compiler/dttable2.c			\
	../acpica/source/compiler/dttemplate.c			\
	../acpica/source/compiler/dtutils.c			\
	../acpica/source/components/executer/exconcat.c		\
	../acpica/source/components/executer/exconvrt.c		\
	../acpica/source/components/executer/excreate.c		\
	../acpica/source/components/executer/exdump.c		\
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
	../acpica/source/components/executer/exstore.c		\
	../acpica/source/components/executer/exstoren.c		\
	../acpica/source/components/executer/exstorob.c		\
	../acpica/source/components/executer/exsystem.c		\
	../acpica/source/components/executer/exutils.c		\
	../acpica/source/common/getopt.c			\
	../acpica/source/components/namespace/nsaccess.c	\
	../acpica/source/components/namespace/nsalloc.c		\
	../acpica/source/components/namespace/nsdump.c		\
	../acpica/source/components/namespace/nsnames.c		\
	../acpica/source/components/namespace/nsobject.c	\
	../acpica/source/components/namespace/nsparse.c		\
	../acpica/source/components/namespace/nssearch.c	\
	../acpica/source/components/namespace/nsutils.c		\
	../acpica/source/components/namespace/nswalk.c		\
	../acpica/source/components/namespace/nsxfobj.c		\
	../acpica/source/os_specific/service_layers/osunixxf.c	\
	../acpica/source/compiler/prexpress.c			\
	../acpica/source/compiler/prmacros.c			\
	../acpica/source/compiler/prscan.c			\
	../acpica/source/compiler/prutils.c			\
	../acpica/source/components/parser/psargs.c		\
	../acpica/source/components/parser/psloop.c		\
	../acpica/source/components/parser/psobject.c		\
	../acpica/source/components/parser/psopcode.c		\
	../acpica/source/components/parser/psopinfo.c		\
	../acpica/source/components/parser/psparse.c		\
	../acpica/source/components/parser/psscope.c		\
	../acpica/source/components/parser/pstree.c		\
	../acpica/source/components/parser/psutils.c		\
	../acpica/source/components/parser/pswalk.c		\
	../acpica/source/components/tables/tbdata.c		\
	../acpica/source/components/tables/tbfadt.c		\
	../acpica/source/components/tables/tbinstal.c		\
	../acpica/source/components/tables/tbprint.c		\
	../acpica/source/components/tables/tbutils.c		\
	../acpica/source/components/tables/tbxface.c		\
	../acpica/source/components/tables/tbxfload.c		\
	../acpica/source/components/utilities/utaddress.c	\
	../acpica/source/components/utilities/utalloc.c		\
	../acpica/source/components/utilities/utascii.c		\
	../acpica/source/components/utilities/utbuffer.c	\
	../acpica/source/components/utilities/utcache.c		\
	../acpica/source/components/utilities/utcopy.c		\
	../acpica/source/components/utilities/utdebug.c		\
	../acpica/source/components/utilities/utdecode.c	\
	../acpica/source/components/utilities/utdelete.c	\
	../acpica/source/components/utilities/uterror.c		\
	../acpica/source/components/utilities/utexcep.c		\
	../acpica/source/components/utilities/utglobal.c	\
	../acpica/source/components/utilities/uthex.c		\
	../acpica/source/components/utilities/utinit.c		\
	../acpica/source/components/utilities/utlock.c		\
	../acpica/source/components/utilities/utmath.c		\
	../acpica/source/components/utilities/utmisc.c		\
	../acpica/source/components/utilities/utmutex.c		\
	../acpica/source/components/utilities/utnonansi.c	\
	../acpica/source/components/utilities/utobject.c	\
	../acpica/source/components/utilities/utownerid.c	\
	../acpica/source/components/utilities/utpredef.c	\
	../acpica/source/components/utilities/utresdecode.c	\
	../acpica/source/components/utilities/utresrc.c		\
	../acpica/source/components/utilities/utstate.c		\
	../acpica/source/components/utilities/utstrtoul64.c	\
	../acpica/source/components/utilities/utstrsuppt.c	\
	../acpica/source/components/utilities/utstring.c	\
	../acpica/source/components/utilities/utuuid.c		\
	../acpica/source/components/utilities/utxface.c		\
	../acpica/source/components/utilities/utxferror.c
