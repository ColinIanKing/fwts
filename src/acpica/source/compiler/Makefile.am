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

AM_CPPFLAGS = -Wall -Wstrict-prototypes -fno-strict-aliasing -D_LINUX \
	      -DACPI_ASL_COMPILER $(CFLAGS) -I$(top_srcdir)/src/acpica/source/include

AM_YFLAGS = -d

#
# Case in-sensitive scanning
#
AM_LFLAGS = -i

ASL_PARSER = 				\
	$(srcdir)/aslcstyle.y		\
	$(srcdir)/aslhelpers.y		\
	$(srcdir)/aslparser.y		\
	$(srcdir)/aslprimaries.y	\
	$(srcdir)/aslresources.y	\
	$(srcdir)/aslrules.y		\
	$(srcdir)/aslsupport.y		\
	$(srcdir)/asltokens.y		\
	$(srcdir)/asltypes.y

ASL_LEXER =
	$(srcdir)/aslcompiler.l		\
	$(srcdir)/aslsupport.l		\
	$(srcdir)/aslcompiler.y.h

$(srcdir)/aslcompiler.y: $(ASL_PARSER)
	m4 -P -I$(srcdir) $(srcdir)/aslparser.y > $@

$(srcdir)/aslcompilerlex.c: $(ASL_LEXER)
	${LEX} ${AM_LFLAGS} -PAslCompiler -o$@ $(srcdir)/aslcompiler.l

.NOTPARALLEL: $(srcdir)/aslcompiler.c
$(srcdir)/aslcompiler.c $(srcdir)/aslcompiler.y.h: $(srcdir)/aslcompiler.y
	${YACC} ${AM_YFLAGS} -d -baslcompiler -pAslCompiler $^
	mv aslcompiler.tab.c aslcompiler.c
	mv aslcompiler.tab.h aslcompiler.y.h

.NOTPARALLEL: $(srcdir)/dtparserlex.c
$(srcdir)/dtparserlex.c $(srcdir)/dtparser.c $(srcdir)/dtparser.y.h: $(srcdir)/dtparser.l $(srcdir)/dtparser.y
	${LEX} ${AM_LFLAGS} -PDtParser -o$(srcdir)/dtparserlex.c $<
	${YACC} ${AM_YFLAGS} -bdtparser -pDtParser $(srcdir)/dtparser.y
	mv dtparser.tab.c dtparser.c
	mv dtparser.tab.h dtparser.y.h

.NOTPARALLEL: $(srcdir)/prparserlex.c
$(srcdir)/prparserlex.c $(srcdir)/prparser.c $(srcdir)/prparser.y.h: $(srcdir)/prparser.l $(srcdir)/prparser.y
	${LEX} ${AM_LFLAGS} -PPrParser -o$(srcdir)/prparserlex.c $<
	${YACC} ${AM_YFLAGS} -bprparser -pPrParser $(srcdir)/prparser.y
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
libfwtsiasl_la_SOURCES = 			\
	fwts_iasl_interface.c 			\
	aslcompilerlex.c			\
	aslcompiler.c				\
	dtparserlex.c				\
	dtparser.c				\
	prparserlex.c				\
	prparser.c				\
	../common/adisasm.c			\
	../common/acfileio.c			\
	../common/adfile.c			\
	../common/adwalk.c			\
	../common/ahids.c			\
	../common/ahpredef.c			\
	../common/ahtable.c			\
	../common/ahuuids.c			\
	../compiler/aslallocate.c		\
	../compiler/aslanalyze.c		\
	../compiler/aslascii.c			\
	../compiler/aslbtypes.c			\
	../compiler/aslcache.c			\
	../compiler/aslcodegen.c		\
	../compiler/aslcompile.c		\
	../compiler/asldebug.c			\
	../compiler/aslerror.c			\
	../compiler/aslexternal.c		\
	../compiler/aslfiles.c			\
	../compiler/aslfileio.c			\
	../compiler/aslfold.c			\
	../compiler/aslhelp.c			\
	../compiler/aslhex.c			\
	../compiler/asllength.c			\
	../compiler/asllisting.c		\
	../compiler/asllistsup.c		\
	../compiler/aslload.c			\
	../compiler/asllookup.c			\
	../compiler/aslmain.c			\
	../compiler/aslmap.c			\
	../compiler/aslmapenter.c		\
	../compiler/aslmapoutput.c		\
	../compiler/aslmaputils.c		\
	../compiler/aslmessages.c		\
	../compiler/aslmethod.c			\
	../compiler/aslnamesp.c			\
	../compiler/asloffset.c			\
	../compiler/aslopcodes.c		\
	../compiler/asloperands.c		\
	../compiler/aslopt.c			\
	../compiler/asloptions.c		\
	../compiler/aslparseop.c		\
	../compiler/aslpredef.c			\
	../compiler/aslprepkg.c			\
	../compiler/aslprintf.c			\
	../compiler/aslprune.c			\
	../compiler/aslresource.c		\
	../compiler/aslrestype1.c		\
	../compiler/aslrestype1i.c		\
	../compiler/aslrestype2.c		\
	../compiler/aslrestype2d.c		\
	../compiler/aslrestype2e.c		\
	../compiler/aslrestype2q.c		\
	../compiler/aslrestype2s.c		\
	../compiler/aslrestype2w.c		\
	../compiler/aslstartup.c		\
	../compiler/aslstubs.c			\
	../compiler/aslpld.c			\
	../compiler/asltransform.c		\
	../compiler/asltree.c			\
	../compiler/aslutils.c			\
	../compiler/asluuid.c			\
	../compiler/aslwalks.c			\
	../compiler/aslxref.c			\
	../compiler/aslxrefout.c		\
	../compiler/cvcompiler.c		\
	../compiler/cvdisasm.c			\
	../compiler/cvparser.c			\
	../common/cmfsize.c			\
	../components/debugger/dbfileio.c	\
	../components/disassembler/dmbuffer.c	\
	../components/disassembler/dmcstyle.c	\
	../components/disassembler/dmdeferred.c	\
	../common/dmextern.c			\
	../components/disassembler/dmnames.c	\
	../components/disassembler/dmopcode.c	\
	../components/disassembler/dmresrc.c	\
	../components/disassembler/dmresrcl.c	\
	../components/disassembler/dmresrcl2.c	\
	../components/disassembler/dmresrcs.c	\
	../common/dmrestag.c			\
	../common/dmswitch.c			\
	../common/dmtable.c			\
	../common/dmtables.c			\
	../common/dmtbdump.c			\
	../common/dmtbdump1.c			\
	../common/dmtbdump2.c			\
	../common/dmtbdump3.c			\
	../common/dmtbinfo.c			\
	../common/dmtbinfo1.c			\
	../common/dmtbinfo2.c			\
	../common/dmtbinfo3.c			\
	../components/disassembler/dmutils.c	\
	../components/disassembler/dmwalk.c	\
	../components/dispatcher/dsargs.c	\
	../components/dispatcher/dscontrol.c	\
	../components/dispatcher/dsfield.c	\
	../components/dispatcher/dsobject.c	\
	../components/dispatcher/dsopcode.c	\
	../components/dispatcher/dspkginit.c	\
	../components/dispatcher/dsutils.c	\
	../components/dispatcher/dswexec.c	\
	../components/dispatcher/dswload.c	\
	../components/dispatcher/dswload2.c	\
	../components/dispatcher/dswscope.c	\
	../components/dispatcher/dswstate.c	\
	../compiler/dtcompile.c			\
	../compiler/dtexpress.c			\
	../compiler/dtfield.c			\
	../compiler/dtio.c			\
	../compiler/dtsubtable.c		\
	../compiler/dttable.c			\
	../compiler/dttable1.c			\
	../compiler/dttable2.c			\
	../compiler/dttemplate.c		\
	../compiler/dtutils.c			\
	../components/executer/exconcat.c	\
	../components/executer/exconvrt.c	\
	../components/executer/excreate.c	\
	../components/executer/exdump.c		\
	../components/executer/exmisc.c		\
	../components/executer/exmutex.c	\
	../components/executer/exnames.c	\
	../components/executer/exoparg1.c	\
	../components/executer/exoparg2.c	\
	../components/executer/exoparg3.c	\
	../components/executer/exoparg6.c	\
	../components/executer/exprep.c		\
	../components/executer/exregion.c	\
	../components/executer/exresnte.c	\
	../components/executer/exresolv.c	\
	../components/executer/exresop.c	\
	../components/executer/exstore.c	\
	../components/executer/exstoren.c	\
	../components/executer/exstorob.c	\
	../components/executer/exsystem.c	\
	../components/executer/exutils.c	\
	../common/getopt.c			\
	../components/namespace/nsaccess.c	\
	../components/namespace/nsalloc.c	\
	../components/namespace/nsdump.c	\
	../components/namespace/nsnames.c	\
	../components/namespace/nsobject.c	\
	../components/namespace/nsparse.c	\
	../components/namespace/nssearch.c	\
	../components/namespace/nsutils.c	\
	../components/namespace/nswalk.c	\
	../components/namespace/nsxfobj.c	\
	../os_specific/service_layers/osunixxf.c\
	../compiler/prexpress.c			\
	../compiler/prmacros.c			\
	../compiler/prscan.c			\
	../compiler/prutils.c			\
	../components/parser/psargs.c		\
	../components/parser/psloop.c		\
	../components/parser/psobject.c		\
	../components/parser/psopcode.c		\
	../components/parser/psopinfo.c		\
	../components/parser/psparse.c		\
	../components/parser/psscope.c		\
	../components/parser/pstree.c		\
	../components/parser/psutils.c		\
	../components/parser/pswalk.c		\
	../components/tables/tbdata.c		\
	../components/tables/tbfadt.c		\
	../components/tables/tbinstal.c		\
	../components/tables/tbprint.c		\
	../components/tables/tbutils.c		\
	../components/tables/tbxface.c		\
	../components/tables/tbxfload.c		\
	../components/utilities/utaddress.c	\
	../components/utilities/utalloc.c	\
	../components/utilities/utascii.c	\
	../components/utilities/utbuffer.c	\
	../components/utilities/utcache.c	\
	../components/utilities/utcopy.c	\
	../components/utilities/utdebug.c	\
	../components/utilities/utdecode.c	\
	../components/utilities/utdelete.c	\
	../components/utilities/uterror.c	\
	../components/utilities/utexcep.c	\
	../components/utilities/utglobal.c	\
	../components/utilities/uthex.c		\
	../components/utilities/utinit.c	\
	../components/utilities/utlock.c	\
	../components/utilities/utmath.c	\
	../components/utilities/utmisc.c	\
	../components/utilities/utmutex.c	\
	../components/utilities/utnonansi.c	\
	../components/utilities/utobject.c	\
	../components/utilities/utownerid.c	\
	../components/utilities/utpredef.c	\
	../components/utilities/utresdecode.c	\
	../components/utilities/utresrc.c	\
	../components/utilities/utstate.c	\
	../components/utilities/utstrtoul64.c	\
	../components/utilities/utstrsuppt.c	\
	../components/utilities/utstring.c	\
	../components/utilities/utuuid.c	\
	../components/utilities/utxface.c	\
	../components/utilities/utxferror.c
