#include <check.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/ir.h"

START_TEST(test_to_and_from_string_no_loops) {
        char *program_string = "+.>++-.<";
        struct program p = string_to_program(program_string);
        ck_assert_uint_eq(p.cmds[0].simple_count, 1);
        ck_assert_uint_eq(p.cmds[1].simple_count, 1);
        ck_assert_uint_eq(p.cmds[2].simple_count, 1);
        ck_assert_uint_eq(p.cmds[3].simple_count, 2);
        ck_assert_uint_eq(p.cmds[4].simple_count, 1);
        ck_assert_uint_eq(p.cmds[5].simple_count, 1);
        ck_assert_uint_eq(p.cmds[6].simple_count, 1);
        char *ret = program_to_string(&p);
        ck_assert_str_eq(ret, program_string);
        free(ret);
        free(p.cmds);
}
END_TEST

START_TEST(test_to_and_from_string_with_loops) {
        char *program_string = "++[>++<-]";
        struct program p = string_to_program(program_string);
        char *ret = program_to_string(&p);
        ck_assert_str_eq(ret, program_string);
        free(ret);
        free(p.cmds);
        program_string = "+++[>+++++<-]>";
        p = string_to_program(program_string);
        ck_assert_uint_eq(p.cmds[0].simple_count, 3);
        ck_assert_uint_eq(p.cmds[2].simple_count, 1);
        ck_assert_uint_eq(p.cmds[3].simple_count, 5);
        ck_assert_uint_eq(p.cmds[4].simple_count, 1);
        ck_assert_uint_eq(p.cmds[5].simple_count, 1);
        ck_assert_uint_eq(p.cmds[7].simple_count, 1);
        ret = program_to_string(&p);
        ck_assert_str_eq(ret, program_string);
        free(ret);
        free(p.cmds);
}
END_TEST

START_TEST(test_to_and_from_string_with_nested_loops) {
        char *program_string = "++[>++[>++<-]<-]";
        struct program p = string_to_program(program_string);
        char *ret = program_to_string(&p);
        ck_assert_str_eq(ret, program_string);
        free(ret);
        free(p.cmds);
}
END_TEST

TCase *ir_cases(void) {
        TCase *tc_core;
        tc_core = tcase_create("IR");
        tcase_add_test(tc_core, test_to_and_from_string_no_loops);
        tcase_add_test(tc_core, test_to_and_from_string_with_loops);
        tcase_add_test(tc_core, test_to_and_from_string_with_nested_loops);
        return tc_core;
}
