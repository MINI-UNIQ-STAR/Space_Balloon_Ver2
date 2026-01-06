#include "unity.h"

int tests_run = 0;
int tests_failed = 0;

void UnityBegin(void) {
    tests_run = 0;
    tests_failed = 0;
}

int UnityEnd(void) {
    printf("-------------------------\n");
    printf("%d Tests Run, %d Failed\n", tests_run, tests_failed);
    return tests_failed;
}
