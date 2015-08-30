AM_CPPFLAGS = -Wall -Werror -Wextra -DDATAROOTDIR=\"$(datarootdir)\" \
	`pkg-config --silence-errors --cflags json` \
	`pkg-config --silence-errors --cflags json-c`

bin_PROGRAMS = kernelscan
kernelscan_SOURCES = kernelscan.c
kernelscan_LDFLAGS = -lpcre

