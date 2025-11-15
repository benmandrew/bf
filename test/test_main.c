#include <stdlib.h>

#include "test_interp.h"
#include "test_ir.h"

int main(void) {
        Suite *s = suite_create("UnitTests");
        suite_add_tcase(s, ir_cases());
        suite_add_tcase(s, interp_cases());
        SRunner *runner = srunner_create(s);
        // srunner_set_fork_status(runner, CK_NOFORK);
        srunner_run_all(runner, CK_NORMAL);
        int no_failed = srunner_ntests_failed(runner);
        srunner_free(runner);
        return (no_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
