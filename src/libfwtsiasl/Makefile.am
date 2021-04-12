#
# Copyright (C) 2011-2021 Canonical, Ltd.
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

AM_CPPFLAGS = -Wall -Wstrict-prototypes			\
	      -fno-strict-aliasing 			\
	      -D_LINUX					\
	      -DACPI_ASL_COMPILER $(CFLAGS)		\
	      -I$(srcdir)/../acpica/source/include	\
	      -I$(srcdir)/../acpica/source/compiler

AM_YFLAGS = -d

#
# Case in-sensitive scanning
#
AM_LFLAGS = -i

ASL_PARSER = 							\
	$(srcdir)/../acpica/source/compiler/aslcstyle.y		\
	$(srcdir)/../acpica/source/compiler/aslhelpers.y	\
	$(srcdir)/../acpica/source/compiler/aslparser.y		\
	$(srcdir)/../acpica/source/compiler/aslprimaries.y	\
	$(srcdir)/../acpica/source/compiler/aslresources.y	\
	$(srcdir)/../acpica/source/compiler/aslrules.y		\
	$(srcdir)/../acpica/source/compiler/aslsupport.y	\
	$(srcdir)/../acpica/source/compiler/asltokens.y		\
	$(srcdir)/../acpica/source/compiler/asltypes.y

ASL_LEXER =
	$(srcdir)/../acpica/source/compiler/aslcompiler.l	\
	$(srcdir)/aslsupport.l					\
	$(srcdir)/aslcompiler.y.h

$(srcdir)/aslcompiler.y: $(ASL_PARSER)
	m4 -P -I$(srcdir)/../acpica/source/compiler $(srcdir)/../acpica/source/compiler/aslparser.y > $(srcdir)/aslcompiler.y

$(srcdir)/aslcompilerlex.c: $(ASL_LEXER)
	${LEX} ${AM_LFLAGS} -PAslCompiler -o$@ $(srcdir)/../acpica/source/compiler/aslcompiler.l

.NOTPARALLEL: $(srcdir)/aslcompiler.c
$(srcdir)/aslcompiler.c $(srcdir)/aslcompiler.y.h: $(srcdir)/aslcompiler.y
	${YACC} ${AM_YFLAGS} -d -baslcompiler -pAslCompiler $^
	mv aslcompiler.tab.c aslcompiler.c
	cp aslcompiler.tab.h aslcompiler.y.h

.NOTPARALLEL: $(srcdir)/dtcompilerparserlex.c
$(srcdir)/dtcompilerparserlex.c $(srcdir)/dtcompilerparser.c $(srcdir)/dtcompilerparser.y.h: $(srcdir)/../acpica/source/compiler/dtcompilerparser.l $(srcdir)/../acpica/source/compiler/dtcompilerparser.y
	${LEX} ${AM_LFLAGS} -PDtCompilerParser -o$(srcdir)/dtcompilerparserlex.c $<
	${YACC} ${AM_YFLAGS} -bdtcompilerparser -pDtCompilerParser $(srcdir)/../acpica/source/compiler/dtcompilerparser.y
	mv dtcompilerparser.tab.c dtcompilerparser.c
	cp dtcompilerparser.tab.h dtcompilerparser.y.h

.NOTPARALLEL: $(srcdir)/dtparserlex.c
$(srcdir)/dtparserlex.c $(srcdir)/dtparser.c $(srcdir)/dtparser.y.h: $(srcdir)/../acpica/source/compiler/dtparser.l $(srcdir)/../acpica/source/compiler/dtparser.y
	${LEX} ${AM_LFLAGS} -PDtParser -o$(srcdir)/dtparserlex.c $<
	${YACC} ${AM_YFLAGS} -bdtparser -pDtParser $(srcdir)/../acpica/source/compiler/dtparser.y
	mv dtparser.tab.c dtparser.c
	cp dtparser.tab.h dtparser.y.h

.NOTPARALLEL: $(srcdir)/prparserlex.c
$(srcdir)/prparserlex.c $(srcdir)/prparser.c $(srcdir)/prparser.y.h: $(srcdir)/../acpica/source/compiler/prparser.l $(srcdir)/../acpica/source/compiler/prparser.y
	${LEX} ${AM_LFLAGS} -PPrParser -o$(srcdir)/prparserlex.c $<
	${YACC} ${AM_YFLAGS} -bprparser -pPrParser $(srcdir)/../acpica/source/compiler/prparser.y
	mv prparser.tab.c prparser.c
	cp prparser.tab.h prparser.y.h

pkglib_LTLIBRARIES = libfwtsiasl.la

BUILT_SOURCES = aslcompiler.y		\
		aslcompiler.y.h		\
		aslcompilerlex.c	\
		aslcompiler.c		\
		dtcompilerparser.y.h	\
		dtcompilerparserlex.c 	\
		dtcompilerparser.c	\
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
libfwtsiasl_la_SOURCES = 						\
	fwts_iasl_interface.c 						\
	aslcompilerlex.c						\
	aslcompiler.c							\
	dtcompilerparserlex.c						\
	dtcompilerparser.c						\
	dtparserlex.c							\
	dtparser.c							\
	prparserlex.c							\
	prparser.c							\
	$(srcdir)/../acpica/source/common/adisasm.c			\
	$(srcdir)/../acpica/source/common/acfileio.c			\
	$(srcdir)/../acpica/source/common/adfile.c			\
	$(srcdir)/../acpica/source/common/adwalk.c			\
	$(srcdir)/../acpica/source/common/ahids.c			\
	$(srcdir)/../acpica/source/common/ahpredef.c			\
	$(srcdir)/../acpica/source/common/ahtable.c			\
	$(srcdir)/../acpica/source/common/ahuuids.c			\
	$(srcdir)/../acpica/source/compiler/aslallocate.c		\
	$(srcdir)/../acpica/source/compiler/aslanalyze.c		\
	$(srcdir)/../acpica/source/compiler/aslascii.c			\
	$(srcdir)/../acpica/source/compiler/aslbtypes.c			\
	$(srcdir)/../acpica/source/compiler/aslcache.c			\
	$(srcdir)/../acpica/source/compiler/aslcodegen.c		\
	$(srcdir)/../acpica/source/compiler/aslcompile.c		\
	$(srcdir)/../acpica/source/compiler/asldebug.c			\
	$(srcdir)/../acpica/source/compiler/aslerror.c			\
	$(srcdir)/../acpica/source/compiler/aslexternal.c		\
	$(srcdir)/../acpica/source/compiler/aslfiles.c			\
	$(srcdir)/../acpica/source/compiler/aslfileio.c			\
	$(srcdir)/../acpica/source/compiler/aslfold.c			\
	$(srcdir)/../acpica/source/compiler/aslhelp.c			\
	$(srcdir)/../acpica/source/compiler/aslhex.c			\
	$(srcdir)/../acpica/source/compiler/asllength.c			\
	$(srcdir)/../acpica/source/compiler/asllisting.c		\
	$(srcdir)/../acpica/source/compiler/asllistsup.c		\
	$(srcdir)/../acpica/source/compiler/aslload.c			\
	$(srcdir)/../acpica/source/compiler/asllookup.c			\
	$(srcdir)/../acpica/source/compiler/aslmain.c			\
	$(srcdir)/../acpica/source/compiler/aslmap.c			\
	$(srcdir)/../acpica/source/compiler/aslmapenter.c		\
	$(srcdir)/../acpica/source/compiler/aslmapoutput.c		\
	$(srcdir)/../acpica/source/compiler/aslmaputils.c		\
	$(srcdir)/../acpica/source/compiler/aslmessages.c		\
	$(srcdir)/../acpica/source/compiler/aslmethod.c			\
	$(srcdir)/../acpica/source/compiler/aslnamesp.c			\
	$(srcdir)/../acpica/source/compiler/asloffset.c			\
	$(srcdir)/../acpica/source/compiler/aslopcodes.c		\
	$(srcdir)/../acpica/source/compiler/asloperands.c		\
	$(srcdir)/../acpica/source/compiler/aslopt.c			\
	$(srcdir)/../acpica/source/compiler/asloptions.c		\
	$(srcdir)/../acpica/source/compiler/aslparseop.c		\
	$(srcdir)/../acpica/source/compiler/aslpredef.c			\
	$(srcdir)/../acpica/source/compiler/aslprepkg.c			\
	$(srcdir)/../acpica/source/compiler/aslprintf.c			\
	$(srcdir)/../acpica/source/compiler/aslprune.c			\
	$(srcdir)/../acpica/source/compiler/aslresource.c		\
	$(srcdir)/../acpica/source/compiler/aslrestype1.c		\
	$(srcdir)/../acpica/source/compiler/aslrestype1i.c		\
	$(srcdir)/../acpica/source/compiler/aslrestype2.c		\
	$(srcdir)/../acpica/source/compiler/aslrestype2d.c		\
	$(srcdir)/../acpica/source/compiler/aslrestype2e.c		\
	$(srcdir)/../acpica/source/compiler/aslrestype2q.c		\
	$(srcdir)/../acpica/source/compiler/aslrestype2s.c		\
	$(srcdir)/../acpica/source/compiler/aslrestype2w.c		\
	$(srcdir)/../acpica/source/compiler/aslstartup.c		\
	$(srcdir)/../acpica/source/compiler/aslstubs.c			\
	$(srcdir)/../acpica/source/compiler/aslpld.c			\
	$(srcdir)/../acpica/source/compiler/asltransform.c		\
	$(srcdir)/../acpica/source/compiler/asltree.c			\
	$(srcdir)/../acpica/source/compiler/aslutils.c			\
	$(srcdir)/../acpica/source/compiler/asluuid.c			\
	$(srcdir)/../acpica/source/compiler/aslwalks.c			\
	$(srcdir)/../acpica/source/compiler/aslxref.c			\
	$(srcdir)/../acpica/source/compiler/aslxrefout.c		\
	$(srcdir)/../acpica/source/compiler/cvcompiler.c		\
	$(srcdir)/../acpica/source/compiler/cvdisasm.c			\
	$(srcdir)/../acpica/source/compiler/cvparser.c			\
	$(srcdir)/../acpica/source/common/cmfsize.c			\
	$(srcdir)/../acpica/source/components/debugger/dbfileio.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmbuffer.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmcstyle.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmdeferred.c	\
	$(srcdir)/../acpica/source/common/dmextern.c			\
	$(srcdir)/../acpica/source/components/disassembler/dmnames.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmopcode.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmresrc.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmresrcl.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmresrcl2.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmresrcs.c	\
	$(srcdir)/../acpica/source/common/dmrestag.c			\
	$(srcdir)/../acpica/source/common/dmswitch.c			\
	$(srcdir)/../acpica/source/common/dmtable.c			\
	$(srcdir)/../acpica/source/common/dmtables.c			\
	$(srcdir)/../acpica/source/common/dmtbdump.c			\
	$(srcdir)/../acpica/source/common/dmtbdump1.c			\
	$(srcdir)/../acpica/source/common/dmtbdump2.c			\
	$(srcdir)/../acpica/source/common/dmtbdump3.c			\
	$(srcdir)/../acpica/source/common/dmtbinfo.c			\
	$(srcdir)/../acpica/source/common/dmtbinfo1.c			\
	$(srcdir)/../acpica/source/common/dmtbinfo2.c			\
	$(srcdir)/../acpica/source/common/dmtbinfo3.c			\
	$(srcdir)/../acpica/source/components/disassembler/dmutils.c	\
	$(srcdir)/../acpica/source/components/disassembler/dmwalk.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsargs.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dscontrol.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsfield.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsobject.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsopcode.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dspkginit.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dsutils.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dswexec.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dswload.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dswload2.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dswscope.c	\
	$(srcdir)/../acpica/source/components/dispatcher/dswstate.c	\
	$(srcdir)/../acpica/source/compiler/dtcompile.c			\
	$(srcdir)/../acpica/source/compiler/dtexpress.c			\
	$(srcdir)/../acpica/source/compiler/dtfield.c			\
	$(srcdir)/../acpica/source/compiler/dtio.c			\
	$(srcdir)/../acpica/source/compiler/dtsubtable.c		\
	$(srcdir)/../acpica/source/compiler/dttable.c			\
	$(srcdir)/../acpica/source/compiler/dttable1.c			\
	$(srcdir)/../acpica/source/compiler/dttable2.c			\
	$(srcdir)/../acpica/source/compiler/dttemplate.c		\
	$(srcdir)/../acpica/source/compiler/dtutils.c			\
	$(srcdir)/../acpica/source/components/executer/exconcat.c	\
	$(srcdir)/../acpica/source/components/executer/exconvrt.c	\
	$(srcdir)/../acpica/source/components/executer/excreate.c	\
	$(srcdir)/../acpica/source/components/executer/exdump.c		\
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
	$(srcdir)/../acpica/source/components/executer/exstore.c	\
	$(srcdir)/../acpica/source/components/executer/exstoren.c	\
	$(srcdir)/../acpica/source/components/executer/exstorob.c	\
	$(srcdir)/../acpica/source/components/executer/exsystem.c	\
	$(srcdir)/../acpica/source/components/executer/exutils.c	\
	$(srcdir)/../acpica/source/common/getopt.c			\
	$(srcdir)/../acpica/source/components/namespace/nsaccess.c	\
	$(srcdir)/../acpica/source/components/namespace/nsalloc.c	\
	$(srcdir)/../acpica/source/components/namespace/nsdump.c	\
	$(srcdir)/../acpica/source/components/namespace/nsnames.c	\
	$(srcdir)/../acpica/source/components/namespace/nsobject.c	\
	$(srcdir)/../acpica/source/components/namespace/nsparse.c	\
	$(srcdir)/../acpica/source/components/namespace/nssearch.c	\
	$(srcdir)/../acpica/source/components/namespace/nsutils.c	\
	$(srcdir)/../acpica/source/components/namespace/nswalk.c	\
	$(srcdir)/../acpica/source/components/namespace/nsxfobj.c	\
	$(srcdir)/../acpica/source/os_specific/service_layers/osunixxf.c\
	$(srcdir)/../acpica/source/compiler/prexpress.c			\
	$(srcdir)/../acpica/source/compiler/prmacros.c			\
	$(srcdir)/../acpica/source/compiler/prscan.c			\
	$(srcdir)/../acpica/source/compiler/prutils.c			\
	$(srcdir)/../acpica/source/components/parser/psargs.c		\
	$(srcdir)/../acpica/source/components/parser/psloop.c		\
	$(srcdir)/../acpica/source/components/parser/psobject.c		\
	$(srcdir)/../acpica/source/components/parser/psopcode.c		\
	$(srcdir)/../acpica/source/components/parser/psopinfo.c		\
	$(srcdir)/../acpica/source/components/parser/psparse.c		\
	$(srcdir)/../acpica/source/components/parser/psscope.c		\
	$(srcdir)/../acpica/source/components/parser/pstree.c		\
	$(srcdir)/../acpica/source/components/parser/psutils.c		\
	$(srcdir)/../acpica/source/components/parser/pswalk.c		\
	$(srcdir)/../acpica/source/components/tables/tbdata.c		\
	$(srcdir)/../acpica/source/components/tables/tbfadt.c		\
	$(srcdir)/../acpica/source/components/tables/tbinstal.c		\
	$(srcdir)/../acpica/source/components/tables/tbprint.c		\
	$(srcdir)/../acpica/source/components/tables/tbutils.c		\
	$(srcdir)/../acpica/source/components/tables/tbxface.c		\
	$(srcdir)/../acpica/source/components/tables/tbxfload.c		\
	$(srcdir)/../acpica/source/components/utilities/utaddress.c	\
	$(srcdir)/../acpica/source/components/utilities/utalloc.c	\
	$(srcdir)/../acpica/source/components/utilities/utascii.c	\
	$(srcdir)/../acpica/source/components/utilities/utbuffer.c	\
	$(srcdir)/../acpica/source/components/utilities/utcache.c	\
	$(srcdir)/../acpica/source/components/utilities/utcopy.c	\
	$(srcdir)/../acpica/source/components/utilities/utdebug.c	\
	$(srcdir)/../acpica/source/components/utilities/utdecode.c	\
	$(srcdir)/../acpica/source/components/utilities/utdelete.c	\
	$(srcdir)/../acpica/source/components/utilities/uterror.c	\
	$(srcdir)/../acpica/source/components/utilities/utexcep.c	\
	$(srcdir)/../acpica/source/components/utilities/utglobal.c	\
	$(srcdir)/../acpica/source/components/utilities/uthex.c		\
	$(srcdir)/../acpica/source/components/utilities/utinit.c	\
	$(srcdir)/../acpica/source/components/utilities/utlock.c	\
	$(srcdir)/../acpica/source/components/utilities/utmath.c	\
	$(srcdir)/../acpica/source/components/utilities/utmisc.c	\
	$(srcdir)/../acpica/source/components/utilities/utmutex.c	\
	$(srcdir)/../acpica/source/components/utilities/utnonansi.c	\
	$(srcdir)/../acpica/source/components/utilities/utobject.c	\
	$(srcdir)/../acpica/source/components/utilities/utownerid.c	\
	$(srcdir)/../acpica/source/components/utilities/utpredef.c	\
	$(srcdir)/../acpica/source/components/utilities/utresdecode.c	\
	$(srcdir)/../acpica/source/components/utilities/utresrc.c	\
	$(srcdir)/../acpica/source/components/utilities/utstate.c	\
	$(srcdir)/../acpica/source/components/utilities/utstrtoul64.c	\
	$(srcdir)/../acpica/source/components/utilities/utstrsuppt.c	\
	$(srcdir)/../acpica/source/components/utilities/utstring.c	\
	$(srcdir)/../acpica/source/components/utilities/utuuid.c	\
	$(srcdir)/../acpica/source/components/utilities/utxface.c	\
	$(srcdir)/../acpica/source/components/utilities/utxferror.c

-include $(top_srcdir)/git.mk
