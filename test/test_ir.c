#include "test_ir.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/ir.h"

START_TEST(test_to_and_from_string_no_loops) {
        char *program_string = "+.>++-.<";
        struct program p = string_to_program(program_string);
        ck_assert_uint_eq(p.cmds[0].value.simple_count, 1);
        ck_assert_uint_eq(p.cmds[1].value.simple_count, 1);
        ck_assert_uint_eq(p.cmds[2].value.simple_count, 1);
        ck_assert_uint_eq(p.cmds[3].value.simple_count, 2);
        ck_assert_uint_eq(p.cmds[4].value.simple_count, 1);
        ck_assert_uint_eq(p.cmds[5].value.simple_count, 1);
        ck_assert_uint_eq(p.cmds[6].value.simple_count, 1);
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
        ck_assert_uint_eq(p.cmds[0].value.simple_count, 3);
        ck_assert_uint_eq(p.cmds[2].value.simple_count, 1);
        ck_assert_uint_eq(p.cmds[3].value.simple_count, 5);
        ck_assert_uint_eq(p.cmds[4].value.simple_count, 1);
        ck_assert_uint_eq(p.cmds[5].value.simple_count, 1);
        ck_assert_uint_eq(p.cmds[7].value.simple_count, 1);
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

START_TEST(test_range_to_label_expands_counts) {
        struct program p = string_to_program("+++>><");
        char label[32];
        program_range_to_label(&p, 0, p.length, label, sizeof(label));
        ck_assert_str_eq(label, "+++>><");
        free(p.cmds);
}
END_TEST

START_TEST(test_range_to_label_honours_bounds) {
        struct program p = string_to_program("+++>><");
        char label[32];
        program_range_to_label(&p, 1, 2, label, sizeof(label));
        ck_assert_str_eq(label, ">>");
        program_range_to_label(&p, 1, 1, label, sizeof(label));
        ck_assert_str_eq(label, "");
        free(p.cmds);
}
END_TEST

START_TEST(test_range_to_label_renders_synthetic_cmds) {
        // program_to_string() skips these; the label renderer must not.
        struct program p = string_to_program("+[-]");
        optimise_program(&p);
        char label[32];
        program_range_to_label(&p, 0, p.length, label, sizeof(label));
        ck_assert_str_eq(label, "+[-]");
        free(p.cmds);

        p = string_to_program("+[>+<-]");
        optimise_program(&p);
        program_range_to_label(&p, 0, p.length, label, sizeof(label));
        ck_assert_str_eq(label, "+[mul]");
        free(p.cmds);
}
END_TEST

START_TEST(test_range_to_label_truncates) {
        struct program p = string_to_program("++++++++++++++++++++");
        char label[8];
        program_range_to_label(&p, 0, p.length, label, sizeof(label));
        ck_assert_str_eq(label, "++++...");
        ck_assert_uint_eq(strlen(label), sizeof(label) - 1);
        free(p.cmds);
}
END_TEST

START_TEST(test_range_to_label_exact_fit_not_truncated) {
        struct program p = string_to_program("+++");
        char label[4];
        program_range_to_label(&p, 0, p.length, label, sizeof(label));
        ck_assert_str_eq(label, "+++");
        free(p.cmds);
}
END_TEST

TCase *ir_cases(void) {
        TCase *tc_core;
        tc_core = tcase_create("IR");
        tcase_add_test(tc_core, test_to_and_from_string_no_loops);
        tcase_add_test(tc_core, test_to_and_from_string_with_loops);
        tcase_add_test(tc_core, test_to_and_from_string_with_nested_loops);
        tcase_add_test(tc_core, test_range_to_label_expands_counts);
        tcase_add_test(tc_core, test_range_to_label_honours_bounds);
        tcase_add_test(tc_core, test_range_to_label_renders_synthetic_cmds);
        tcase_add_test(tc_core, test_range_to_label_truncates);
        tcase_add_test(tc_core, test_range_to_label_exact_fit_not_truncated);
        return tc_core;
}
