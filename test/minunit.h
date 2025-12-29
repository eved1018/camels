#ifndef MINUNIT_H
#define MINUNIT_H

#include <stdio.h>

/* External test counter */
extern int tests_run;

/* Buffer for formatted error messages with file and line info */
static char minunit_error_buffer[512];

/* Assert macro with file and line number reporting */
#define mu_assert(message, test)                                                                                       \
    do {                                                                                                               \
        if (!(test)) {                                                                                                 \
            snprintf(minunit_error_buffer, sizeof(minunit_error_buffer), "%s:%d: %s", __FILE__, __LINE__, message);    \
            return minunit_error_buffer;                                                                               \
        }                                                                                                              \
    } while (0)

/* Test runner macro with test name reporting */
#define mu_run_test(test)                                                                                              \
    do {                                                                                                               \
        char* message = test();                                                                                        \
        tests_run++;                                                                                                   \
        if (message) {                                                                                                 \
            printf("  ✗ %s\n    %s\n", #test, message);                                                                \
            return message;                                                                                            \
        } else {                                                                                                       \
            printf("  ✓ %s\n", #test);                                                                                 \
        }                                                                                                              \
    } while (0)

#endif /* MINUNIT_H */
