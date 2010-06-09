/* 
 * Copyright (C) 2006, Intel Corp
 * Copyright (C) 2007, AMD Inc
 * Copyright (C) 2010 Canonical
 *
 * This code was originally part of the Linux-ready Firmware Developer Kit
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
 #define _GNU_SOURCE
#include <sched.h>
#include <string.h>
#include <stdint.h>

#include "fwts.h"

void exec_cpuid(int cpu, uint32 cmd, cpu_registers*  regs) 
{
	cpu_set_t mask, oldmask;

	if (cpu != CURRENT_CPU) {
		sched_getaffinity(0, sizeof(oldmask), &oldmask);
		CPU_ZERO(&mask);
		CPU_SET(cpu, &mask);
		sched_setaffinity(0, sizeof(mask), &mask);
	}

#if defined(__i386__)
    	__asm__ __volatile__ (  "pushl %%ebx \n\t"
				"cpuid	\n\t"
				"movl %%ebx,%%esi \n\t"
				"popl %%ebx \n\t"
			   	: "=a"(regs->eax),"=S"(regs->ebx),
                             	"=c"(regs->ecx),"=d"(regs->edx) 
			   	: "a"(cmd)
	);
#elif defined (__x86_64__)
    	__asm__ __volatile__ ( "cpuid \n\t" 
			   : "=a"(regs->eax),"=b"(regs->ebx),
                             "=c"(regs->ecx),"=d"(regs->edx) 
			   : "a"(cmd) 
	);
#endif
	
	if (cpu != CURRENT_CPU)
		sched_setaffinity(0, sizeof(oldmask), &oldmask);
}
