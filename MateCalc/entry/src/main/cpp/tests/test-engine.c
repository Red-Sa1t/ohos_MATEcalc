/*
 * Behaviour tests for the GTK-free calculator engine (port of MathEquation).
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version. See http://www.gnu.org/copyleft/gpl.html the full text of the
 * license.
 */

#include <locale.h>
#include <stdio.h>
#include <string.h>

#include "calc-engine.h"
#include "financial.h"

static int fails = 0, passes = 0;

#define CHECK_STR(label, actual, expected) check_str(__LINE__, label, actual, expected)
#define CHECK_INT(label, actual, expected) check_int(__LINE__, label, actual, expected)

static void
check_str(int line, const char *label, const char *actual, const char *expected)
{
    if (actual && strcmp(actual, expected) == 0)
        passes++;
    else {
        printf("*FAIL line %d: %s -> '%s', expected '%s'\n", line, label, actual ? actual : "(null)", expected);
        fails++;
    }
}

static void
check_int(int line, const char *label, long actual, long expected)
{
    if (actual == expected)
        passes++;
    else {
        printf("*FAIL line %d: %s -> %ld, expected %ld\n", line, label, actual, expected);
        fails++;
    }
}

static void
type(CalcEngine *engine, const char *keys)
{
    /* Each UTF-8 character is a key press like a button click */
    const char *p = keys;

    while (*p) {
        const char *next = g_utf8_next_char(p);
        char key[8];

        memcpy(key, p, (size_t)(next - p));
        key[next - p] = '\0';
        if (key[0] >= '0' && key[0] <= '9' && key[1] == '\0')
            calc_engine_insert_digit(engine, (guint)(key[0] - '0'));
        else if (strcmp(key, "-") == 0)
            calc_engine_insert_subtract(engine);
        else if (strcmp(key, ".") == 0)
            calc_engine_insert_numeric_point(engine);
        else
            calc_engine_insert(engine, key);
        p = next;
    }
}

static void
test_basic(void)
{
    CalcEngine *engine = calc_engine_new(NULL);
    gint start, end;

    type(engine, "12+3×4");
    CHECK_STR("display", calc_engine_get_display(engine), "12+3×4");
    calc_engine_solve(engine);
    CHECK_STR("solve", calc_engine_get_display(engine), "24");
    calc_engine_get_ans_range(engine, &start, &end);
    CHECK_INT("ans start", start, 0);
    CHECK_INT("ans end", end, 2);
    CHECK_INT("is result", calc_engine_is_result(engine), TRUE);

    /* Operator keeps the answer, digit replaces it */
    type(engine, "÷2");
    CHECK_STR("continue with ans", calc_engine_get_display(engine), "24÷2");
    calc_engine_solve(engine);
    CHECK_STR("ans used", calc_engine_get_display(engine), "12");
    type(engine, "7");
    CHECK_STR("digit clears result", calc_engine_get_display(engine), "7");

    /* Undo returns to the previous state, redo reapplies */
    calc_engine_undo(engine);
    CHECK_STR("undo", calc_engine_get_display(engine), "12");
    calc_engine_redo(engine);
    CHECK_STR("redo", calc_engine_get_display(engine), "7");

    /* Solving a result returns to the equation */
    calc_engine_clear(engine);
    type(engine, "2+2");
    calc_engine_solve(engine);
    calc_engine_solve(engine);
    CHECK_STR("solve result undoes", calc_engine_get_display(engine), "2+2");

    /* Missing brackets are closed */
    calc_engine_clear(engine);
    type(engine, "2×(3+4");
    calc_engine_solve(engine);
    CHECK_STR("auto bracket", calc_engine_get_display(engine), "14");

    /* ×× becomes ^ */
    calc_engine_clear(engine);
    type(engine, "2××10");
    CHECK_STR("double multiply", calc_engine_get_display(engine), "2^10");
    calc_engine_solve(engine);
    CHECK_STR("power", calc_engine_get_display(engine), "1024");

    /* Errors go to the status line */
    calc_engine_clear(engine);
    type(engine, "1÷0");
    calc_engine_solve(engine);
    CHECK_STR("division by zero keeps display", calc_engine_get_display(engine), "1÷0");
    CHECK_STR("division by zero status", calc_engine_get_status(engine), "Division by zero is undefined");

    calc_engine_clear(engine);
    type(engine, "foo+1");
    calc_engine_solve(engine);
    CHECK_STR("unknown variable", calc_engine_get_status(engine), "Unknown variable 'foo'");

    /* Status is cleared by the next edit */
    type(engine, "2");
    CHECK_STR("status cleared", calc_engine_get_status(engine), "");

    calc_engine_free(engine);
}

static void
test_editing(void)
{
    CalcEngine *engine = calc_engine_new(NULL);
    gint start, end;

    type(engine, "123");
    calc_engine_set_selection(engine, 1, 1);
    type(engine, "9");
    CHECK_STR("insert at cursor", calc_engine_get_display(engine), "1923");
    calc_engine_get_selection(engine, &start, &end);
    CHECK_INT("cursor after insert", end, 2);

    calc_engine_backspace(engine);
    CHECK_STR("backspace", calc_engine_get_display(engine), "123");
    calc_engine_delete(engine);
    CHECK_STR("delete", calc_engine_get_display(engine), "13");

    calc_engine_set_selection(engine, 0, 2);
    type(engine, "5");
    CHECK_STR("replace selection", calc_engine_get_display(engine), "5");

    calc_engine_replace(engine, 1, 1, "+6");
    CHECK_STR("raw replace", calc_engine_get_display(engine), "5+6");
    calc_engine_set_selection(engine, 0, 1);
    {
        gchar *copy = calc_engine_copy(engine);
        CHECK_STR("copy selection", copy, "5");
        g_free(copy);
    }

    /* Superscript exponent entry */
    calc_engine_clear(engine);
    type(engine, "2");
    calc_engine_insert_exponent(engine);
    CHECK_INT("superscript mode", calc_engine_get_number_mode(engine), SUPERSCRIPT);
    type(engine, "-3");
    CHECK_STR("superscript minus", calc_engine_get_display(engine), "2×10⁻³");
    calc_engine_solve(engine);
    CHECK_STR("exponent value", calc_engine_get_display(engine), "0.002");

    calc_engine_set_number_mode(engine, SUBSCRIPT);
    calc_engine_clear(engine);
    CHECK_INT("clear resets mode", calc_engine_get_number_mode(engine), NORMAL);

    calc_engine_free(engine);
}

static void
test_format(void)
{
    CalcEngine *engine = calc_engine_new(NULL);

    calc_engine_set_thousands_separator(engine, ',');
    calc_engine_set_show_thousands_separators(engine, TRUE);
    type(engine, "1234567");
    CHECK_STR("typed separators", calc_engine_get_display(engine), "1,234,567");
    calc_engine_backspace(engine);
    CHECK_STR("separators after backspace", calc_engine_get_display(engine), "123,456");
    type(engine, "×1000");
    calc_engine_solve(engine);
    CHECK_STR("result separators", calc_engine_get_display(engine), "123,456,000");
    calc_engine_set_show_thousands_separators(engine, FALSE);
    CHECK_STR("reformat ans", calc_engine_get_display(engine), "123456000");

    calc_engine_clear(engine);
    type(engine, "1÷3");
    calc_engine_solve(engine);
    CHECK_STR("default accuracy", calc_engine_get_display(engine), "0.333333333");
    calc_engine_set_accuracy(engine, 3);
    CHECK_STR("accuracy 3", calc_engine_get_display(engine), "0.333");
    calc_engine_set_number_format(engine, MP_DISPLAY_FORMAT_SCIENTIFIC);
    CHECK_STR("scientific", calc_engine_get_display(engine), "3.333×10⁻¹");

    /* Locale radix */
    calc_engine_set_number_format(engine, MP_DISPLAY_FORMAT_AUTOMATIC);
    calc_engine_set_thousands_separator(engine, ' ');
    calc_engine_set_radix(engine, ',');
    calc_engine_clear(engine);
    type(engine, "1.5+1");
    CHECK_STR("radix key", calc_engine_get_display(engine), "1,5+1");
    calc_engine_solve(engine);
    CHECK_STR("radix result", calc_engine_get_display(engine), "2,5");

    calc_engine_free(engine);
}

static void
test_programming(void)
{
    CalcEngine *engine = calc_engine_new(NULL);
    uint64_t bits = 0;
    gchar *label = NULL;

    calc_engine_set_base(engine, 16);
    type(engine, "FF");
    calc_engine_solve(engine);
    CHECK_STR("hex value", calc_engine_get_display(engine), "FF");
    CHECK_INT("bits available", calc_engine_get_bits(engine, &bits, &label), TRUE);
    CHECK_INT("bits", (long)bits, 255);
    CHECK_STR("base label", label, "377₈ = 255₁₀");
    g_free(label);

    calc_engine_toggle_bit(engine, 63);
    CHECK_STR("toggle bit 0", calc_engine_get_display(engine), "FE");
    calc_engine_shift(engine, 4);
    CHECK_STR("shift left", calc_engine_get_display(engine), "FE0");
    calc_engine_shift(engine, -8);
    CHECK_STR("shift right", calc_engine_get_display(engine), "F");

    calc_engine_set_base(engine, 10);
    CHECK_STR("ans in decimal", calc_engine_get_display(engine), "15");

    calc_engine_clear(engine);
    type(engine, "1.5");
    CHECK_INT("fraction has no bits", calc_engine_get_bits(engine, &bits, &label), FALSE);

    calc_engine_clear(engine);
    type(engine, "360");
    calc_engine_factorize(engine);
    CHECK_STR("factorize", calc_engine_get_display(engine), "2^3 × 3^2 × 5");

    calc_engine_clear(engine);
    type(engine, "2.5");
    calc_engine_factorize(engine);
    CHECK_STR("factorize non-integer", calc_engine_get_status(engine), "Need an integer to factorize");

    calc_engine_clear(engine);
    calc_engine_insert_character_code(engine, "A");
    CHECK_STR("character code", calc_engine_get_display(engine), "65");

    calc_engine_free(engine);
}

static void
test_variables_history(void)
{
    CalcEngine *engine = calc_engine_new(NULL);
    gchar *text;
    gchar **names;

    type(engine, "6×7");
    calc_engine_solve(engine);
    calc_engine_store(engine, "x");
    text = calc_engine_get_variable_text(engine, "x");
    CHECK_STR("stored variable", text, "42");
    g_free(text);

    calc_engine_clear(engine);
    calc_engine_recall(engine, "x");
    type(engine, "+1");
    calc_engine_solve(engine);
    CHECK_STR("recall variable", calc_engine_get_display(engine), "43");

    calc_engine_clear(engine);
    type(engine, "y=5");
    calc_engine_solve(engine);
    names = calc_engine_get_variable_names(engine);
    CHECK_INT("assignment creates variable", g_strv_length(names), 2);
    g_strfreev(names);
    calc_engine_delete_variable(engine, "y");
    names = calc_engine_get_variable_names(engine);
    CHECK_INT("variable deleted", g_strv_length(names), 1);
    g_strfreev(names);

    /* History: 6×7, x+1, y=5 */
    CHECK_INT("history count", calc_engine_get_history_count(engine), 3);
    CHECK_STR("history equation", calc_engine_get_history_equation(engine, 0), "6×7");
    text = calc_engine_get_history_answer(engine, 1);
    CHECK_STR("history answer", text, "43");
    g_free(text);

    calc_engine_clear(engine);
    calc_engine_history_set_equation(engine, 0);
    CHECK_STR("history set equation", calc_engine_get_display(engine), "6×7");
    type(engine, "+");
    calc_engine_history_insert_answer(engine, 1);
    CHECK_STR("history insert answer", calc_engine_get_display(engine), "6×7+43");

    calc_engine_clear_history(engine);
    CHECK_INT("history cleared", calc_engine_get_history_count(engine), 0);

    calc_engine_free(engine);
}

static void
test_converter_financial(void)
{
    CalcEngine *engine = calc_engine_new(NULL);
    gchar *source = NULL, *target = NULL;
    const gchar *args[4] = {"1000", "100", "9", NULL};

    type(engine, "10");
    CHECK_INT("convert", calc_engine_convert(engine, "length", "kilometer", "meter", &source, &target), TRUE);
    CHECK_STR("convert source", source, "10 km");
    CHECK_STR("convert target", target, "10000 m");
    g_free(source);
    g_free(target);
    CHECK_INT("apply conversion", calc_engine_apply_conversion(engine, "length", "kilometer", "meter"), TRUE);
    CHECK_STR("converted display", calc_engine_get_display(engine), "10000");

    /* Currency needs rates */
    CHECK_INT("currency without rates", calc_engine_convert(engine, "currency", "EUR", "USD", &source, &target), FALSE);
    calc_engine_load_currency_rates(engine, "\nEuro\t0.8\nU.S. dollar\t0.5\n", NULL);
    CHECK_INT("currency with rates", calc_engine_convert(engine, "currency", "EUR", "USD", &source, &target), TRUE);
    CHECK_STR("currency target", target, "$16000");
    g_free(source);
    g_free(target);

    calc_engine_clear(engine);
    type(engine, "20 km in m");
    calc_engine_solve(engine);
    CHECK_STR("conversion expression", calc_engine_get_display(engine), "20000");

    calc_engine_financial(engine, FINC_SLN_DIALOG, args);
    CHECK_STR("financial sln", calc_engine_get_display(engine), "100");

    {
        const gchar *ddb[4] = {"1000", "5", "-1", NULL};
        calc_engine_financial(engine, FINC_DDB_DIALOG, ddb);
        CHECK_STR("financial error status", calc_engine_get_status(engine), "Error: the number of periods must be positive");
    }

    calc_engine_free(engine);
}

int
main(void)
{
    setlocale(LC_ALL, "C");

    test_basic();
    test_editing();
    test_format();
    test_programming();
    test_variables_history();
    test_converter_financial();

    if (fails == 0)
        printf("Passed all %i tests\n", passes);
    return fails;
}
