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

ACPICA = $(top_srcdir)/src/acpica/source

AM_CPPFLAGS = -Wall -Wstrict-prototypes		\
	      -fno-strict-aliasing 		\
	      -D_LINUX				\
	      -DACPI_ASL_COMPILER $(CFLAGS)	\
	      -I$(ACPICA)/include		\
	      -I$(ACPICA)/compiler

AM_YFLAGS = -d

#
# Case in-sensitive scanning
#
AM_LFLAGS = -i

ASL_PARSER = 					\
	$(ACPICA)/compiler/aslcstyle.y		\
	$(ACPICA)/compiler/aslhelpers.y		\
	$(ACPICA)/compiler/aslparser.y		\
	$(ACPICA)/compiler/aslprimaries.y	\
	$(ACPICA)/compiler/aslresources.y	\
	$(ACPICA)/compiler/aslrules.y		\
	$(ACPICA)/compiler/aslsupport.y		\
	$(ACPICA)/compiler/asltokens.y		\
	$(ACPICA)/compiler/asltypes.y

ASL_LEXER =
	$(ACPICA)/compiler/aslcompiler.l	\
	$(srcdir)/aslsupport.l			\
	$(srcdir)/aslcompiler.y.h

$(srcdir)/aslcompiler.y: $(ASL_PARSER)
	m4 -P -I$(ACPICA)/compiler $(ACPICA)/compiler/aslparser.y > $@

$(srcdir)/aslcompilerlex.c: $(ASL_LEXER)
	${LEX} ${AM_LFLAGS} -PAslCompiler -o$@ $(ACPICA)/compiler/aslcompiler.l

.NOTPARALLEL: $(srcdir)/aslcompiler.c
$(srcdir)/aslcompiler.c $(srcdir)/aslcompiler.y.h: $(srcdir)/aslcompiler.y
	${YACC} ${AM_YFLAGS} -d -baslcompiler -pAslCompiler $^
	mv aslcompiler.tab.c aslcompiler.c
	mv aslcompiler.tab.h aslcompiler.y.h

.NOTPARALLEL: $(srcdir)/dtparserlex.c
$(srcdir)/dtparserlex.c $(srcdir)/dtparser.c $(srcdir)/dtparser.y.h: $(ACPICA)/compiler/dtparser.l $(ACPICA)/compiler/dtparser.y
	${LEX} ${AM_LFLAGS} -PDtParser -o$(srcdir)/dtparserlex.c $<
	${YACC} ${AM_YFLAGS} -bdtparser -pDtParser $(ACPICA)/compiler/dtparser.y
	mv dtparser.tab.c dtparser.c
	mv dtparser.tab.h dtparser.y.h

.NOTPARALLEL: $(srcdir)/prparserlex.c
$(srcdir)/prparserlex.c $(srcdir)/prparser.c $(srcdir)/prparser.y.h: $(ACPICA)/compiler/prparser.l $(ACPICA)/compiler/prparser.y
	${LEX} ${AM_LFLAGS} -PPrParser -o$(srcdir)/prparserlex.c $<
	${YACC} ${AM_YFLAGS} -bprparser -pPrParser $(ACPICA)/compiler/prparser.y
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
libfwtsiasl_la_SOURCES = 				\
	fwts_iasl_interface.c 				\
	aslcompilerlex.c				\
	aslcompiler.c					\
	dtparserlex.c					\
	dtparser.c					\
	prparserlex.c					\
	prparser.c					\
	$(ACPICA)/common/adisasm.c			\
	$(ACPICA)/common/acfileio.c			\
	$(ACPICA)/common/adfile.c			\
	$(ACPICA)/common/adwalk.c			\
	$(ACPICA)/common/ahids.c			\
	$(ACPICA)/common/ahpredef.c			\
	$(ACPICA)/common/ahtable.c			\
	$(ACPICA)/common/ahuuids.c			\
	$(ACPICA)/compiler/aslallocate.c		\
	$(ACPICA)/compiler/aslanalyze.c			\
	$(ACPICA)/compiler/aslascii.c			\
	$(ACPICA)/compiler/aslbtypes.c			\
	$(ACPICA)/compiler/aslcache.c			\
	$(ACPICA)/compiler/aslcodegen.c			\
	$(ACPICA)/compiler/aslcompile.c			\
	$(ACPICA)/compiler/asldebug.c			\
	$(ACPICA)/compiler/aslerror.c			\
	$(ACPICA)/compiler/aslexternal.c		\
	$(ACPICA)/compiler/aslfiles.c			\
	$(ACPICA)/compiler/aslfileio.c			\
	$(ACPICA)/compiler/aslfold.c			\
	$(ACPICA)/compiler/aslhelp.c			\
	$(ACPICA)/compiler/aslhex.c			\
	$(ACPICA)/compiler/asllength.c			\
	$(ACPICA)/compiler/asllisting.c			\
	$(ACPICA)/compiler/asllistsup.c			\
	$(ACPICA)/compiler/aslload.c			\
	$(ACPICA)/compiler/asllookup.c			\
	$(ACPICA)/compiler/aslmain.c			\
	$(ACPICA)/compiler/aslmap.c			\
	$(ACPICA)/compiler/aslmapenter.c		\
	$(ACPICA)/compiler/aslmapoutput.c		\
	$(ACPICA)/compiler/aslmaputils.c		\
	$(ACPICA)/compiler/aslmessages.c		\
	$(ACPICA)/compiler/aslmethod.c			\
	$(ACPICA)/compiler/aslnamesp.c			\
	$(ACPICA)/compiler/asloffset.c			\
	$(ACPICA)/compiler/aslopcodes.c			\
	$(ACPICA)/compiler/asloperands.c		\
	$(ACPICA)/compiler/aslopt.c			\
	$(ACPICA)/compiler/asloptions.c			\
	$(ACPICA)/compiler/aslparseop.c			\
	$(ACPICA)/compiler/aslpredef.c			\
	$(ACPICA)/compiler/aslprepkg.c			\
	$(ACPICA)/compiler/aslprintf.c			\
	$(ACPICA)/compiler/aslprune.c			\
	$(ACPICA)/compiler/aslresource.c		\
	$(ACPICA)/compiler/aslrestype1.c		\
	$(ACPICA)/compiler/aslrestype1i.c		\
	$(ACPICA)/compiler/aslrestype2.c		\
	$(ACPICA)/compiler/aslrestype2d.c		\
	$(ACPICA)/compiler/aslrestype2e.c		\
	$(ACPICA)/compiler/aslrestype2q.c		\
	$(ACPICA)/compiler/aslrestype2s.c		\
	$(ACPICA)/compiler/aslrestype2w.c		\
	$(ACPICA)/compiler/aslstartup.c			\
	$(ACPICA)/compiler/aslstubs.c			\
	$(ACPICA)/compiler/aslpld.c			\
	$(ACPICA)/compiler/asltransform.c		\
	$(ACPICA)/compiler/asltree.c			\
	$(ACPICA)/compiler/aslutils.c			\
	$(ACPICA)/compiler/asluuid.c			\
	$(ACPICA)/compiler/aslwalks.c			\
	$(ACPICA)/compiler/aslxref.c			\
	$(ACPICA)/compiler/aslxrefout.c			\
	$(ACPICA)/compiler/cvcompiler.c			\
	$(ACPICA)/compiler/cvdisasm.c			\
	$(ACPICA)/compiler/cvparser.c			\
	$(ACPICA)/common/cmfsize.c			\
	$(ACPICA)/components/debugger/dbfileio.c	\
	$(ACPICA)/components/disassembler/dmbuffer.c	\
	$(ACPICA)/components/disassembler/dmcstyle.c	\
	$(ACPICA)/components/disassembler/dmdeferred.c	\
	$(ACPICA)/common/dmextern.c			\
	$(ACPICA)/components/disassembler/dmnames.c	\
	$(ACPICA)/components/disassembler/dmopcode.c	\
	$(ACPICA)/components/disassembler/dmresrc.c	\
	$(ACPICA)/components/disassembler/dmresrcl.c	\
	$(ACPICA)/components/disassembler/dmresrcl2.c	\
	$(ACPICA)/components/disassembler/dmresrcs.c	\
	$(ACPICA)/common/dmrestag.c			\
	$(ACPICA)/common/dmswitch.c			\
	$(ACPICA)/common/dmtable.c			\
	$(ACPICA)/common/dmtables.c			\
	$(ACPICA)/common/dmtbdump.c			\
	$(ACPICA)/common/dmtbdump1.c			\
	$(ACPICA)/common/dmtbdump2.c			\
	$(ACPICA)/common/dmtbdump3.c			\
	$(ACPICA)/common/dmtbinfo.c			\
	$(ACPICA)/common/dmtbinfo1.c			\
	$(ACPICA)/common/dmtbinfo2.c			\
	$(ACPICA)/common/dmtbinfo3.c			\
	$(ACPICA)/components/disassembler/dmutils.c	\
	$(ACPICA)/components/disassembler/dmwalk.c	\
	$(ACPICA)/components/dispatcher/dsargs.c	\
	$(ACPICA)/components/dispatcher/dscontrol.c	\
	$(ACPICA)/components/dispatcher/dsfield.c	\
	$(ACPICA)/components/dispatcher/dsobject.c	\
	$(ACPICA)/components/dispatcher/dsopcode.c	\
	$(ACPICA)/components/dispatcher/dspkginit.c	\
	$(ACPICA)/components/dispatcher/dsutils.c	\
	$(ACPICA)/components/dispatcher/dswexec.c	\
	$(ACPICA)/components/dispatcher/dswload.c	\
	$(ACPICA)/components/dispatcher/dswload2.c	\
	$(ACPICA)/components/dispatcher/dswscope.c	\
	$(ACPICA)/components/dispatcher/dswstate.c	\
	$(ACPICA)/compiler/dtcompile.c			\
	$(ACPICA)/compiler/dtexpress.c			\
	$(ACPICA)/compiler/dtfield.c			\
	$(ACPICA)/compiler/dtio.c			\
	$(ACPICA)/compiler/dtsubtable.c			\
	$(ACPICA)/compiler/dttable.c			\
	$(ACPICA)/compiler/dttable1.c			\
	$(ACPICA)/compiler/dttable2.c			\
	$(ACPICA)/compiler/dttemplate.c			\
	$(ACPICA)/compiler/dtutils.c			\
	$(ACPICA)/components/executer/exconcat.c	\
	$(ACPICA)/components/executer/exconvrt.c	\
	$(ACPICA)/components/executer/excreate.c	\
	$(ACPICA)/components/executer/exdump.c		\
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
	$(ACPICA)/components/executer/exstore.c		\
	$(ACPICA)/components/executer/exstoren.c	\
	$(ACPICA)/components/executer/exstorob.c	\
	$(ACPICA)/components/executer/exsystem.c	\
	$(ACPICA)/components/executer/exutils.c		\
	$(ACPICA)/common/getopt.c			\
	$(ACPICA)/components/namespace/nsaccess.c	\
	$(ACPICA)/components/namespace/nsalloc.c	\
	$(ACPICA)/components/namespace/nsdump.c		\
	$(ACPICA)/components/namespace/nsnames.c	\
	$(ACPICA)/components/namespace/nsobject.c	\
	$(ACPICA)/components/namespace/nsparse.c	\
	$(ACPICA)/components/namespace/nssearch.c	\
	$(ACPICA)/components/namespace/nsutils.c	\
	$(ACPICA)/components/namespace/nswalk.c		\
	$(ACPICA)/components/namespace/nsxfobj.c	\
	$(ACPICA)/os_specific/service_layers/osunixxf.c	\
	$(ACPICA)/compiler/prexpress.c			\
	$(ACPICA)/compiler/prmacros.c			\
	$(ACPICA)/compiler/prscan.c			\
	$(ACPICA)/compiler/prutils.c			\
	$(ACPICA)/components/parser/psargs.c		\
	$(ACPICA)/components/parser/psloop.c		\
	$(ACPICA)/components/parser/psobject.c		\
	$(ACPICA)/components/parser/psopcode.c		\
	$(ACPICA)/components/parser/psopinfo.c		\
	$(ACPICA)/components/parser/psparse.c		\
	$(ACPICA)/components/parser/psscope.c		\
	$(ACPICA)/components/parser/pstree.c		\
	$(ACPICA)/components/parser/psutils.c		\
	$(ACPICA)/components/parser/pswalk.c		\
	$(ACPICA)/components/tables/tbdata.c		\
	$(ACPICA)/components/tables/tbfadt.c		\
	$(ACPICA)/components/tables/tbinstal.c		\
	$(ACPICA)/components/tables/tbprint.c		\
	$(ACPICA)/components/tables/tbutils.c		\
	$(ACPICA)/components/tables/tbxface.c		\
	$(ACPICA)/components/tables/tbxfload.c		\
	$(ACPICA)/components/utilities/utaddress.c	\
	$(ACPICA)/components/utilities/utalloc.c	\
	$(ACPICA)/components/utilities/utascii.c	\
	$(ACPICA)/components/utilities/utbuffer.c	\
	$(ACPICA)/components/utilities/utcache.c	\
	$(ACPICA)/components/utilities/utcopy.c		\
	$(ACPICA)/components/utilities/utdebug.c	\
	$(ACPICA)/components/utilities/utdecode.c	\
	$(ACPICA)/components/utilities/utdelete.c	\
	$(ACPICA)/components/utilities/uterror.c	\
	$(ACPICA)/components/utilities/utexcep.c	\
	$(ACPICA)/components/utilities/utglobal.c	\
	$(ACPICA)/components/utilities/uthex.c		\
	$(ACPICA)/components/utilities/utinit.c		\
	$(ACPICA)/components/utilities/utlock.c		\
	$(ACPICA)/components/utilities/utmath.c		\
	$(ACPICA)/components/utilities/utmisc.c		\
	$(ACPICA)/components/utilities/utmutex.c	\
	$(ACPICA)/components/utilities/utnonansi.c	\
	$(ACPICA)/components/utilities/utobject.c	\
	$(ACPICA)/components/utilities/utownerid.c	\
	$(ACPICA)/components/utilities/utpredef.c	\
	$(ACPICA)/components/utilities/utresdecode.c	\
	$(ACPICA)/components/utilities/utresrc.c	\
	$(ACPICA)/components/utilities/utstate.c	\
	$(ACPICA)/components/utilities/utstrtoul64.c	\
	$(ACPICA)/components/utilities/utstrsuppt.c	\
	$(ACPICA)/components/utilities/utstring.c	\
	$(ACPICA)/components/utilities/utuuid.c		\
	$(ACPICA)/components/utilities/utxface.c	\
	$(ACPICA)/components/utilities/utxferror.c
