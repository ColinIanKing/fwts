/*
 * Copyright (C) 2010-2013 Canonical
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

#ifndef __FWTS_WAKEALARM_H__
#define __FWTS_WAKEALARM_H__

#include "fwts_framework.h"

#define WAKEALARM "/sys/class/rtc/rtc0/wakealarm"

int fwts_wakealarm_get_irq_state(void);
int fwts_wakealarm_test_firing(fwts_framework *fw, const int sleep);
int fwts_wakealarm_trigger(fwts_framework *fw, const int seconds);

#endif
