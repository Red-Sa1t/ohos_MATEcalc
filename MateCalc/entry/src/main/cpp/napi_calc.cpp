/*
 * N-API bindings exposing the MATE Calculator engine to ArkTS.
 *
 * A single engine instance backs the calculator window. Text offsets cross the
 * boundary as UTF-16 code units (ArkTS string indices) and are converted to the
 * engine's code point offsets here. Solving and factorizing run on a worker
 * thread and resolve a Promise with the new state.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version. See http://www.gnu.org/copyleft/gpl.html the full text of the
 * license.
 */

#include <napi/native_api.h>

#include <cstdint>
#include <string>
#include <vector>

/* The multi-precision headers declare their own C++ API and must be included
 * outside the extern "C" block below. */
#include <gmp.h>
#include <mpfr.h>
#include <mpc.h>

extern "C" {
#include "calc-engine.h"
#include "currency-manager.h"
#include "unit-manager.h"
}

namespace {

CalcEngine *g_engine = nullptr;

CalcEngine *Engine()
{
    if (g_engine == nullptr) {
        g_engine = calc_engine_new(nullptr);
    }
    return g_engine;
}

/* Value helpers */

std::string GetString(napi_env env, napi_value value)
{
    size_t length = 0;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) {
        return "";
    }
    std::string result(length, '\0');
    napi_get_value_string_utf8(env, value, &result[0], length + 1, &length);
    result.resize(length);
    return result;
}

int32_t GetInt(napi_env env, napi_value value, int32_t fallback = 0)
{
    int32_t result = fallback;
    napi_get_value_int32(env, value, &result);
    return result;
}

bool GetBool(napi_env env, napi_value value)
{
    bool result = false;
    napi_get_value_bool(env, value, &result);
    return result;
}

napi_value MakeString(napi_env env, const char *text)
{
    napi_value result;
    napi_create_string_utf8(env, text != nullptr ? text : "", NAPI_AUTO_LENGTH, &result);
    return result;
}

napi_value MakeInt(napi_env env, int64_t number)
{
    napi_value result;
    napi_create_int64(env, number, &result);
    return result;
}

napi_value MakeBool(napi_env env, bool flag)
{
    napi_value result;
    napi_get_boolean(env, flag, &result);
    return result;
}

napi_value Undefined(napi_env env)
{
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

void SetProperty(napi_env env, napi_value object, const char *name, napi_value value)
{
    napi_set_named_property(env, object, name, value);
}

std::vector<napi_value> GetArgs(napi_env env, napi_callback_info info, size_t expected)
{
    size_t argc = expected;
    std::vector<napi_value> args(expected);
    napi_get_cb_info(env, info, &argc, expected > 0 ? args.data() : nullptr, nullptr, nullptr);
    if (argc < expected) {
        for (size_t i = argc; i < expected; i++) {
            args[i] = Undefined(env);
        }
    }
    return args;
}

uint32_t FirstCodePoint(const std::string &text)
{
    gunichar c = g_utf8_get_char_validated(text.c_str(), static_cast<gssize>(text.size()));
    return (c == static_cast<gunichar>(-1) || c == static_cast<gunichar>(-2)) ? 0 : c;
}

/* Offset conversion between UTF-16 code units and code points */

int32_t CodePointsToUtf16(const char *text, int32_t offset)
{
    if (offset < 0) {
        return offset;
    }
    int32_t units = 0;
    const char *p = text;
    for (int32_t i = 0; i < offset && *p != '\0'; i++) {
        units += g_utf8_get_char(p) > 0xFFFF ? 2 : 1;
        p = g_utf8_next_char(p);
    }
    return units;
}

int32_t Utf16ToCodePoints(const char *text, int32_t units)
{
    if (units <= 0) {
        return 0;
    }
    int32_t points = 0;
    int32_t consumed = 0;
    const char *p = text;
    while (*p != '\0' && consumed < units) {
        consumed += g_utf8_get_char(p) > 0xFFFF ? 2 : 1;
        p = g_utf8_next_char(p);
        points++;
    }
    return points;
}

/* State snapshot returned to ArkTS after every operation */
napi_value MakeState(napi_env env)
{
    CalcEngine *engine = Engine();
    const char *display = calc_engine_get_display(engine);
    gint start = 0;
    gint end = 0;
    gint ansStart = -1;
    gint ansEnd = -1;

    calc_engine_get_selection(engine, &start, &end);
    calc_engine_get_ans_range(engine, &ansStart, &ansEnd);

    napi_value state;
    napi_create_object(env, &state);
    SetProperty(env, state, "display", MakeString(env, display));
    SetProperty(env, state, "selectionStart", MakeInt(env, CodePointsToUtf16(display, start)));
    SetProperty(env, state, "selectionEnd", MakeInt(env, CodePointsToUtf16(display, end)));
    SetProperty(env, state, "ansStart", MakeInt(env, CodePointsToUtf16(display, ansStart)));
    SetProperty(env, state, "ansEnd", MakeInt(env, CodePointsToUtf16(display, ansEnd)));
    SetProperty(env, state, "status", MakeString(env, calc_engine_get_status(engine)));
    SetProperty(env, state, "numberMode", MakeInt(env, calc_engine_get_number_mode(engine)));
    SetProperty(env, state, "isResult", MakeBool(env, calc_engine_is_result(engine)));
    SetProperty(env, state, "inSolve", MakeBool(env, calc_engine_in_solve(engine)));
    SetProperty(env, state, "revision", MakeInt(env, calc_engine_get_revision(engine)));
    return state;
}

/* Lifecycle */

napi_value Init(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    std::string variablesFile = GetString(env, args[0]);
    if (g_engine != nullptr) {
        calc_engine_free(g_engine);
    }
    g_engine = calc_engine_new(variablesFile.empty() ? nullptr : variablesFile.c_str());
    return MakeState(env);
}

napi_value GetState(napi_env env, napi_callback_info info)
{
    return MakeState(env);
}

/* Editing */

napi_value Insert(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    std::string text = GetString(env, args[0]);
    calc_engine_insert(Engine(), text.c_str());
    return MakeState(env);
}

napi_value InsertDigit(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    int32_t digit = GetInt(env, args[0], -1);
    if (digit >= 0 && digit < 16) {
        calc_engine_insert_digit(Engine(), static_cast<guint>(digit));
    }
    return MakeState(env);
}

napi_value InsertNumericPoint(napi_env env, napi_callback_info info)
{
    calc_engine_insert_numeric_point(Engine());
    return MakeState(env);
}

napi_value InsertExponent(napi_env env, napi_callback_info info)
{
    calc_engine_insert_exponent(Engine());
    return MakeState(env);
}

napi_value InsertSubtract(napi_env env, napi_callback_info info)
{
    calc_engine_insert_subtract(Engine());
    return MakeState(env);
}

napi_value InsertCharacterCode(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    std::string text = GetString(env, args[0]);
    calc_engine_insert_character_code(Engine(), text.c_str());
    return MakeState(env);
}

napi_value Replace(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 3);
    const char *display = calc_engine_get_display(Engine());
    int32_t start = Utf16ToCodePoints(display, GetInt(env, args[0]));
    int32_t end = Utf16ToCodePoints(display, GetInt(env, args[1]));
    std::string text = GetString(env, args[2]);
    calc_engine_replace(Engine(), start, end, text.c_str());
    return MakeState(env);
}

napi_value SetSelection(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 2);
    const char *display = calc_engine_get_display(Engine());
    calc_engine_set_selection(Engine(), Utf16ToCodePoints(display, GetInt(env, args[0])),
                              Utf16ToCodePoints(display, GetInt(env, args[1])));
    return MakeState(env);
}

napi_value Backspace(napi_env env, napi_callback_info info)
{
    calc_engine_backspace(Engine());
    return MakeState(env);
}

napi_value DeleteChar(napi_env env, napi_callback_info info)
{
    calc_engine_delete(Engine());
    return MakeState(env);
}

napi_value Clear(napi_env env, napi_callback_info info)
{
    calc_engine_clear(Engine());
    return MakeState(env);
}

napi_value Undo(napi_env env, napi_callback_info info)
{
    calc_engine_undo(Engine());
    return MakeState(env);
}

napi_value Redo(napi_env env, napi_callback_info info)
{
    calc_engine_redo(Engine());
    return MakeState(env);
}

napi_value Copy(napi_env env, napi_callback_info info)
{
    gchar *text = calc_engine_copy(Engine());
    napi_value result = MakeString(env, text);
    g_free(text);
    return result;
}

napi_value SetNumberMode(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    calc_engine_set_number_mode(Engine(), static_cast<NumberMode>(GetInt(env, args[0])));
    return MakeState(env);
}

napi_value SetStatus(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    std::string status = GetString(env, args[0]);
    calc_engine_set_status(Engine(), status.c_str());
    return MakeState(env);
}

/* Solving on a worker thread */

struct JobContext {
    CalcJob *job = nullptr;
    napi_deferred deferred = nullptr;
    napi_async_work work = nullptr;
};

void ExecuteJob(napi_env env, void *data)
{
    auto *context = static_cast<JobContext *>(data);
    calc_job_compute(context->job);
}

void CompleteJob(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<JobContext *>(data);
    calc_engine_job_finish(Engine(), context->job);
    napi_resolve_deferred(env, context->deferred, MakeState(env));
    napi_delete_async_work(env, context->work);
    delete context;
}

napi_value StartJob(napi_env env, CalcJob *job)
{
    napi_value promise;
    napi_deferred deferred;
    napi_create_promise(env, &deferred, &promise);

    if (job == nullptr) {
        napi_resolve_deferred(env, deferred, MakeState(env));
        return promise;
    }

    auto *context = new JobContext();
    context->job = job;
    context->deferred = deferred;
    napi_value resourceName = MakeString(env, "MateCalcSolve");
    napi_create_async_work(env, nullptr, resourceName, ExecuteJob, CompleteJob, context, &context->work);
    napi_queue_async_work(env, context->work);
    return promise;
}

napi_value Solve(napi_env env, napi_callback_info info)
{
    return StartJob(env, calc_engine_solve_prepare(Engine()));
}

napi_value Factorize(napi_env env, napi_callback_info info)
{
    return StartJob(env, calc_engine_factorize_prepare(Engine()));
}

/* Settings */

napi_value SetSettings(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    napi_value settings = args[0];
    CalcEngine *engine = Engine();
    bool has = false;
    napi_value value;

    auto read = [&](const char *name) -> bool {
        has = false;
        napi_has_named_property(env, settings, name, &has);
        if (!has) {
            return false;
        }
        napi_get_named_property(env, settings, name, &value);
        napi_valuetype type;
        napi_typeof(env, value, &type);
        return type != napi_undefined && type != napi_null;
    };

    /* Locale characters first so the reformat uses them */
    if (read("thousandsSeparator")) {
        uint32_t c = FirstCodePoint(GetString(env, value));
        if (c != 0) {
            calc_engine_set_thousands_separator(engine, c);
        }
    }
    if (read("radix")) {
        uint32_t c = FirstCodePoint(GetString(env, value));
        if (c != 0) {
            calc_engine_set_radix(engine, c);
        }
    }
    if (read("accuracy")) {
        calc_engine_set_accuracy(engine, GetInt(env, value, 9));
    }
    if (read("showThousandsSeparators")) {
        calc_engine_set_show_thousands_separators(engine, GetBool(env, value));
    }
    if (read("showTrailingZeroes")) {
        calc_engine_set_show_trailing_zeroes(engine, GetBool(env, value));
    }
    if (read("numberFormat")) {
        calc_engine_set_number_format(engine, static_cast<MpDisplayFormat>(GetInt(env, value)));
    }
    if (read("base")) {
        calc_engine_set_base(engine, GetInt(env, value, 10));
    }
    if (read("wordSize")) {
        calc_engine_set_word_size(engine, GetInt(env, value, 32));
    }
    if (read("angleUnits")) {
        calc_engine_set_angle_units(engine, static_cast<MPAngleUnit>(GetInt(env, value)));
    }
    return MakeState(env);
}

/* Programmer mode */

napi_value Shift(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    calc_engine_shift(Engine(), GetInt(env, args[0]));
    return MakeState(env);
}

napi_value ToggleBit(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    int32_t bit = GetInt(env, args[0], -1);
    if (bit >= 0 && bit < 64) {
        calc_engine_toggle_bit(Engine(), static_cast<guint>(bit));
    }
    return MakeState(env);
}

napi_value GetBits(napi_env env, napi_callback_info info)
{
    uint64_t bits = 0;
    gchar *label = nullptr;
    bool enabled = calc_engine_get_bits(Engine(), &bits, &label);

    /* 64 character string, most significant bit first (bit index 0) */
    std::string binary(64, '0');
    if (enabled) {
        for (int i = 0; i < 64; i++) {
            if (bits & (1ULL << (63 - i))) {
                binary[i] = '1';
            }
        }
    }

    napi_value result;
    napi_create_object(env, &result);
    SetProperty(env, result, "enabled", MakeBool(env, enabled));
    SetProperty(env, result, "bits", MakeString(env, binary.c_str()));
    SetProperty(env, result, "label", MakeString(env, label));
    g_free(label);
    return result;
}

/* Variables */

napi_value Store(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    std::string name = GetString(env, args[0]);
    calc_engine_store(Engine(), name.c_str());
    return MakeState(env);
}

napi_value Recall(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    std::string name = GetString(env, args[0]);
    calc_engine_recall(Engine(), name.c_str());
    return MakeState(env);
}

napi_value GetVariables(napi_env env, napi_callback_info info)
{
    gchar **names = calc_engine_get_variable_names(Engine());
    napi_value result;
    napi_create_array(env, &result);
    uint32_t index = 0;
    for (gchar **name = names; name != nullptr && *name != nullptr; name++) {
        gchar *text = calc_engine_get_variable_text(Engine(), *name);
        napi_value item;
        napi_create_object(env, &item);
        SetProperty(env, item, "name", MakeString(env, *name));
        SetProperty(env, item, "value", MakeString(env, text));
        napi_set_element(env, result, index++, item);
        g_free(text);
    }
    g_strfreev(names);
    return result;
}

napi_value DeleteVariable(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    std::string name = GetString(env, args[0]);
    calc_engine_delete_variable(Engine(), name.c_str());
    return Undefined(env);
}

napi_value GetAnswer(napi_env env, napi_callback_info info)
{
    gchar *text = calc_engine_get_answer_text(Engine());
    napi_value result = MakeString(env, text);
    g_free(text);
    return result;
}

/* History */

napi_value GetHistory(napi_env env, napi_callback_info info)
{
    CalcEngine *engine = Engine();
    guint count = calc_engine_get_history_count(engine);
    napi_value result;
    napi_create_array_with_length(env, count, &result);
    for (guint i = 0; i < count; i++) {
        gchar *answer = calc_engine_get_history_answer(engine, i);
        napi_value item;
        napi_create_object(env, &item);
        SetProperty(env, item, "equation", MakeString(env, calc_engine_get_history_equation(engine, i)));
        SetProperty(env, item, "answer", MakeString(env, answer));
        napi_set_element(env, result, i, item);
        g_free(answer);
    }
    return result;
}

napi_value HistoryInsertAnswer(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    int32_t index = GetInt(env, args[0], -1);
    if (index >= 0 && static_cast<guint>(index) < calc_engine_get_history_count(Engine())) {
        calc_engine_history_insert_answer(Engine(), static_cast<guint>(index));
    }
    return MakeState(env);
}

napi_value HistorySetEquation(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 1);
    int32_t index = GetInt(env, args[0], -1);
    if (index >= 0 && static_cast<guint>(index) < calc_engine_get_history_count(Engine())) {
        calc_engine_history_set_equation(Engine(), static_cast<guint>(index));
    }
    return MakeState(env);
}

napi_value ClearHistory(napi_env env, napi_callback_info info)
{
    calc_engine_clear_history(Engine());
    return Undefined(env);
}

/* Units and currencies */

napi_value GetUnitCategories(napi_env env, napi_callback_info info)
{
    napi_value result;
    napi_create_array(env, &result);
    uint32_t categoryIndex = 0;
    for (const GList *c = unit_manager_get_categories(unit_manager_get_default()); c != nullptr; c = c->next) {
        auto *category = static_cast<UnitCategory *>(c->data);
        napi_value item;
        napi_create_object(env, &item);
        SetProperty(env, item, "name", MakeString(env, unit_category_get_name(category)));
        SetProperty(env, item, "displayName", MakeString(env, unit_category_get_display_name(category)));

        napi_value units;
        napi_create_array(env, &units);
        uint32_t unitIndex = 0;
        for (const GList *u = unit_category_get_units(category); u != nullptr; u = u->next) {
            auto *unit = static_cast<Unit *>(u->data);
            napi_value unitItem;
            napi_create_object(env, &unitItem);
            SetProperty(env, unitItem, "name", MakeString(env, unit_get_name(unit)));
            SetProperty(env, unitItem, "displayName", MakeString(env, unit_get_display_name(unit)));
            napi_set_element(env, units, unitIndex++, unitItem);
        }
        SetProperty(env, item, "units", units);
        napi_set_element(env, result, categoryIndex++, item);
    }
    return result;
}

napi_value Convert(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 3);
    std::string category = GetString(env, args[0]);
    std::string source = GetString(env, args[1]);
    std::string target = GetString(env, args[2]);
    gchar *sourceText = nullptr;
    gchar *targetText = nullptr;
    bool ok = calc_engine_convert(Engine(), category.c_str(), source.c_str(), target.c_str(),
                                  &sourceText, &targetText);

    napi_value result;
    napi_create_object(env, &result);
    SetProperty(env, result, "ok", MakeBool(env, ok));
    SetProperty(env, result, "source", MakeString(env, ok ? sourceText : ""));
    SetProperty(env, result, "target", MakeString(env, ok ? targetText : ""));
    g_free(sourceText);
    g_free(targetText);
    return result;
}

napi_value ApplyConversion(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 3);
    std::string category = GetString(env, args[0]);
    std::string source = GetString(env, args[1]);
    std::string target = GetString(env, args[2]);
    calc_engine_apply_conversion(Engine(), category.c_str(), source.c_str(), target.c_str());
    return MakeState(env);
}

napi_value LoadCurrencyRates(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 2);
    std::string imf = GetString(env, args[0]);
    std::string ecb = GetString(env, args[1]);
    calc_engine_load_currency_rates(Engine(), imf.empty() ? nullptr : imf.c_str(),
                                    ecb.empty() ? nullptr : ecb.c_str());
    return MakeState(env);
}

napi_value HasCurrencyRates(napi_env env, napi_callback_info info)
{
    return MakeBool(env, currency_manager_has_rates(currency_manager_get_default()));
}

/* Financial */

napi_value Financial(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 2);
    int32_t function = GetInt(env, args[0], -1);
    std::string values[4];
    const gchar *argv[4] = {nullptr, nullptr, nullptr, nullptr};

    bool isArray = false;
    napi_is_array(env, args[1], &isArray);
    if (isArray) {
        uint32_t length = 0;
        napi_get_array_length(env, args[1], &length);
        for (uint32_t i = 0; i < length && i < 4; i++) {
            napi_value element;
            napi_get_element(env, args[1], i, &element);
            values[i] = GetString(env, element);
            argv[i] = values[i].c_str();
        }
    }
    if (function >= 0) {
        calc_engine_financial(Engine(), function, argv);
    }
    return MakeState(env);
}

/* Install the upstream translation catalog used by _() in the C core.
 * Must be called before the first unit/currency lookup. */
napi_value SetTranslations(napi_env env, napi_callback_info info)
{
    auto args = GetArgs(env, info, 2);
    uint32_t length = 0;
    napi_get_array_length(env, args[0], &length);
    g_compat_clear_translations();
    for (uint32_t i = 0; i < length; i++) {
        napi_value key;
        napi_value value;
        napi_get_element(env, args[0], i, &key);
        napi_get_element(env, args[1], i, &value);
        std::string k = GetString(env, key);
        std::string v = GetString(env, value);
        g_compat_set_translation(k.c_str(), v.c_str());
    }
    return Undefined(env);
}

napi_value GetLibraryVersions(napi_env env, napi_callback_info info)
{
    napi_value result;
    napi_create_object(env, &result);
    SetProperty(env, result, "mpfr", MakeString(env, mpfr_get_version()));
    SetProperty(env, result, "mpc", MakeString(env, mpc_get_version()));
    return result;
}

} // namespace

EXTERN_C_START
static napi_value RegisterModule(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        {"init", nullptr, Init, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getState", nullptr, GetState, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"insert", nullptr, Insert, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"insertDigit", nullptr, InsertDigit, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"insertNumericPoint", nullptr, InsertNumericPoint, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"insertExponent", nullptr, InsertExponent, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"insertSubtract", nullptr, InsertSubtract, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"insertCharacterCode", nullptr, InsertCharacterCode, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"replace", nullptr, Replace, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setSelection", nullptr, SetSelection, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"backspace", nullptr, Backspace, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"deleteChar", nullptr, DeleteChar, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"clear", nullptr, Clear, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"undo", nullptr, Undo, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"redo", nullptr, Redo, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"copy", nullptr, Copy, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setNumberMode", nullptr, SetNumberMode, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setStatus", nullptr, SetStatus, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"solve", nullptr, Solve, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"factorize", nullptr, Factorize, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setSettings", nullptr, SetSettings, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"shift", nullptr, Shift, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"toggleBit", nullptr, ToggleBit, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getBits", nullptr, GetBits, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"store", nullptr, Store, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"recall", nullptr, Recall, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getVariables", nullptr, GetVariables, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"deleteVariable", nullptr, DeleteVariable, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getAnswer", nullptr, GetAnswer, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getHistory", nullptr, GetHistory, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"historyInsertAnswer", nullptr, HistoryInsertAnswer, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"historySetEquation", nullptr, HistorySetEquation, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"clearHistory", nullptr, ClearHistory, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getUnitCategories", nullptr, GetUnitCategories, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"convert", nullptr, Convert, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"applyConversion", nullptr, ApplyConversion, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"loadCurrencyRates", nullptr, LoadCurrencyRates, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"hasCurrencyRates", nullptr, HasCurrencyRates, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"financial", nullptr, Financial, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setTranslations", nullptr, SetTranslations, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getLibraryVersions", nullptr, GetLibraryVersions, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module g_matecalcModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = RegisterModule,
    .nm_modname = "matecalc",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterMatecalcModule(void)
{
    napi_module_register(&g_matecalcModule);
}
