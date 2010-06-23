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

#include "fwts.h"

static int example_init(fwts_framework *fw)
{
	/* Put pre-test initialistion code here */

	/* Returns:
	 *	FWTS_ERROR - failed, abort test 
	 *	FWTS_OK    - success, do tests 
	 */
	return FWTS_OK;
}

static int example_deinit(fwts_framework *fw)
{
	/* Put post-test de-initialistion code here */

	/* Returns:
	 *	FWTS_ERROR - failed, abort test 
	 *	FWTS_OK    - success, do tests 
	 */
	return FWTS_OK;
}

static char *example_headline(void)
{
	/* Return the name of the test scenario */
	return "Example test name.";
}

static int example_test1(fwts_framework *fw)
{
	/* Do your test */
	 
	/* Log success or failure */
	fwts_passed(fw, "Test passed, hurrah!");
	/*
	fwts_failed_critical(fw, "Test failed!");
	fwts_failed_high(fw, "Test failed!");
	fwts_failed_medium(fw, "Test failed!");
	fwts_failed_low(fw, "Test failed!");
	*/

	/* Returns:
	 *	FWTS_ERROR - failed, abort test 
	 *	FWTS_OK    - success, do tests 
	 */
	return FWTS_OK
}


/*
 *  Null terminated array of tests to run, in this
 *  scenario, we just have one test.
 */
static fwts_framework_tests example_tests[] = {
	example_test1,
	NULL
};

static fwts_framework_ops example_ops = {
	example_headline,
	example_init,	/* Can be NULL if not required */
	example_deinit,	/* Can be NULL if not required */
	example_tests
};

/*
 *   See fwts_framework.h for flags,
 */
FWTS_REGISTER(example, &example_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);
