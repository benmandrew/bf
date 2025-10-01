#include "test_interp.h"

#include <stdio.h>
#include <stdlib.h>

#include "../src/interp.h"
#include "../src/ir.h"

#define NOFD -1

START_TEST(test_inc_and_dp) {
        struct program p = string_to_program("++>+++><");
        struct context_t ctx = init_context(p);
        ck_assert_int_eq(ctx.data[0], 0);
        int result = interp(&ctx, NOFD, NOFD, false);
        ck_assert_int_eq(ctx.data[0], 1);
        ck_assert_int_eq(result, 0);
        while (!interp(&ctx, NOFD, NOFD, false))
                ;
        char *str = context_to_string(&ctx);
        ck_assert_str_eq(str, "---\n"
                              "    ++>+++><\n"
                              "PC:         ^\n"
                              "    2 3 0 \n"
                              "DP:   ^\n");
        free(str);
}
END_TEST

START_TEST(test_loop) {
        // Multiply 3 by 5
        struct program p = string_to_program("+++[>+++++<-]>");
        struct context_t ctx = init_context(p);
        while (!interp(&ctx, NOFD, NOFD, false))
                ;
        char *str = context_to_string(&ctx);
        ck_assert_str_eq(str, "---\n"
                              "    +++[>+++++<-]>\n"
                              "PC:               ^\n"
                              "    0 15 \n"
                              "DP:   ^\n");
        free(str);
}
END_TEST

START_TEST(test_nested_loop) {
        struct program p = string_to_program("++[>++[>++<-]<-]");
        struct context_t ctx = init_context(p);
        while (!interp(&ctx, NOFD, NOFD, false))
                ;
        char *str = context_to_string(&ctx);
        ck_assert_str_eq(str, "---\n"
                              "    ++[>++[>++<-]<-]\n"
                              "PC:                 ^\n"
                              "    0 0 8 \n"
                              "DP: ^\n");
        free(str);
}
END_TEST

TCase *interp_cases(void) {
        TCase *tc_core;
        tc_core = tcase_create("Interp");
        tcase_add_test(tc_core, test_inc_and_dp);
        tcase_add_test(tc_core, test_loop);
        tcase_add_test(tc_core, test_nested_loop);
        return tc_core;
}
