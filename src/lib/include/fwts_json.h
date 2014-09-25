/*
 * Copyright (C) 2012-2014 Canonical
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

#ifndef __FWTS_JSON_H__
#define __FWTS_JSON_H__

#include <json.h>

#define __FWTS_JSON_ERR_PTR__ ((json_object*) -1)
/*
 *  Older versions of json-c may return an error in an
 *  object as a ((json_object*)-1), where as newer
 *  versions return NULL, so check for these. Sigh.
 */
#define FWTS_JSON_ERROR(ptr) \
	( (ptr == NULL) || ((json_object*)ptr == __FWTS_JSON_ERR_PTR__) )

#endif
