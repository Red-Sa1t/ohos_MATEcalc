/*
 * Tests for the parts of the calculator core that the HarmonyOS port changed:
 * exchange-rate loading (IMF TSV / ECB XML without GIO/libxml2), financial
 * functions decoupled from MathEquation, and variable persistence.
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

#include "currency-manager.h"
#include "financial.h"
#include "math-variables.h"
#include "mp-equation.h"
#include "mp-serializer.h"
#include "unit-manager.h"

static int fails = 0, passes = 0;

static void
check_number(const char *label, const MPNumber *x, const char *expected)
{
    MpSerializer *serializer = mp_serializer_new(MP_DISPLAY_FORMAT_FIXED, 10, 9);
    char *s = mp_serializer_to_string(serializer, x);

    if (strcmp(s, expected) == 0)
        passes++;
    else {
        printf("*FAIL: %s -> %s, expected %s\n", label, s, expected);
        fails++;
    }
    g_free(s);
    g_object_unref(serializer);
}

static int
convert(const MPNumber *x, const char *x_units, const char *z_units, MPNumber *z, void *data)
{
    return unit_manager_convert_by_symbol(unit_manager_get_default(), x, x_units, z_units, z);
}

static void
check_expression(const char *expression, const char *expected)
{
    MPEquationOptions options;
    MPNumber result = mp_new();
    char *error_token = NULL;
    MPErrorCode error;

    memset(&options, 0, sizeof(options));
    options.base = 10;
    options.wordlen = 32;
    options.angle_units = MP_DEGREES;
    options.convert = convert;

    error = mp_equation_parse(expression, &options, &result, &error_token);
    if (error != PARSER_ERR_NONE) {
        printf("*FAIL: '%s' -> error %s, expected %s\n", expression, mp_error_code_to_string(error), expected);
        fails++;
    } else
        check_number(expression, &result, expected);
    g_free(error_token);
    mp_clear(&result);
}

static const char *imf_data =
    "Representative Exchange Rates for Selected Currencies for October 2026\n"
    "\n"
    "Currency\tOctober 15, 2026\tOctober 14, 2026\n"
    "Euro\t0.8\t0.81\n"
    "U.S. dollar\t\t0.75\n"
    "Chinese yuan\t5\t5.1\n"
    "\n"
    "Euro\t99\n";

static const char *ecb_data =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gesmes:Envelope xmlns:gesmes=\"http://www.gesmes.org/xml/2002-08-01\" "
    "xmlns=\"http://www.ecb.int/vocabulary/2002-08-01/eurofxref\">\n"
    "  <Cube>\n"
    "    <Cube time='2026-10-15'>\n"
    "      <Cube currency='USD' rate='2'/>\n"
    "      <Cube currency='BGN' rate='1.9558'/>\n"
    "      <Cube rate=\"4\" currency=\"PLN\"/>\n"
    "    </Cube>\n"
    "  </Cube>\n"
    "</gesmes:Envelope>\n";

static void
test_currency(void)
{
    CurrencyManager *manager = currency_manager_get_default();

    /* Without rates, currency conversion is not possible */
    if (currency_manager_has_rates(manager) || currency_manager_get_value(manager, "EUR") != NULL) {
        printf("*FAIL: rates available before loading\n");
        fails++;
    } else
        passes++;

    currency_manager_load_rates(manager, imf_data, ecb_data);

    /* IMF values are reciprocals; rates after the second blank line are ignored */
    check_number("EUR value", currency_manager_get_value(manager, "EUR"), "1.25");
    /* Empty columns are skipped */
    check_number("USD value", currency_manager_get_value(manager, "USD"), "1.333333333");
    /* ECB only tops up currencies the IMF did not provide */
    check_number("BGN value", currency_manager_get_value(manager, "BGN"), "2.44475");
    check_number("PLN value", currency_manager_get_value(manager, "PLN"), "5");
    /* Fixed ECB rate */
    check_number("EEK value", currency_manager_get_value(manager, "EEK"), "19.558754499");

    check_expression("10 EUR in USD", "10.666666667");
    check_expression("1 EUR in BGN", "1.9558");
    check_expression("4 CNY in EUR", "25");

    /* Reloading replaces previous rates */
    currency_manager_load_rates(manager, "\nEuro\t0.5\n", NULL);
    check_number("EUR reloaded", currency_manager_get_value(manager, "EUR"), "2");
    if (currency_manager_get_value(manager, "USD") != NULL) {
        printf("*FAIL: USD rate kept after reload\n");
        fails++;
    } else
        passes++;
}

static void
test_financial(void)
{
    MPNumber a = mp_new(), b = mp_new(), c = mp_new(), d = mp_new(), result = mp_new();
    const char *error;

    /* Straight-line depreciation: (1000 - 100) / 9 */
    mp_set_from_integer(1000, &a);
    mp_set_from_integer(100, &b);
    mp_set_from_integer(9, &c);
    error = do_finc_expression(FINC_SLN_DIALOG, &a, &b, &c, &d, &result);
    check_number("sln", &result, "100");
    if (error) { printf("*FAIL: sln error %s\n", error); fails++; }

    /* Double-declining balance, first period: 2 * 1000 / 5 */
    mp_set_from_integer(1000, &a);
    mp_set_from_integer(5, &b);
    mp_set_from_integer(1, &c);
    error = do_finc_expression(FINC_DDB_DIALOG, &a, &b, &c, &d, &result);
    check_number("ddb", &result, "400");
    if (error) { printf("*FAIL: ddb error %s\n", error); fails++; }

    /* Negative period is rejected */
    mp_set_from_integer(-1, &c);
    error = do_finc_expression(FINC_DDB_DIALOG, &a, &b, &c, &d, &result);
    if (!error) { printf("*FAIL: ddb accepted negative period\n"); fails++; } else passes++;

    mp_clear(&a); mp_clear(&b); mp_clear(&c); mp_clear(&d); mp_clear(&result);
}

static void
test_variables(void)
{
    const char *path = "build/test-registers";
    MathVariables *variables;
    MPNumber x = mp_new();
    MPNumber *value;
    gchar **names;

    remove(path);
    variables = math_variables_new(path);
    mp_set_from_string("1.5", 10, &x);
    math_variables_set(variables, "rate", &x);
    mp_set_from_integer(7, &x);
    math_variables_set(variables, "n", &x);
    math_variables_set(variables, "n", &x);

    /* A new instance reads the persisted values back */
    variables = math_variables_new(path);
    value = math_variables_get(variables, "rate");
    if (value) check_number("variable rate", value, "1.5"); else { printf("*FAIL: rate not persisted\n"); fails++; }
    value = math_variables_get(variables, "n");
    if (value) check_number("variable n", value, "7"); else { printf("*FAIL: n not persisted\n"); fails++; }

    /* Values written in scientific notation are read back (upstream lost them) */
    mp_set_from_string("42", 10, &x);
    math_variables_set(variables, "big", &x);
    mp_set_from_string("-0.00015", 10, &x);
    math_variables_set(variables, "small", &x);
    variables = math_variables_new(path);
    value = math_variables_get(variables, "big");
    if (value) check_number("variable big", value, "42"); else { printf("*FAIL: big not persisted\n"); fails++; }
    value = math_variables_get(variables, "small");
    if (value) check_number("variable small", value, "−0.00015"); else { printf("*FAIL: small not persisted\n"); fails++; }
    math_variables_delete(variables, "big");
    math_variables_delete(variables, "small");

    names = math_variables_get_names(variables);
    if (g_strv_length(names) == 2) passes++; else { printf("*FAIL: expected 2 variables, got %u\n", g_strv_length(names)); fails++; }
    g_strfreev(names);

    math_variables_delete(variables, "rate");
    variables = math_variables_new(path);
    if (math_variables_get(variables, "rate") == NULL) passes++; else { printf("*FAIL: deleted variable persisted\n"); fails++; }

    mp_clear(&x);
}

int
main(void)
{
    setlocale(LC_ALL, "C");

    test_currency();
    test_financial();
    test_variables();

    if (fails == 0)
        printf("Passed all %i tests\n", passes);
    return fails;
}
