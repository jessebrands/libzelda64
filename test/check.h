/*
 * check.h: test assertions
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of libzelda64.
 *
 * libzelda64 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libzelda64 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libzelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ZELDA64_TEST_CHECK_H
#define ZELDA64_TEST_CHECK_H

#include <stdio.h>

#include "zelda64/zelda64.h"

/*
 * Count of failed assertions.
 */
static int failures = 0;

/*
 * Checks if a result matches the expected resulted.
 */
static inline void
check(char const* what, enum zelda64_result const result, enum zelda64_result const expected) {
    int const ok = result == expected;
    printf("%-42s %-20s %s\n", what, zelda64_result_string(result), ok ? "ok" : "FAIL");
    if (!ok) {
        failures += 1;
    }
}

/*
 * Check a boolean value.
 */
static inline void
check_that(char const* what, int const ok) {
    printf("%-42s %-20s %s\n", what, "", ok ? "ok" : "FAIL");
    if (!ok) {
        failures += 1;
    }
}

/*
 * Checks if any assertions failed during program execution.
 */
static inline int
check_report(void) {
    printf("\n%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}

#endif //ZELDA64_TEST_CHECK_H
