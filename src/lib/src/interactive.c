/*
 * Copyright (C) 2010 Canonical
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "fwts.h"

/*
 *  fwts_getchar()
 *	read a char from tty, no echo canonical mode
 */
static int fwts_getchar(void)
{
	struct termios oldterm;
	struct termios newterm;
	int ch;
	int fd = fileno(stdin);

	tcgetattr(fd, &oldterm);
	memcpy(&newterm, &oldterm, sizeof(struct termios));

	newterm.c_lflag &= ~(ECHO | ICANON);
	newterm.c_cc[VMIN] = 1;
	newterm.c_cc[VTIME] = 0;
	tcsetattr(fd, TCSANOW, &newterm);

	ch = getchar();
	
	tcsetattr(fd, TCSANOW, &oldterm);

	return ch;
}

/*
 *  fwts_printf()
 *	plain old printf() wrapped for fwts
 */ 
int fwts_printf(fwts_framework *fw, const char *fmt, ...)
{
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = vfprintf(stdout, fmt, ap);
	fflush(stdout);
	va_end(ap);

	return len;
}

/*
 *  fwts_press_entrer()
 *	prompt and wait for enter key
 */
int fwts_press_enter(fwts_framework *fw)
{
	fprintf(stdout, "Press <Enter> to continue");
	fflush(stdout);
	
	while (fwts_getchar() != '\n')
		;
	fprintf(stdout, "\n");
	fflush(stdout);
	
	return FWTS_OK;
}

/*
 *  fwts_get_reply()
 *	prompt and wait for a given reply that matches given options string
 */
int fwts_get_reply(fwts_framework *fw, const char *message, const char *options)
{
	int ch;

	fprintf(stdout, "%s", message);
	fflush(stdout);

	while (index(options, (ch = fwts_getchar())) == NULL)
		;
	fprintf(stdout, "\n");
	fflush(stdout);

	return ch;
}

