#
# We need to build this as a separate shared library with the iasl functionality built in. We
# just export the fwts_* interface.
#
AM_CPPFLAGS = -Wall -Wstrict-prototypes -fno-strict-aliasing -D_LINUX -DACPI_ASL_COMPILER $(CFLAGS) -I$(top_srcdir)/src/acpica/source/include

ACPICA_COMPILER     = .
ACPICA_COMMON       = ../common
ACPICA_COMPONENTS   = ../components
ACPICA_UTILS        = $(ACPICA_COMPONENTS)/utilities
ACPICA_NAMESPACE    = $(ACPICA_COMPONENTS)/namespace
ACPICA_PARSER       = $(ACPICA_COMPONENTS)/parser
ACPICA_DISPATCHER   = $(ACPICA_COMPONENTS)/dispatcher
ACPICA_EXECUTER     = $(ACPICA_COMPONENTS)/executer
ACPICA_DEBUGGER     = $(ACPICA_COMPONENTS)/debugger
ACPICA_DISASSEMBLER = $(ACPICA_COMPONENTS)/disassembler
ACPICA_TABLES       = $(ACPICA_COMPONENTS)/tables
ACPICA_SERVICE      = ../os_specific/service_layers

AM_YFLAGS = -v -d -y

#
# Case in-sensitive scanning
#
AM_LFLAGS = -i

aslcompilerparse.c: aslcompiler.y
	${YACC} ${AM_YFLAGS} -baslcompiler -pAslCompiler aslcompiler.y
	mv aslcompiler.tab.c aslcompilerparse.c
	mv aslcompiler.tab.h aslcompiler.y.h

aslcompilerlex.c: aslcompiler.l
	${LEX} ${AM_LFLAGS} -PAslCompiler -oaslcompilerlex.c aslcompiler.l

dtparserparse.c: dtparser.y
	${YACC} ${AM_YFLAGS} -bdtparser -pDtParser dtparser.y
	mv dtparser.tab.c dtparserparse.c
	mv dtparser.tab.h dtparser.y.h

dtparserlex.c: dtparser.l
	${LEX} ${AM_LFLAGS} -PDtParser -odtparserlex.c dtparser.l

prparserparse.c: prparser.y
	${YACC} ${AM_YFLAGS} -bprparser -pPrParser prparser.y
	mv prparser.tab.c prparserparse.c
	mv prparser.tab.h prparser.y.h

prparserlex.c: prparser.l
	${LEX} ${AM_LFLAGS} -PPrParser -oprparserlex.c prparser.l

pkglib_LTLIBRARIES = libfwtsiasl.la

BUILT_SOURCES = aslcompilerparse.c aslcompilerlex.c \
		dtparserparse.c dtparserlex.c \
		prparserparse.c prparserlex.c

#
# Just export fwts specific API so we don't clash with core ACPICA library
#
libfwtsiasl_la_LDFLAGS = -export-symbols-regex "fwts_.*" -lpthread -version-info 1:0:0

CLEANFILES = y.output y.tab.c y.tab.h aslcompiler.y.h \
	aslcompilerparse.c aslcompilerlex.c \
	dtparserparse.c dtparserlex.c

libfwtsiasl_la_SOURCES = \
	$(ACPICA_COMPILER)/aslcompile.c \
	$(ACPICA_COMPILER)/aslcompilerlex.c \
	$(ACPICA_COMPILER)/aslcompilerparse.c \
	$(ACPICA_COMPILER)/aslanalyze.c \
	$(ACPICA_COMPILER)/aslcodegen.c \
	$(ACPICA_COMPILER)/aslerror.c \
	$(ACPICA_COMPILER)/aslfiles.c \
	$(ACPICA_COMPILER)/aslfold.c \
	$(ACPICA_COMPILER)/asllength.c \
	$(ACPICA_COMPILER)/asllisting.c \
	$(ACPICA_COMPILER)/aslload.c \
	$(ACPICA_COMPILER)/asllookup.c \
	$(ACPICA_COMPILER)/aslmap.c \
	$(ACPICA_COMPILER)/aslopcodes.c \
	$(ACPICA_COMPILER)/asloperands.c \
	$(ACPICA_COMPILER)/aslopt.c \
	$(ACPICA_COMPILER)/aslpredef.c \
	$(ACPICA_COMPILER)/aslresource.c \
	$(ACPICA_COMPILER)/aslrestype1.c \
	$(ACPICA_COMPILER)/aslrestype1i.c \
	$(ACPICA_COMPILER)/aslrestype2.c \
	$(ACPICA_COMPILER)/aslrestype2d.c \
	$(ACPICA_COMPILER)/aslrestype2e.c \
	$(ACPICA_COMPILER)/aslrestype2q.c \
	$(ACPICA_COMPILER)/aslrestype2w.c \
	$(ACPICA_COMPILER)/aslstartup.c \
	$(ACPICA_COMPILER)/aslstubs.c \
	$(ACPICA_COMPILER)/asltransform.c \
	$(ACPICA_COMPILER)/asltree.c \
	$(ACPICA_COMPILER)/aslutils.c \
	$(ACPICA_COMPILER)/aslbtypes.c \
	$(ACPICA_COMPILER)/aslwalks.c \
	$(ACPICA_COMPILER)/asluuid.c \
	$(ACPICA_COMPILER)/aslmethod.c \
	$(ACPICA_COMPILER)/aslhex.c \
	$(ACPICA_COMPILER)/aslnamesp.c \
	$(ACPICA_COMPILER)/aslfileio.c \
	$(ACPICA_COMPILER)/asllistsup.c \
	$(ACPICA_COMPILER)/asloffset.c \
	$(ACPICA_COMPILER)/aslxref.c \
	$(ACPICA_COMPILER)/aslprepkg.c \
	$(ACPICA_COMPILER)/dtfield.c \
	$(ACPICA_COMPILER)/dtio.c \
	$(ACPICA_COMPILER)/dtsubtable.c \
	$(ACPICA_COMPILER)/dttemplate.c \
	$(ACPICA_COMPILER)/dttable.c \
	$(ACPICA_COMPILER)/dtutils.c \
	$(ACPICA_COMPILER)/dtexpress.c \
	$(ACPICA_COMPILER)/dtcompile.c \
	$(ACPICA_COMPILER)/dtparserparse.c \
	$(ACPICA_COMPILER)/dtparserlex.c \
	$(ACPICA_COMPILER)/prparserparse.c \
	$(ACPICA_COMPILER)/prparserlex.c \
	$(ACPICA_COMPILER)/prscan.c \
	$(ACPICA_COMPILER)/aslrestype2s.c \
	$(ACPICA_COMPILER)/prmacros.c \
	$(ACPICA_COMPILER)/prutils.c \
	$(ACPICA_COMPILER)/prexpress.c \
	$(ACPICA_UTILS)/utalloc.c \
	$(ACPICA_UTILS)/utcache.c \
	$(ACPICA_UTILS)/utcopy.c \
	$(ACPICA_UTILS)/utdebug.c \
	$(ACPICA_UTILS)/utdelete.c \
	$(ACPICA_UTILS)/utglobal.c \
	$(ACPICA_UTILS)/utinit.c \
	$(ACPICA_UTILS)/utlock.c \
	$(ACPICA_UTILS)/utobject.c \
	$(ACPICA_UTILS)/utmisc.c \
	$(ACPICA_UTILS)/utmath.c \
	$(ACPICA_UTILS)/utmutex.c \
	$(ACPICA_UTILS)/utresrc.c \
	$(ACPICA_UTILS)/utstate.c \
	$(ACPICA_UTILS)/utxface.c \
	$(ACPICA_UTILS)/utxferror.c \
	$(ACPICA_UTILS)/utdecode.c \
	$(ACPICA_UTILS)/utpredef.c \
	$(ACPICA_UTILS)/utstring.c \
	$(ACPICA_UTILS)/utaddress.c \
	$(ACPICA_UTILS)/utownerid.c \
	$(ACPICA_UTILS)/utexcep.c \
	$(ACPICA_NAMESPACE)/nsaccess.c \
	$(ACPICA_NAMESPACE)/nsalloc.c \
	$(ACPICA_NAMESPACE)/nsdump.c \
	$(ACPICA_NAMESPACE)/nsnames.c \
	$(ACPICA_NAMESPACE)/nsobject.c \
	$(ACPICA_NAMESPACE)/nsparse.c \
	$(ACPICA_NAMESPACE)/nssearch.c \
	$(ACPICA_NAMESPACE)/nsutils.c \
	$(ACPICA_NAMESPACE)/nswalk.c \
	$(ACPICA_NAMESPACE)/nsxfobj.c \
	$(ACPICA_PARSER)/psargs.c \
	$(ACPICA_PARSER)/psloop.c \
	$(ACPICA_PARSER)/psopcode.c \
	$(ACPICA_PARSER)/psparse.c \
	$(ACPICA_PARSER)/psscope.c \
	$(ACPICA_PARSER)/pstree.c \
	$(ACPICA_PARSER)/psutils.c \
	$(ACPICA_PARSER)/pswalk.c \
	$(ACPICA_PARSER)/psobject.c \
	$(ACPICA_PARSER)/psopinfo.c \
	$(ACPICA_DISPATCHER)/dswscope.c \
	$(ACPICA_DISPATCHER)/dswstate.c \
	$(ACPICA_DISPATCHER)/dsfield.c \
	$(ACPICA_DISPATCHER)/dsobject.c \
	$(ACPICA_DISPATCHER)/dsopcode.c \
	$(ACPICA_DISPATCHER)/dsutils.c \
	$(ACPICA_DISPATCHER)/dswexec.c \
	$(ACPICA_DISPATCHER)/dswload.c \
	$(ACPICA_DISPATCHER)/dswload2.c \
	$(ACPICA_DISPATCHER)/dsargs.c \
	$(ACPICA_DISPATCHER)/dscontrol.c \
	$(ACPICA_EXECUTER)/exconvrt.c \
	$(ACPICA_EXECUTER)/excreate.c \
	$(ACPICA_EXECUTER)/exdump.c \
	$(ACPICA_EXECUTER)/exmisc.c \
	$(ACPICA_EXECUTER)/exmutex.c \
	$(ACPICA_EXECUTER)/exnames.c \
	$(ACPICA_EXECUTER)/exoparg1.c \
	$(ACPICA_EXECUTER)/exoparg2.c \
	$(ACPICA_EXECUTER)/exoparg3.c \
	$(ACPICA_EXECUTER)/exoparg6.c \
	$(ACPICA_EXECUTER)/exprep.c \
	$(ACPICA_EXECUTER)/exregion.c \
	$(ACPICA_EXECUTER)/exresnte.c \
	$(ACPICA_EXECUTER)/exresolv.c \
	$(ACPICA_EXECUTER)/exresop.c \
	$(ACPICA_EXECUTER)/exstore.c \
	$(ACPICA_EXECUTER)/exstoren.c \
	$(ACPICA_EXECUTER)/exstorob.c \
	$(ACPICA_EXECUTER)/exsystem.c \
	$(ACPICA_EXECUTER)/exutils.c \
	$(ACPICA_COMMON)/adfile.c \
	$(ACPICA_COMMON)/adisasm.c \
	$(ACPICA_COMMON)/adwalk.c \
	$(ACPICA_COMMON)/ahpredef.c \
	$(ACPICA_COMMON)/dmextern.c \
	$(ACPICA_COMMON)/dmrestag.c \
	$(ACPICA_COMMON)/dmtable.c \
	$(ACPICA_COMMON)/dmtbinfo.c \
	$(ACPICA_COMMON)/dmtbdump.c \
	$(ACPICA_DEBUGGER)/dbfileio.c \
	$(ACPICA_DISASSEMBLER)/dmbuffer.c \
	$(ACPICA_DISASSEMBLER)/dmnames.c \
	$(ACPICA_DISASSEMBLER)/dmopcode.c \
	$(ACPICA_DISASSEMBLER)/dmobject.c \
	$(ACPICA_DISASSEMBLER)/dmresrc.c \
	$(ACPICA_DISASSEMBLER)/dmresrcl.c \
	$(ACPICA_DISASSEMBLER)/dmresrcs.c \
	$(ACPICA_DISASSEMBLER)/dmutils.c \
	$(ACPICA_DISASSEMBLER)/dmwalk.c \
	$(ACPICA_DISASSEMBLER)/dmdeferred.c \
	$(ACPICA_DISASSEMBLER)/dmresrcl2.c \
	$(ACPICA_TABLES)/tbfadt.c \
	$(ACPICA_TABLES)/tbinstal.c \
	$(ACPICA_TABLES)/tbutils.c \
	$(ACPICA_TABLES)/tbxface.c \
	$(ACPICA_SERVICE)/osunixxf.c \
	$(ACPICA_COMPILER)/fwts_iasl_interface.c 
