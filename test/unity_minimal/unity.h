#ifndef UNITY_MINIMAL_H
#define UNITY_MINIMAL_H

#include <stdio.h>
#include <math.h>
#include <stdint.h>

// Colors
#define RED   "\x1B[31m"
#define GREEN "\x1B[32m"
#define RESET "\x1B[0m"

extern int tests_run;
extern int tests_failed;

void UnityBegin(void);
int UnityEnd(void);

#define UNITY_BEGIN() UnityBegin()
#define UNITY_END() UnityEnd()

// Boolean Assertions
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf(RED "FAIL: %s:%d" RESET "\n", __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

// Float Assertions
#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual) \
    do { \
        if (fabs((double)(expected) - (double)(actual)) > (double)(delta)) { \
            printf(RED "FAIL: Expected %f +/- %f but got %f at %s:%d" RESET "\n", \
                   (double)(expected), (double)(delta), (double)(actual), __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

// Integer Assertions
#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    do { \
        int e = (int)(expected); \
        int a = (int)(actual); \
        if (e != a) { \
            printf(RED "FAIL: Expected %d but got %d at %s:%d" RESET "\n", \
                   e, a, __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

// Byte/uint8 Assertions
#define TEST_ASSERT_EQUAL_UINT8(expected, actual) \
    do { \
        unsigned char e = (unsigned char)(expected); \
        unsigned char a = (unsigned char)(actual); \
        if (e != a) { \
            printf(RED "FAIL: Expected %u but got %u at %s:%d" RESET "\n", \
                   (unsigned int)e, (unsigned int)a, __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

// Int32 Assertions
#define TEST_ASSERT_EQUAL_INT32(expected, actual) \
    do { \
        int32_t e = (int32_t)(expected); \
        int32_t a = (int32_t)(actual); \
        if (e != a) { \
            printf(RED "FAIL: Expected %ld but got %ld at %s:%d" RESET "\n", \
                   (long)e, (long)a, __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

// Float Equality (with small epsilon)
#define TEST_ASSERT_EQUAL_FLOAT(expected, actual) \
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, expected, actual)

#define RUN_TEST(test_func) \
    do { \
        extern void setUp(void); \
        extern void tearDown(void); \
        setUp(); \
        printf("Running %s... ", #test_func); \
        int failed_before = tests_failed; \
        test_func(); \
        if (tests_failed == failed_before) { \
            printf(GREEN "PASS" RESET "\n"); \
        } \
        tearDown(); \
        tests_run++; \
    } while (0)

#endif
