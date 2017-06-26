#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"

#include "common-timeStamps.h"
#include "data-structures.h"
#include "newton-data-structures.h"
#include "newton-parser.h"
#include "newton-parser-expression.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton-api.h"

#include "minunit.h"
#include "test-driver.h"
#include "test-newton-api.h"
#include "test-common.h"

int tests_run = 0;

char * all_tests()
{
    mu_run_test(test_newtonApiInit_notNull);
    mu_run_test(test_newtonApiInit_notNullInvariant);
    mu_run_test(test_newtonApiGetInvariantByParameters_Valid);
    mu_run_test(test_newtonApiGetPhysicsTypeByName_Valid);

    mu_run_test(test_testNthIrNodeOfType);
    mu_run_test(test_testNthIrNodeOfTypes);
    mu_run_test(test_newtonCheckSingleInvariant);
    mu_run_test(test_newtonApiGetPhysicsTypeByNameAndSubindex_Valid);

    return 0;
}

int main(int argc, char **argv) {
    char *result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
