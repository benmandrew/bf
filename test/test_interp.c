#include <check.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/interp.h"

#define NOFD -1

START_TEST(test_inc_and_dp) {
        char *program = "++>+++><";
        struct context_t ctx = init_context(program);
        ck_assert_int_eq(ctx.data[0], 0);
        int result = interp(&ctx, NOFD, NOFD, false);
        ck_assert_int_eq(ctx.data[0], 1);
        ck_assert_int_eq(result, 0);
        while (!interp(&ctx, NOFD, NOFD, false))
                ;
        ck_assert_str_eq(context_to_string(&ctx), "---\n"
                                                  "    ++>+++><\n"
                                                  "PC:         ^\n"
                                                  "    2 3 0 \n"
                                                  "DP:   ^\n");
}
END_TEST

START_TEST(test_loop) {
        // Multiply 3 by 5
        char *program = "+++[>+++++<-]>";
        struct context_t ctx = init_context(program);
        while (!interp(&ctx, NOFD, NOFD, false))
                ;
        ck_assert_str_eq(context_to_string(&ctx), "---\n"
                                                  "    +++[>+++++<-]>\n"
                                                  "PC:               ^\n"
                                                  "    0 15 \n"
                                                  "DP:   ^\n");
}
END_TEST

START_TEST(test_nested_loop) {
        char *program = "++[>++[>++<-]<-]";
        struct context_t ctx = init_context(program);
        while (!interp(&ctx, NOFD, NOFD, false))
                ;
        ck_assert_str_eq(context_to_string(&ctx), "---\n"
                                                  "    ++[>++[>++<-]<-]\n"
                                                  "PC:                 ^\n"
                                                  "    0 0 8 \n"
                                                  "DP: ^\n");
}
END_TEST

Suite *suite(void) {
        Suite *s;
        TCase *tc_core;

        s = suite_create("InterpTest");
        tc_core = tcase_create("Core");

        tcase_add_test(tc_core, test_inc_and_dp);
        tcase_add_test(tc_core, test_loop);
        tcase_add_test(tc_core, test_nested_loop);
        suite_add_tcase(s, tc_core);

        return s;
}

int main(void) {
        Suite *s = suite();
        SRunner *runner = srunner_create(s);

        srunner_run_all(runner, CK_NORMAL);
        int no_failed = srunner_ntests_failed(runner);
        srunner_free(runner);
        return (no_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
