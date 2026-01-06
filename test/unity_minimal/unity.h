#ifndef UNITY_MINIMAL_H
#define UNITY_MINIMAL_H

#include <stdio.h>
#include <math.h>

// Colors
#define RED   "\x1B[31m"
#define GREEN "\x1B[32m"
#define RESET "\x1B[0m"

extern int tests_run;
extern int tests_failed;

void UnityBegin(void);
int UnityEnd(void);

// Basic Assertions
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf(RED "FAIL: %s:%d" RESET "\n", __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual) \
    do { \
        if (fabs((double)(expected) - (double)(actual)) > (double)(delta)) { \
            printf(RED "FAIL: Expected %f +/- %f but got %f at %s:%d" RESET "\n", \
                   (double)(expected), (double)(delta), (double)(actual), __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s... ", #test_func); \
        int failed_before = tests_failed; \
        test_func(); \
        if (tests_failed == failed_before) { \
            printf(GREEN "PASS" RESET "\n"); \
        } \
        tests_run++; \
    } while (0)

#endif
