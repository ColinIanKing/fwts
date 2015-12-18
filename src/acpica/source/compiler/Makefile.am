#
# We need to build this as a separate shared library with the iasl functionality built in. We
# just export the fwts_* interface.
#
AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -Wstrict-prototypes -fno-strict-aliasing -D_LINUX \
		-DACPI_ASL_COMPILER $(CFLAGS) -I$(top_srcdir)/src/acpica/source/include

AM_YFLAGS = -v -d -y

#
# Case in-sensitive scanning
#
AM_LFLAGS = -i

aslcompiler.c: $(srcdir)/aslparser.y
	m4 -P -I$(srcdir) $^ > aslcompiler.y
	${YACC} ${AM_YFLAGS} -baslcompiler -pAslCompiler aslcompiler.y
	mv aslcompiler.tab.c aslcompiler.c
	mv aslcompiler.tab.h aslcompiler.y.h

aslcompilerlex.c: $(srcdir)/aslcompiler.l
	${LEX} ${AM_LFLAGS} -PAslCompiler -oaslcompilerlex.c $^

dtparser.c: $(srcdir)/dtparser.y
	${YACC} ${AM_YFLAGS} -bdtparser -pDtParser $^
	mv dtparser.tab.c dtparser.c
	mv dtparser.tab.h dtparser.y.h

dtparserlex.c: $(srcdir)/dtparser.l
	${LEX} ${AM_LFLAGS} -PDtParser -odtparserlex.c $^

prparser.c: $(srcdir)/prparser.y
	${YACC} ${AM_YFLAGS} -bprparser -pPrParser $^
	mv prparser.tab.c prparser.c
	mv prparser.tab.h prparser.y.h

prparserlex.c: $(srcdir)/prparser.l
	${LEX} ${AM_LFLAGS} -PPrParser -oprparserlex.c $^

pkglib_LTLIBRARIES = libfwtsiasl.la

BUILT_SOURCES = aslcompiler.c 			\
		aslcompilerlex.c		\
		dtparser.c			\
		dtparserlex.c 			\
		prparser.c			\
		prparserlex.c

#
# Just export fwts specific API so we don't clash with core ACPICA library
#
libfwtsiasl_la_LDFLAGS = -export-symbols-regex "fwts_.*" -lpthread -version-info 1:0:0

CLEANFILES = y.output y.tab.c y.tab.h aslcompiler.y.h $(BUILT_SOURCES)

libfwtsiasl_la_CPPFLAGS = $(AM_CPPFLAGS)
libfwtsiasl_la_SOURCES = 			\
	fwts_iasl_interface.c 			\
	aslascii.c				\
	aslcompile.c 				\
	aslcompilerlex.c 			\
	aslcompiler.c 				\
	aslanalyze.c 				\
	aslcodegen.c 				\
	aslerror.c 				\
	aslfiles.c 				\
	aslfold.c 				\
	asllength.c 				\
	asllisting.c 				\
	aslload.c 				\
	asllookup.c 				\
	aslmap.c 				\
	aslmapoutput.c				\
	aslmaputils.c				\
	aslmessages.c				\
	aslopcodes.c 				\
	asloperands.c 				\
	aslopt.c 				\
	aslpredef.c 				\
	aslresource.c 				\
	aslrestype1.c 				\
	aslrestype1i.c 				\
	aslrestype2.c 				\
	aslrestype2d.c 				\
	aslrestype2e.c 				\
	aslrestype2q.c 				\
	aslrestype2w.c 				\
	aslstartup.c 				\
	aslstubs.c 				\
	asltransform.c 				\
	asltree.c 				\
	aslutils.c 				\
	aslbtypes.c 				\
	aslwalks.c 				\
	asluuid.c 				\
	aslmethod.c 				\
	aslhex.c 				\
	aslnamesp.c 				\
	aslfileio.c 				\
	asllistsup.c 				\
	asloffset.c 				\
	aslxref.c 				\
	aslprepkg.c 				\
	aslprintf.c				\
	aslprune.c				\
	dtfield.c 				\
	dtio.c 					\
	dtsubtable.c 				\
	dttemplate.c 				\
	dttable.c 				\
	dttable1.c				\
	dttable2.c				\
	dtutils.c 				\
	dtexpress.c 				\
	dtcompile.c 				\
	dtparser.c	 			\
	dtparserlex.c 				\
	prparser.c 				\
	prparserlex.c 				\
	prscan.c 				\
	aslrestype2s.c 				\
	prmacros.c 				\
	prutils.c 				\
	prexpress.c 				\
	../components/utilities/utalloc.c 	\
	../components/utilities/utcache.c 	\
	../components/utilities/utcopy.c 	\
	../components/utilities/utdebug.c 	\
	../components/utilities/utdelete.c 	\
	../components/utilities/utglobal.c 	\
	../components/utilities/uthex.c		\
	../components/utilities/utinit.c 	\
	../components/utilities/utlock.c 	\
	../components/utilities/utobject.c 	\
	../components/utilities/utmisc.c 	\
	../components/utilities/utmath.c 	\
	../components/utilities/utmutex.c 	\
	../components/utilities/utresrc.c 	\
	../components/utilities/utstate.c 	\
	../components/utilities/utxface.c 	\
	../components/utilities/utxferror.c 	\
	../components/utilities/utdecode.c 	\
	../components/utilities/utpredef.c 	\
	../components/utilities/utstring.c 	\
	../components/utilities/utaddress.c 	\
	../components/utilities/utownerid.c 	\
	../components/utilities/utexcep.c 	\
	../components/utilities/utuuid.c	\
	../components/utilities/utxfinit.c	\
	../components/namespace/nsaccess.c 	\
	../components/namespace/nsalloc.c 	\
	../components/namespace/nsdump.c 	\
	../components/namespace/nsnames.c 	\
	../components/namespace/nsobject.c 	\
	../components/namespace/nsparse.c 	\
	../components/namespace/nssearch.c 	\
	../components/namespace/nsutils.c 	\
	../components/namespace/nswalk.c 	\
	../components/namespace/nsxfobj.c 	\
	../components/parser/psargs.c 		\
	../components/parser/psloop.c 		\
	../components/parser/psopcode.c 	\
	../components/parser/psparse.c 		\
	../components/parser/psscope.c 		\
	../components/parser/pstree.c 		\
	../components/parser/psutils.c 		\
	../components/parser/pswalk.c 		\
	../components/parser/psobject.c 	\
	../components/parser/psopinfo.c 	\
	../components/dispatcher/dswscope.c 	\
	../components/dispatcher/dswstate.c 	\
	../components/dispatcher/dsfield.c 	\
	../components/dispatcher/dsobject.c 	\
	../components/dispatcher/dsopcode.c 	\
	../components/dispatcher/dsutils.c 	\
	../components/dispatcher/dswexec.c 	\
	../components/dispatcher/dswload.c 	\
	../components/dispatcher/dswload2.c 	\
	../components/dispatcher/dsargs.c 	\
	../components/dispatcher/dscontrol.c 	\
	../components/executer/exconvrt.c 	\
	../components/executer/excreate.c 	\
	../components/executer/exdump.c 	\
	../components/executer/exmisc.c 	\
	../components/executer/exmutex.c 	\
	../components/executer/exnames.c 	\
	../components/executer/exoparg1.c 	\
	../components/executer/exoparg2.c 	\
	../components/executer/exoparg3.c 	\
	../components/executer/exoparg6.c 	\
	../components/executer/exprep.c 	\
	../components/executer/exregion.c 	\
	../components/executer/exresnte.c 	\
	../components/executer/exresolv.c 	\
	../components/executer/exresop.c 	\
	../components/executer/exstore.c 	\
	../components/executer/exstoren.c 	\
	../components/executer/exstorob.c 	\
	../components/executer/exsystem.c 	\
	../components/executer/exutils.c 	\
	../common/adfile.c 			\
	../common/adisasm.c 			\
	../common/adwalk.c 			\
	../common/ahids.c 			\
	../common/ahpredef.c 			\
	../common/cmfsize.c			\
	../common/dmextern.c 			\
	../common/dmrestag.c 			\
	../common/dmtable.c 			\
	../common/dmtables.c			\
	../common/dmtbinfo.c 			\
	../common/dmtbdump.c 			\
	../common/getopt.c			\
	../components/debugger/dbfileio.c 	\
	../components/disassembler/dmbuffer.c 	\
	../components/disassembler/dmcstyle.c	\
	../components/disassembler/dmnames.c 	\
	../components/disassembler/dmopcode.c 	\
	../components/disassembler/dmresrc.c 	\
	../components/disassembler/dmresrcl.c 	\
	../components/disassembler/dmresrcs.c 	\
	../components/disassembler/dmutils.c 	\
	../components/disassembler/dmwalk.c 	\
	../components/disassembler/dmdeferred.c \
	../components/disassembler/dmresrcl2.c 	\
	../components/tables/tbdata.c		\
	../components/tables/tbfadt.c 		\
	../components/tables/tbinstal.c 	\
	../components/tables/tbutils.c 		\
	../components/tables/tbxface.c 		\
	../components/tables/tbxfroot.c		\
	../components/tables/tbxfload.c		\
	../components/tables/tbprint.c		\
	../tools/acpiexec/aeinitfile.c		\
	../os_specific/service_layers/osunixxf.c

