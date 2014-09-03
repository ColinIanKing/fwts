/*
 * Copyright (C) 2014 Canonical
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

#ifndef __FWTS_PM_METHOD_MODE_H__
#define __FWTS_PM_METHOD_MODE_H__

#include <glib.h>
#include <gio/gio.h>
#include <time.h>
#include <stdbool.h>

#define FWTS_ENABLE_LOGIND 	\
	((GLIB_MAJOR_VERSION >= 2) && (GLIB_MINOR_VERSION >= 26))

#include "fwts_types.h"

typedef struct
{
	fwts_framework *fw;
	time_t t_start;
	time_t t_end;
#if FWTS_ENABLE_LOGIND
	GDBusProxy *logind_proxy;
	GDBusConnection *logind_connection;
	GMainLoop *gmainloop;
	char *action;
#endif
	int  min_delay;
} fwts_pm_method_vars;

static inline void free_pm_method_vars(void *);
static inline void freep(void *);

#define _cleanup_free_pm_vars_ __attribute__((cleanup(free_pm_method_vars)))
#define _cleanup_free_ __attribute__((cleanup(freep)))

#define PM_SUSPEND_LOGIND		"Suspend"
#define PM_SUSPEND_HYBRID_LOGIND	"HybridSleep"
#define PM_HIBERNATE_LOGIND		"Hibernate"

#define FWTS_SUSPEND		"FWTS_SUSPEND"
#define FWTS_RESUME		"FWTS_RESUME"
#define FWTS_HIBERNATE	"FWTS_HIBERNATE"
#define FWTS_RESUME	"FWTS_RESUME"

static inline void free_pm_method_vars(void *vars)
{
	fwts_pm_method_vars *var = *(void**)vars;

#if FWTS_ENABLE_LOGIND
	if (var) {
		if (var->logind_proxy) {
			g_object_unref(var->logind_proxy);
			var->logind_proxy = NULL;
		}
		if (var->logind_connection) {
			g_object_unref(var->logind_connection);
			var->logind_connection = NULL;
		}
		if (var->gmainloop) {
			g_main_loop_unref(var->gmainloop);
			var->gmainloop = NULL;
		}
		if (var->action) {
			free(var->action);
			var->action = NULL;
		}
	}
#endif
	free(var);
	var = NULL;
}

static inline void freep(void *p)
{
	free(*(void**) p);
}

int fwts_logind_init_proxy(fwts_pm_method_vars *fwts_settings);
int fwts_logind_wait_for_resume_from_action(fwts_pm_method_vars *fwts_settings,	const char *action,	int minimum_delay);
bool fwts_logind_can_suspend(fwts_pm_method_vars *fwts_settings);
bool fwts_logind_can_hybrid_suspend(fwts_pm_method_vars *fwts_settings);
bool fwts_logind_can_hibernate(fwts_pm_method_vars *fwts_settings);
bool fwts_sysfs_can_suspend(const fwts_pm_method_vars *fwts_settings);
bool fwts_sysfs_can_hybrid_suspend(const fwts_pm_method_vars *fwts_settings);
bool fwts_sysfs_can_hibernate(const fwts_pm_method_vars *fwts_settings);
int fwts_sysfs_do_suspend(const fwts_pm_method_vars *fwts_settings, bool s3_hybrid);
int fwts_sysfs_do_hibernate(const fwts_pm_method_vars *fwts_settings);

#endif
