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
#include "newton-parser.h"
#include "newton-parser-expression.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton-api.h"

#include "minunit.h"
#include "test-newton-api.h"


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
