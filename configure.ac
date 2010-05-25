AC_INIT([biostestkit], [0.1], [colin.king@canonical.com])
          AM_INIT_AUTOMAKE([-Wall -Werror foreign])
          AC_PROG_CC
	  AC_PROG_RANLIB
  	  AC_CHECK_FUNCS([localtime_r])
	  AC_CHECK_HEADERS([stdlib.h])
	  AC_CHECK_HEADERS([string.h])
	  AC_CHECK_HEADERS([unistd.h])
	  AC_FUNC_MALLOC
          AC_CONFIG_HEADERS([config.h])
          AC_CONFIG_FILES([
           Makefile
           src/Makefile
           src/lib/Makefile
           src/lib/src/Makefile
          ])
          AC_OUTPUT
