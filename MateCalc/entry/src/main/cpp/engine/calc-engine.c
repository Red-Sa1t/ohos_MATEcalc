/*
 * Copyright (C) 2008-2011 Robert Ancell
 * HarmonyOS port: GTK-free calculator state machine.
 *
 * Ported from mate-calc's math-equation.c. The GtkTextBuffer is replaced by a
 * UTF-8 string with code point offsets; GtkTextMarks (cursor, selection, ans
 * span) are plain offsets adjusted with the same gravity rules, and the
 * insert-text/delete-range signal handlers become explicit pre/post hooks
 * around buffer edits so undo, ans tracking and thousands separators behave
 * like upstream.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version. See http://www.gnu.org/copyleft/gpl.html the full text of the
 * license.
 */

#include <ctype.h>
#include <string.h>

#include <glib/gi18n.h>

#include "calc-engine.h"
#include "currency-manager.h"
#include "financial.h"
#include "math-variables.h"
#include "mp-equation.h"
#include "unit-manager.h"

#define MAX_HISTORY 1000

typedef struct {
    MPNumber ans;          /* Answer of last calculation */
    gchar *expression;     /* Expression entered by user */
    gint ans_start, ans_end; /* Start and end characters for ans variable in expression */
    gint cursor;
    NumberMode number_mode; /* ??? */
    gboolean can_super_minus; /* TRUE if entering minus can generate a superscript minus */
    gboolean entered_multiply; /* Last insert was a multiply character */
    gchar *status;         /* Equation status */
} CalcState;

typedef struct {
    gchar *equation;
    MPNumber answer;
} HistoryEntry;

struct CalcEngine
{
    GString *text;            /* Display text (UTF-8) */
    gint cursor;              /* Insert mark (code points) */
    gint selection_bound;     /* Selection bound mark (code points) */
    gint ans_start, ans_end;  /* -1 when there is no ans span */
    guint revision;

    gint word_size;           /* Word size in bits */
    MPAngleUnit angle_units;  /* Units for trigonometric functions */
    NumberMode number_mode;   /* ??? */
    gboolean can_super_minus; /* TRUE if entering minus can generate a superscript minus */

    gunichar digits[16];      /* Localized digits */

    GList *undo_stack;        /* History of expression mode states */
    GList *redo_stack;
    gboolean in_undo_operation;

    gboolean in_reformat;

    gboolean in_delete;

    gboolean in_solve;

    MathVariables *variables;
    MpSerializer *serializer;

    CalcState state;          /* Equation state */

    HistoryEntry *history;
    guint history_count;
};

struct CalcJob
{
    CalcEngine *engine;
    gboolean factorize;
    gchar *equation;          /* Equation to solve */
    MPNumber number;          /* Number to factorize */
    MpSerializer *serializer; /* Settings snapshot for factorize */
    MPEquationOptions options;

    /* Results */
    MPNumber *number_result;
    gchar *text_result;
    gchar *error;
};

static void reformat_separators(CalcEngine *engine);

/* Text buffer helpers */

static gint
utf8_length(const gchar *text)
{
    gint count = 0;
    const gchar *p;

    for (p = text; *p; p = g_utf8_next_char(p))
        count++;
    return count;
}

static gsize
byte_offset(const gchar *text, gint offset)
{
    const gchar *p = text;

    while (offset > 0 && *p) {
        p = g_utf8_next_char(p);
        offset--;
    }
    return (gsize)(p - text);
}

static gint
char_count(CalcEngine *engine)
{
    return utf8_length(engine->text->str);
}

static gint
clamp_offset(CalcEngine *engine, gint offset)
{
    gint count = char_count(engine);

    if (offset < 0)
        return 0;
    if (offset > count)
        return count;
    return offset;
}

static gchar *
get_text_range(CalcEngine *engine, gint start, gint end)
{
    gsize s = byte_offset(engine->text->str, start);
    gsize e = byte_offset(engine->text->str, end);

    return g_strndup(engine->text->str + s, e - s);
}

static void
adjust_mark_insert(gint *mark, gint offset, gint length, gboolean left_gravity)
{
    if (*mark < 0)
        return;
    if (*mark > offset || (*mark == offset && !left_gravity))
        *mark += length;
}

static void
adjust_mark_delete(gint *mark, gint start, gint end)
{
    if (*mark < 0)
        return;
    if (*mark >= end)
        *mark -= end - start;
    else if (*mark > start)
        *mark = start;
}

static gboolean
has_selection(CalcEngine *engine)
{
    return engine->cursor != engine->selection_bound;
}

static void
place_cursor(CalcEngine *engine, gint offset)
{
    engine->cursor = engine->selection_bound = clamp_offset(engine, offset);
}

static void
get_ans_offsets(CalcEngine *engine, gint *start, gint *end)
{
    *start = engine->ans_start;
    *end = engine->ans_end;
}

static void
clear_ans(CalcEngine *engine, gboolean remove_tag)
{
    (void)remove_tag;
    engine->ans_start = engine->ans_end = -1;
}

/* Status */

void
calc_engine_set_status(CalcEngine *engine, const gchar *status)
{
    g_return_if_fail(engine != NULL);
    g_return_if_fail(status != NULL);

    g_free(engine->state.status);
    engine->state.status = g_strdup(status);
}

const gchar *
calc_engine_get_status(CalcEngine *engine)
{
    g_return_val_if_fail(engine != NULL, NULL);
    return engine->state.status;
}

/* Undo */

static CalcState *
get_current_state(CalcEngine *engine)
{
    CalcState *state;

    state = g_malloc0(sizeof(CalcState));
    state->ans = mp_new();

    mp_set_from_mp(&engine->state.ans, &state->ans);
    state->expression = g_strdup(engine->text->str);
    state->ans_start = engine->ans_start;
    state->ans_end = engine->ans_end;
    state->cursor = engine->cursor;
    state->number_mode = engine->number_mode;
    state->can_super_minus = engine->can_super_minus;
    state->entered_multiply = engine->state.entered_multiply;
    state->status = g_strdup(engine->state.status);

    return state;
}

static void
free_state(gpointer data)
{
    CalcState *state = data;

    mp_clear(&state->ans);
    g_free(state->expression);
    g_free(state->status);
    g_free(state);
}

static void
push_undo_stack(CalcEngine *engine)
{
    if (engine->in_undo_operation)
        return;

    calc_engine_set_status(engine, "");

    /* Can't redo anymore */
    g_list_free_full(engine->redo_stack, free_state);
    engine->redo_stack = NULL;

    engine->undo_stack = g_list_prepend(engine->undo_stack, get_current_state(engine));
}

/* Buffer edits with the upstream signal handler semantics */

static void
buffer_delete(CalcEngine *engine, gint start, gint end)
{
    gsize s, e;

    start = clamp_offset(engine, start);
    end = clamp_offset(engine, end);
    if (start > end) {
        gint t = start;
        start = end;
        end = t;
    }
    if (start == end)
        return;

    /* pre_delete_range_cb */
    if (!engine->in_reformat) {
        push_undo_stack(engine);
        engine->in_delete = TRUE;

        if (engine->ans_start >= 0) {
            gint ans_start, ans_end;

            get_ans_offsets(engine, &ans_start, &ans_end);
            /* Deleted part of ans */
            if (start < ans_end && end > ans_start)
                clear_ans(engine, TRUE);
        }
    }

    s = byte_offset(engine->text->str, start);
    e = byte_offset(engine->text->str, end);
    memmove(engine->text->str + s, engine->text->str + e, engine->text->len - e + 1);
    engine->text->len -= e - s;
    engine->revision++;

    adjust_mark_delete(&engine->cursor, start, end);
    adjust_mark_delete(&engine->selection_bound, start, end);
    adjust_mark_delete(&engine->ans_start, start, end);
    adjust_mark_delete(&engine->ans_end, start, end);

    /* delete_range_cb */
    if (!engine->in_reformat) {
        engine->state.entered_multiply = FALSE;

        /* Update thousands separators */
        reformat_separators(engine);
    }
}

static void
buffer_insert(CalcEngine *engine, gint offset, const gchar *text)
{
    GString *new_text;
    gsize at;
    gint length;

    if (!text || !*text)
        return;
    offset = clamp_offset(engine, offset);

    /* pre_insert_text_cb */
    if (!engine->in_reformat) {
        gunichar c;

        /* If following a delete then have already pushed undo stack */
        if (!engine->in_delete)
            push_undo_stack(engine);

        /* Clear result on next digit entered if cursor at end of line */
        c = g_utf8_get_char(text);
        if ((g_unichar_isdigit(c) || c == mp_serializer_get_radix(engine->serializer)) &&
             calc_engine_is_result(engine) &&
             engine->cursor >= char_count(engine)) {
            engine->in_reformat = TRUE;
            buffer_delete(engine, 0, char_count(engine));
            engine->in_reformat = FALSE;
            clear_ans(engine, FALSE);
            offset = 0;
        }

        if (engine->ans_start >= 0) {
            gint ans_start, ans_end;

            get_ans_offsets(engine, &ans_start, &ans_end);
            /* Inserted inside or right before ans */
            if (offset >= ans_start && offset < ans_end)
                clear_ans(engine, TRUE);
        }
    }

    length = utf8_length(text);
    at = byte_offset(engine->text->str, offset);
    new_text = g_string_sized_new(engine->text->len + strlen(text) + 1);
    g_string_append_len(new_text, engine->text->str, (gssize)at);
    g_string_append(new_text, text);
    g_string_append(new_text, engine->text->str + at);
    g_string_free(engine->text, TRUE);
    engine->text = new_text;
    engine->revision++;

    /* Cursor and selection have right gravity, ans start right, ans end left */
    adjust_mark_insert(&engine->cursor, offset, length, FALSE);
    adjust_mark_insert(&engine->selection_bound, offset, length, FALSE);
    adjust_mark_insert(&engine->ans_start, offset, length, FALSE);
    adjust_mark_insert(&engine->ans_end, offset, length, TRUE);

    /* insert_text_cb */
    if (!engine->in_reformat) {
        engine->state.entered_multiply = strcmp(text, "×") == 0;

        /* Update thousands separators */
        reformat_separators(engine);
    }
}

static void
buffer_set_text(CalcEngine *engine, const gchar *text)
{
    buffer_delete(engine, 0, char_count(engine));
    buffer_insert(engine, 0, text);
}

static void
delete_selection(CalcEngine *engine)
{
    if (has_selection(engine))
        buffer_delete(engine, engine->cursor, engine->selection_bound);
}

static void
insert_at_cursor(CalcEngine *engine, const gchar *text)
{
    buffer_insert(engine, engine->cursor, text);
}

/* Called at the end of every public editing operation: GTK clears in_delete
 * from an idle handler after a delete-range. */
static void
end_operation(CalcEngine *engine)
{
    engine->in_delete = FALSE;
}

/* Reformatting */

static void
reformat_ans(CalcEngine *engine)
{
    gchar *orig_ans_text;
    gchar *ans_text;

    if (engine->ans_start < 0)
        return;

    orig_ans_text = get_text_range(engine, engine->ans_start, engine->ans_end);
    ans_text = mp_serializer_to_string(engine->serializer, &engine->state.ans);
    if (strcmp(orig_ans_text, ans_text) != 0) {
        gint start = engine->ans_start;

        engine->in_undo_operation = TRUE;
        engine->in_reformat = TRUE;

        buffer_delete(engine, engine->ans_start, engine->ans_end);
        buffer_insert(engine, start, ans_text);
        engine->ans_start = start;
        engine->ans_end = start + utf8_length(ans_text);

        engine->in_reformat = FALSE;
        engine->in_undo_operation = FALSE;
    }
    g_free(orig_ans_text);
    g_free(ans_text);
}

static gint
count_digits(CalcEngine *engine, const gchar *text)
{
    const gchar *read_iter;
    gint count = 0;

    read_iter = text;
    while (*read_iter != '\0') {
        if (!g_unichar_isdigit(g_utf8_get_char(read_iter)))
            return count;

        read_iter = g_utf8_next_char(read_iter);

        /* Allow a thousands separator between digits follow a digit */
        if (g_utf8_get_char(read_iter) == mp_serializer_get_thousands_separator(engine->serializer)) {
            read_iter = g_utf8_next_char(read_iter);
            if (!g_unichar_isdigit(g_utf8_get_char(read_iter)))
                return count;
        }

        count++;
    }

    return count;
}

static void
reformat_separators(CalcEngine *engine)
{
    gchar *text;
    const gchar *read_iter;
    gint ans_start, ans_end;
    gint offset, digit_offset = 0;
    gboolean in_number = FALSE, in_radix = FALSE, last_is_tsep = FALSE;

    engine->in_undo_operation = TRUE;
    engine->in_reformat = TRUE;

    /* Iterate over a snapshot; edits are applied to the live buffer at the
     * tracked offset like the upstream GtkTextIter based loop. */
    text = g_strdup(engine->text->str);
    get_ans_offsets(engine, &ans_start, &ans_end);
    for (read_iter = text, offset = 0; *read_iter != '\0'; read_iter = g_utf8_next_char(read_iter), offset++) {
        gunichar c;
        gboolean expect_tsep;

        /* See what digit this character is */
        c = g_utf8_get_char(read_iter);

        expect_tsep = mp_serializer_get_base(engine->serializer) == 10 &&
                      mp_serializer_get_show_thousands_separators(engine->serializer) &&
                      in_number && !in_radix && !last_is_tsep &&
                      digit_offset > 0 && digit_offset % mp_serializer_get_thousands_separator_count(engine->serializer) == 0;
        last_is_tsep = FALSE;

        /* Don't mess with ans */
        if (offset >= ans_start && offset <= ans_end) {
            in_number = in_radix = FALSE;
            continue;
        }
        if (g_unichar_isdigit(c)) {
            if (!in_number)
                digit_offset = count_digits(engine, read_iter);
            in_number = TRUE;

            /* Expected a thousands separator between these digits - insert it */
            if (expect_tsep) {
                gchar buffer[7];
                gint len;

                len = g_unichar_to_utf8(mp_serializer_get_thousands_separator(engine->serializer), buffer);
                buffer[len] = '\0';
                buffer_insert(engine, offset, buffer);
                if (ans_start > offset) {
                    ans_start++;
                    ans_end++;
                }
                offset++;
                last_is_tsep = TRUE;
            }

            digit_offset--;
        }
        else if (c == mp_serializer_get_radix(engine->serializer)) {
            in_number = in_radix = TRUE;
        }
        else if (c == mp_serializer_get_thousands_separator(engine->serializer)) {
            /* Didn't expect thousands separator - delete it.
             * HarmonyOS port: only when it sits between digits, otherwise a
             * space separator (C/fr locales) would eat the spaces in
             * factorization results such as "2^3 × 3". */
            if (!expect_tsep && in_number && g_unichar_isdigit(g_utf8_get_char(g_utf8_next_char(read_iter)))) {
                buffer_delete(engine, offset, offset + 1);
                if (ans_start > offset) {
                    ans_start--;
                    ans_end--;
                }
                offset--;
            }
            else
                last_is_tsep = TRUE;
        }
        else {
            in_number = in_radix = FALSE;
        }
    }

    g_free(text);

    engine->in_reformat = FALSE;
    engine->in_undo_operation = FALSE;
}

static void
reformat_display(CalcEngine *engine)
{
    /* Change ans */
    reformat_ans(engine);

    /* Add/remove thousands separators */
    reformat_separators(engine);
}

static void
apply_state(CalcEngine *engine, CalcState *state)
{
    /* Disable undo detection */
    engine->in_undo_operation = TRUE;

    mp_set_from_mp(&state->ans, &engine->state.ans);

    engine->in_reformat = TRUE;
    buffer_set_text(engine, state->expression);
    engine->in_reformat = FALSE;
    place_cursor(engine, state->cursor);
    clear_ans(engine, FALSE);
    if (state->ans_start >= 0) {
        engine->ans_start = state->ans_start;
        engine->ans_end = state->ans_end;
    }

    calc_engine_set_number_mode(engine, state->number_mode);
    engine->can_super_minus = state->can_super_minus;
    engine->state.entered_multiply = state->entered_multiply;
    calc_engine_set_status(engine, state->status);

    engine->in_undo_operation = FALSE;
}

/* Construction */

CalcEngine *
calc_engine_new(const char *variables_file)
{
    const char *default_digits[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"};
    CalcEngine *engine = g_new0(CalcEngine, 1);
    int i;

    engine->text = g_string_new("");
    engine->ans_start = engine->ans_end = -1;

    for (i = 0; i < 16; i++)
        engine->digits[i] = g_utf8_get_char(default_digits[i]);

    engine->variables = math_variables_new(variables_file);

    engine->state.ans = mp_new();
    engine->state.status = g_strdup("");
    engine->word_size = 32;
    engine->angle_units = MP_DEGREES;
    engine->serializer = mp_serializer_new(MP_DISPLAY_FORMAT_AUTOMATIC, 10, 9);
    mp_set_from_integer(0, &engine->state.ans);

    return engine;
}

void
calc_engine_free(CalcEngine *engine)
{
    guint i;

    if (!engine)
        return;
    g_string_free(engine->text, TRUE);
    g_list_free_full(engine->undo_stack, free_state);
    g_list_free_full(engine->redo_stack, free_state);
    mp_clear(&engine->state.ans);
    g_free(engine->state.status);
    for (i = 0; i < engine->history_count; i++) {
        g_free(engine->history[i].equation);
        mp_clear(&engine->history[i].answer);
    }
    g_free(engine->history);
    g_object_unref(engine->serializer);
    g_free(engine);
}

/* Display state */

const gchar *
calc_engine_get_display(CalcEngine *engine)
{
    g_return_val_if_fail(engine != NULL, NULL);
    return engine->text->str;
}

void
calc_engine_get_selection(CalcEngine *engine, gint *start, gint *end)
{
    *start = MIN(engine->cursor, engine->selection_bound);
    *end = MAX(engine->cursor, engine->selection_bound);
}

void
calc_engine_set_selection(CalcEngine *engine, gint start, gint end)
{
    g_return_if_fail(engine != NULL);
    engine->selection_bound = clamp_offset(engine, start);
    engine->cursor = clamp_offset(engine, end);
}

void
calc_engine_get_ans_range(CalcEngine *engine, gint *start, gint *end)
{
    get_ans_offsets(engine, start, end);
}

guint
calc_engine_get_revision(CalcEngine *engine)
{
    return engine->revision;
}

gboolean
calc_engine_is_empty(CalcEngine *engine)
{
    g_return_val_if_fail(engine != NULL, FALSE);
    return engine->text->len == 0;
}

static gchar *
get_equation(CalcEngine *engine)
{
    gchar *text;
    GString *eq_text;
    gint ans_start = -1, ans_end = -1, offset;
    const gchar *read_iter;
    gboolean last_is_digit = FALSE;

    text = g_strdup(engine->text->str);
    eq_text = g_string_sized_new(strlen(text) + 1);

    if (engine->ans_start >= 0)
        get_ans_offsets(engine, &ans_start, &ans_end);

    for (read_iter = text, offset = 0; *read_iter != '\0'; read_iter = g_utf8_next_char(read_iter), offset++) {
        gunichar c;
        gboolean is_digit, next_is_digit;

        c = g_utf8_get_char(read_iter);
        is_digit = g_unichar_isdigit(c);
        next_is_digit = g_unichar_isdigit(g_utf8_get_char(g_utf8_next_char(read_iter)));

        /* Replace ans text with variable */
        if (offset == ans_start) {
             gint i;

             g_string_append(eq_text, "ans");
             for (i = 0; i < ans_end - ans_start - 1 && *read_iter; i++)
                 read_iter = g_utf8_next_char(read_iter);
             offset += ans_end - ans_start - 1;
             is_digit = FALSE;
             continue;
        }

        /* Ignore thousands separators (including spaces between digits) */
        if (c == mp_serializer_get_thousands_separator(engine->serializer) && last_is_digit && next_is_digit) {
            /* Skip official thousands separator */
        }
        else if (c == ' ' && last_is_digit) {
            /* Skip consecutive spaces between digits - look ahead to find next non-space character */
            const gchar *lookahead = read_iter;
            while (*lookahead != '\0' && g_utf8_get_char(lookahead) == ' ') {
                lookahead = g_utf8_next_char(lookahead);
            }
            /* Only skip if the next non-space character is a digit */
            if (*lookahead != '\0' && g_unichar_isdigit(g_utf8_get_char(lookahead))) {
                /* Skip all consecutive spaces - advance read_iter to the last space */
                while (g_utf8_get_char(g_utf8_next_char(read_iter)) == ' ') {
                    read_iter = g_utf8_next_char(read_iter);
                    offset++;
                }
                /* Current space will be skipped by not appending it */
            } else {
                /* Keep this space - it's not between digits */
                g_string_append_unichar(eq_text, c);
            }
        }
        /* Substitute radix character */
        else if (c == mp_serializer_get_radix(engine->serializer) && (last_is_digit || next_is_digit))
            g_string_append_unichar(eq_text, '.');
        else
            g_string_append_unichar(eq_text, c);

        last_is_digit = is_digit;
    }
    g_free(text);

    return g_string_free(eq_text, FALSE);
}

gboolean
calc_engine_is_result(CalcEngine *engine)
{
    char *text;
    gboolean result;

    g_return_val_if_fail(engine != NULL, FALSE);

    text = get_equation(engine);
    result = strcmp(text, "ans") == 0;
    g_free(text);

    return result;
}

static gboolean
get_number(CalcEngine *engine, MPNumber *z)
{
    gchar *text;
    gboolean result;

    if (calc_engine_is_result(engine)) {
        mp_set_from_mp(&engine->state.ans, z);
        return TRUE;
    }
    else {
        text = get_equation(engine);
        result = !mp_serializer_from_string(engine->serializer, text, z);
        g_free(text);
        return result;
    }
}

NumberMode
calc_engine_get_number_mode(CalcEngine *engine)
{
    g_return_val_if_fail(engine != NULL, NORMAL);
    return engine->number_mode;
}

void
calc_engine_set_number_mode(CalcEngine *engine, NumberMode mode)
{
    g_return_if_fail(engine != NULL);

    if (engine->number_mode == mode)
        return;

    engine->can_super_minus = mode == SUPERSCRIPT;

    engine->number_mode = mode;
}

/* Settings */

void
calc_engine_set_accuracy(CalcEngine *engine, gint accuracy)
{
    g_return_if_fail(engine != NULL);

    if (mp_serializer_get_trailing_digits(engine->serializer) == accuracy)
        return;
    mp_serializer_set_trailing_digits(engine->serializer, accuracy);
    reformat_display(engine);
}

gint
calc_engine_get_accuracy(CalcEngine *engine)
{
    return mp_serializer_get_trailing_digits(engine->serializer);
}

void
calc_engine_set_show_thousands_separators(CalcEngine *engine, gboolean visible)
{
    g_return_if_fail(engine != NULL);

    if (mp_serializer_get_show_thousands_separators(engine->serializer) == visible)
        return;

    mp_serializer_set_show_thousands_separators(engine->serializer, visible);
    reformat_display(engine);
}

gboolean
calc_engine_get_show_thousands_separators(CalcEngine *engine)
{
    return mp_serializer_get_show_thousands_separators(engine->serializer);
}

void
calc_engine_set_show_trailing_zeroes(CalcEngine *engine, gboolean visible)
{
    g_return_if_fail(engine != NULL);

    if (mp_serializer_get_show_trailing_zeroes(engine->serializer) == visible)
        return;

    mp_serializer_set_show_trailing_zeroes(engine->serializer, visible);
    reformat_display(engine);
}

gboolean
calc_engine_get_show_trailing_zeroes(CalcEngine *engine)
{
    return mp_serializer_get_show_trailing_zeroes(engine->serializer);
}

void
calc_engine_set_number_format(CalcEngine *engine, MpDisplayFormat format)
{
    g_return_if_fail(engine != NULL);

    if (mp_serializer_get_number_format(engine->serializer) == format)
        return;

    mp_serializer_set_number_format(engine->serializer, format);
    reformat_display(engine);
}

MpDisplayFormat
calc_engine_get_number_format(CalcEngine *engine)
{
    return mp_serializer_get_number_format(engine->serializer);
}

void
calc_engine_set_base(CalcEngine *engine, gint base)
{
    g_return_if_fail(engine != NULL);

    if (mp_serializer_get_base(engine->serializer) == base)
        return;

    mp_serializer_set_base(engine->serializer, base);
    reformat_display(engine);
}

gint
calc_engine_get_base(CalcEngine *engine)
{
    return mp_serializer_get_base(engine->serializer);
}

void
calc_engine_set_word_size(CalcEngine *engine, gint word_size)
{
    g_return_if_fail(engine != NULL);
    engine->word_size = word_size;
}

gint
calc_engine_get_word_size(CalcEngine *engine)
{
    return engine->word_size;
}

void
calc_engine_set_angle_units(CalcEngine *engine, MPAngleUnit angle_units)
{
    g_return_if_fail(engine != NULL);
    engine->angle_units = angle_units;
}

MPAngleUnit
calc_engine_get_angle_units(CalcEngine *engine)
{
    return engine->angle_units;
}

void
calc_engine_set_radix(CalcEngine *engine, gunichar radix)
{
    g_return_if_fail(engine != NULL);

    if (mp_serializer_get_radix(engine->serializer) == radix)
        return;
    mp_serializer_set_radix(engine->serializer, radix);
    reformat_display(engine);
}

gunichar
calc_engine_get_radix(CalcEngine *engine)
{
    return mp_serializer_get_radix(engine->serializer);
}

void
calc_engine_set_thousands_separator(CalcEngine *engine, gunichar separator)
{
    g_return_if_fail(engine != NULL);

    if (mp_serializer_get_thousands_separator(engine->serializer) == separator)
        return;
    mp_serializer_set_thousands_separator(engine->serializer, separator);
    reformat_display(engine);
}

/* Editing */

void
calc_engine_set(CalcEngine *engine, const gchar *text)
{
    g_return_if_fail(engine != NULL);
    g_return_if_fail(text != NULL);
    buffer_set_text(engine, text);
    clear_ans(engine, FALSE);
    end_operation(engine);
}

static void
add_history(CalcEngine *engine, const gchar *equation, const MPNumber *answer)
{
    HistoryEntry *entry;

    /* Same as math_history_insert_entry: skip repeated equations */
    if (engine->history_count >= 1 &&
        g_strcmp0(engine->history[engine->history_count - 1].equation, equation) == 0)
        return;

    if (engine->history_count == MAX_HISTORY) {
        g_free(engine->history[0].equation);
        mp_clear(&engine->history[0].answer);
        memmove(engine->history, engine->history + 1, sizeof(HistoryEntry) * (MAX_HISTORY - 1));
        engine->history_count--;
    }
    if (engine->history_count % 16 == 0)
        engine->history = realloc(engine->history, sizeof(HistoryEntry) * (engine->history_count + 16));
    entry = &engine->history[engine->history_count++];
    entry->equation = g_strdup(equation);
    entry->answer = mp_new();
    mp_set_from_mp(answer, &entry->answer);
}

static void
set_with_history(CalcEngine *engine, const gchar *text)
{
    MPNumber x = mp_new();
    gchar *expression;

    get_number(engine, &x);
    expression = g_strdup(engine->text->str);
    calc_engine_set(engine, text);

    /* Notify history */
    add_history(engine, expression, &x);

    g_free(expression);
    mp_clear(&x);
}

static void
set_number(CalcEngine *engine, const MPNumber *x)
{
    char *text;

    /* Notify history */
    add_history(engine, engine->text->str, x);

    /* Show the number in the user chosen format */
    text = mp_serializer_to_string(engine->serializer, x);
    buffer_set_text(engine, text);
    mp_set_from_mp(x, &engine->state.ans);

    /* Mark this text as the answer variable */
    clear_ans(engine, FALSE);
    engine->ans_start = 0;
    engine->ans_end = char_count(engine);

    g_free(text);
}

void
calc_engine_insert(CalcEngine *engine, const gchar *text)
{
    g_return_if_fail(engine != NULL);
    g_return_if_fail(text != NULL);

    /* Replace ** with ^ (not on all keyboards) */
    if (!has_selection(engine) &&
        strcmp(text, "×") == 0 && engine->state.entered_multiply) {
        if (engine->cursor > 0)
            buffer_delete(engine, engine->cursor - 1, engine->cursor);
        insert_at_cursor(engine, "^");
        end_operation(engine);
        return;
    }

    /* Can't enter superscript minus after entering digits */
    if (strstr("⁰¹²³⁴⁵⁶⁷⁸⁹", text) != NULL || strcmp("⁻", text) == 0)
        engine->can_super_minus = FALSE;

    /* Disable super/subscript mode when finished entering */
    if (strstr("⁻⁰¹²³⁴⁵⁶⁷⁸⁹₀₁₂₃₄₅₆₇₈₉", text) == NULL)
        calc_engine_set_number_mode(engine, NORMAL);

    delete_selection(engine);
    insert_at_cursor(engine, text);
    end_operation(engine);
}

void
calc_engine_insert_digit(CalcEngine *engine, guint digit)
{
    static const char *subscript_digits[] = {"₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉", NULL};
    static const char *superscript_digits[] = {"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹", NULL};

    g_return_if_fail(engine != NULL);
    g_return_if_fail(digit < 16);

    if (engine->number_mode == NORMAL || digit >= 10) {
        gchar buffer[7];
        gint len;
        len = g_unichar_to_utf8(engine->digits[digit], buffer);
        buffer[len] = '\0';
        calc_engine_insert(engine, buffer);
    }
    else if (engine->number_mode == SUPERSCRIPT)
        calc_engine_insert(engine, superscript_digits[digit]);
    else if (engine->number_mode == SUBSCRIPT)
        calc_engine_insert(engine, subscript_digits[digit]);
}

void
calc_engine_insert_numeric_point(CalcEngine *engine)
{
    gchar buffer[7];
    gint len;

    g_return_if_fail(engine != NULL);

    len = g_unichar_to_utf8(mp_serializer_get_radix(engine->serializer), buffer);
    buffer[len] = '\0';
    calc_engine_insert(engine, buffer);
}

static void
insert_number(CalcEngine *engine, const MPNumber *x)
{
    char *text;

    text = mp_serializer_to_string(engine->serializer, x);
    calc_engine_insert(engine, text);
    g_free(text);
}

void
calc_engine_insert_exponent(CalcEngine *engine)
{
    g_return_if_fail(engine != NULL);
    calc_engine_insert(engine, "×10");
    calc_engine_set_number_mode(engine, SUPERSCRIPT);
}

void
calc_engine_insert_subtract(CalcEngine *engine)
{
    g_return_if_fail(engine != NULL);
    if (engine->number_mode == SUPERSCRIPT && engine->can_super_minus) {
        calc_engine_insert(engine, "⁻");
        engine->can_super_minus = FALSE;
    }
    else {
        calc_engine_insert(engine, "−");
        calc_engine_set_number_mode(engine, NORMAL);
    }
}

void
calc_engine_insert_character_code(CalcEngine *engine, const gchar *text)
{
    MPNumber x = mp_new();
    int i = 0;

    g_return_if_fail(engine != NULL);
    g_return_if_fail(text != NULL);

    if (text[0] == '\0') {
        mp_clear(&x);
        return;
    }

    /* Same as character_code_dialog_response_cb: big-endian bytes of the UTF-8 text */
    mp_set_from_integer(0, &x);
    while (TRUE) {
        mp_add_integer(&x, (guchar)text[i], &x);
        if (text[i+1]) {
             mp_shift(&x, 8, &x);
             i++;
        }
        else
            break;
    }
    insert_number(engine, &x);
    mp_clear(&x);
}

void
calc_engine_replace(CalcEngine *engine, gint start, gint end, const gchar *text)
{
    g_return_if_fail(engine != NULL);

    start = clamp_offset(engine, start);
    end = clamp_offset(engine, end);
    if (end > start)
        buffer_delete(engine, start, end);
    if (text && *text) {
        place_cursor(engine, start);
        insert_at_cursor(engine, text);
    } else
        place_cursor(engine, start);
    end_operation(engine);
}

void
calc_engine_backspace(CalcEngine *engine)
{
    g_return_if_fail(engine != NULL);

    /* Can't delete empty display */
    if (calc_engine_is_empty(engine))
        return;

    if (has_selection(engine))
        delete_selection(engine);
    else if (engine->cursor > 0)
        buffer_delete(engine, engine->cursor - 1, engine->cursor);
    end_operation(engine);
}

void
calc_engine_delete(CalcEngine *engine)
{
    g_return_if_fail(engine != NULL);

    if (engine->cursor >= char_count(engine))
        return;

    buffer_delete(engine, engine->cursor, engine->cursor + 1);
    end_operation(engine);
}

void
calc_engine_clear(CalcEngine *engine)
{
    g_return_if_fail(engine != NULL);

    calc_engine_set_number_mode(engine, NORMAL);
    buffer_set_text(engine, "");
    clear_ans(engine, FALSE);
    end_operation(engine);
}

void
calc_engine_undo(CalcEngine *engine)
{
    GList *link;
    CalcState *state;

    g_return_if_fail(engine != NULL);

    if (!engine->undo_stack) {
        calc_engine_set_status(engine,
                               /* Error shown when trying to undo with no undo history */
                               _("No undo history"));
        return;
    }

    link = engine->undo_stack;
    engine->undo_stack = link->next;
    if (engine->undo_stack)
        engine->undo_stack->prev = NULL;
    state = link->data;
    free(link);

    engine->redo_stack = g_list_prepend(engine->redo_stack, get_current_state(engine));

    apply_state(engine, state);
    free_state(state);
}

void
calc_engine_redo(CalcEngine *engine)
{
    GList *link;
    CalcState *state;

    g_return_if_fail(engine != NULL);

    if (!engine->redo_stack) {
        calc_engine_set_status(engine,
                               /* Error shown when trying to redo with no redo history */
                               _("No redo history"));
        return;
    }

    link = engine->redo_stack;
    engine->redo_stack = link->next;
    if (engine->redo_stack)
        engine->redo_stack->prev = NULL;
    state = link->data;
    free(link);

    engine->undo_stack = g_list_prepend(engine->undo_stack, get_current_state(engine));

    apply_state(engine, state);
    free_state(state);
}

gchar *
calc_engine_copy(CalcEngine *engine)
{
    gint start, end;

    g_return_val_if_fail(engine != NULL, NULL);

    calc_engine_get_selection(engine, &start, &end);
    if (start == end) {
        start = 0;
        end = char_count(engine);
    }
    return get_text_range(engine, start, end);
}

/* Solving */

static int
variable_is_defined(const char *name, void *data)
{
    CalcJob *job = data;
    char *c, *lower_name;

    lower_name = strdup(name);
    for (c = lower_name; *c; c++)
        *c = tolower(*c);

    if (strcmp(lower_name, "rand") == 0 ||
        strcmp(lower_name, "ans") == 0) {
        g_free(lower_name);
        return 1;
    }
    g_free(lower_name);

    return math_variables_get(job->engine->variables, name) != NULL;
}

static int
get_variable(const char *name, MPNumber *z, void *data)
{
    char *c, *lower_name;
    int result = 1;
    CalcJob *job = data;
    MPNumber *t;

    lower_name = strdup(name);
    for (c = lower_name; *c; c++)
        *c = tolower(*c);

    if (strcmp(lower_name, "rand") == 0)
        mp_set_from_random(z);
    else if (strcmp(lower_name, "ans") == 0)
        mp_set_from_mp(&job->number, z);
    else {
        t = math_variables_get(job->engine->variables, name);
        if (t)
            mp_set_from_mp(t, z);
        else
            result = 0;
    }
    free(lower_name);

    return result;
}

static void
set_variable(const char *name, const MPNumber *x, void *data)
{
    CalcJob *job = data;
    /* FIXME: Don't allow writing to built-in variables, e.g. ans, rand, sin, ... */
    math_variables_set(job->engine->variables, name, x);
}

static int
convert(const MPNumber *x, const char *x_units, const char *z_units, MPNumber *z, void *data)
{
    return unit_manager_convert_by_symbol(unit_manager_get_default(), x, x_units, z_units, z);
}

static CalcJob *
job_new(CalcEngine *engine)
{
    CalcJob *job = g_new0(CalcJob, 1);

    job->engine = engine;
    job->number = mp_new();
    return job;
}

static void
job_free(CalcJob *job)
{
    g_free(job->equation);
    mp_clear(&job->number);
    if (job->serializer)
        g_object_unref(job->serializer);
    if (job->number_result) {
        mp_clear(job->number_result);
        g_free(job->number_result);
    }
    g_free(job->text_result);
    g_free(job->error);
    g_free(job);
}

gboolean
calc_engine_in_solve(CalcEngine *engine)
{
    return engine->in_solve;
}

CalcJob *
calc_engine_solve_prepare(CalcEngine *engine)
{
    CalcJob *job;
    GString *equation_text;
    gchar *c, *text;
    gint n_brackets = 0;

    g_return_val_if_fail(engine != NULL, NULL);

    // FIXME: should replace calculation or give error message
    if (engine->in_solve)
        return NULL;

    if (calc_engine_is_empty(engine))
        return NULL;

    /* If showing a result return to the equation that caused it */
    // FIXME: Result may not be here due to solve (i.e. the user may have entered "ans")
    if (calc_engine_is_result(engine)) {
        calc_engine_undo(engine);
        return NULL;
    }

    engine->in_solve = TRUE;

    calc_engine_set_number_mode(engine, NORMAL);

    text = get_equation(engine);
    equation_text = g_string_new(text);
    g_free(text);
    /* Count the number of brackets and automatically add missing closing brackets */
    for (c = equation_text->str; *c; c++) {
        if (*c == '(')
            n_brackets++;
        else if (*c == ')')
            n_brackets--;
    }
    while (n_brackets > 0) {
        g_string_append_c(equation_text, ')');
        n_brackets--;
    }

    job = job_new(engine);
    job->equation = g_string_free(equation_text, FALSE);
    mp_set_from_mp(&engine->state.ans, &job->number);
    job->options.base = mp_serializer_get_base(engine->serializer);
    job->options.wordlen = engine->word_size;
    job->options.angle_units = engine->angle_units;
    job->options.variable_is_defined = variable_is_defined;
    job->options.get_variable = get_variable;
    job->options.set_variable = set_variable;
    job->options.convert = convert;
    job->options.callback_data = job;

    return job;
}

static MpSerializer *
copy_serializer(MpSerializer *serializer, MpDisplayFormat format)
{
    MpSerializer *copy = mp_serializer_new(format, mp_serializer_get_base(serializer),
                                           mp_serializer_get_trailing_digits(serializer));

    mp_serializer_set_leading_digits(copy, mp_serializer_get_leading_digits(serializer));
    mp_serializer_set_radix(copy, mp_serializer_get_radix(serializer));
    mp_serializer_set_thousands_separator(copy, mp_serializer_get_thousands_separator(serializer));
    mp_serializer_set_show_thousands_separators(copy, mp_serializer_get_show_thousands_separators(serializer));
    mp_serializer_set_show_trailing_zeroes(copy, mp_serializer_get_show_trailing_zeroes(serializer));
    return copy;
}

CalcJob *
calc_engine_factorize_prepare(CalcEngine *engine)
{
    CalcJob *job;
    MPNumber x = mp_new();

    g_return_val_if_fail(engine != NULL, NULL);

    // FIXME: should replace calculation or give error message
    if (engine->in_solve) {
        mp_clear(&x);
        return NULL;
    }

    if (!get_number(engine, &x) || !mp_is_integer(&x)) {
        /* Error displayed when trying to factorize a non-integer value */
        calc_engine_set_status(engine, _("Need an integer to factorize"));
        mp_clear(&x);
        return NULL;
    }
    engine->in_solve = TRUE;

    job = job_new(engine);
    job->factorize = TRUE;
    mp_set_from_mp(&x, &job->number);
    job->serializer = copy_serializer(engine->serializer, MP_DISPLAY_FORMAT_FIXED);
    mp_clear(&x);

    return job;
}

static void
solve_real(CalcJob *job)
{
    gint result;
    gchar *error_token = NULL;
    MPNumber z = mp_new();

    result = mp_equation_parse(job->equation, &job->options, &z, &error_token);

    switch (result) {
        case PARSER_ERR_NONE:
            job->number_result = g_malloc(sizeof(MPNumber));
            *job->number_result = mp_new();
            mp_set_from_mp(&z, job->number_result);
            break;

        case PARSER_ERR_OVERFLOW:
            job->error = g_strdup(/* Error displayed to user when they perform a bitwise operation on numbers greater than the current word */
                               _("Overflow. Try a bigger word size"));
            break;

        case PARSER_ERR_UNKNOWN_VARIABLE:
            job->error = g_strdup_printf(/* Error displayed to user when they an unknown variable is entered */
                                      _("Unknown variable '%s'"), error_token);
            break;

        case PARSER_ERR_UNKNOWN_FUNCTION:
            job->error = g_strdup_printf(/* Error displayed to user when an unknown function is entered */
                                      _("Function '%s' is not defined"), error_token);
            break;

        case PARSER_ERR_UNKNOWN_CONVERSION:
            job->error = g_strdup(/* Error displayed to user when an conversion with unknown units is attempted */
                               _("Unknown conversion"));
            break;

        case PARSER_ERR_MP:
            if (mp_get_error())
                job->error = g_strdup(mp_get_error());
            else if (error_token)
                job->error = g_strdup_printf(/* Uncategorized error. Show error token to user*/
                                    _("Malformed expression at token '%s'"), error_token);
            else
                job->error = g_strdup (/* Unknown error. */
                                    _("Malformed expression"));
            break;

        default:
            job->error = g_strdup(/* Error displayed to user when they enter an invalid calculation */
                               _("Malformed expression"));
            break;
    }
    g_free(error_token);
    mp_clear(&z);
}

static void
factorize_real(CalcJob *job)
{
    GString *text;
    GList *factors, *factor, *next_factor;
    int e = 1;

    factors = mp_factorize(&job->number);

    text = g_string_new("");

    for (factor = factors; factor != NULL; factor = factor->next)
    {
        gchar *temp;
        MPNumber *n;

        n = factor->data;
        next_factor = factor->next;
        if (next_factor != NULL && mp_compare(n, next_factor->data) == 0)
        {
            e++;
            continue;
        }
        temp = mp_serializer_to_string(job->serializer, n);
        g_string_append(text, temp);

        if (e > 1)
        {
            g_string_append(text, "^");
            g_string_append_printf(text, "%d", e);
        }
        if (factor->next)
            g_string_append(text, " × ");

        e = 1;
        g_free(temp);
    }
    for (factor = factors; factor != NULL; factor = factor->next)
        mp_free(factor->data);
    g_list_free(factors);

    job->text_result = g_string_free(text, FALSE);
}

void
calc_job_compute(CalcJob *job)
{
    if (job->factorize)
        factorize_real(job);
    else
        solve_real(job);
}

void
calc_engine_job_finish(CalcEngine *engine, CalcJob *job)
{
    if (!job)
        return;

    engine->in_solve = FALSE;

    if (!job->error)
        calc_engine_set_status(engine, "");

    if (job->error != NULL) {
        calc_engine_set_status(engine, job->error);
    }
    else if (job->number_result != NULL) {
        set_number(engine, job->number_result);
    }
    else if (job->text_result != NULL) {
        set_with_history(engine, job->text_result);
    }
    end_operation(engine);
    job_free(job);
}

void
calc_engine_solve(CalcEngine *engine)
{
    CalcJob *job = calc_engine_solve_prepare(engine);

    if (job) {
        calc_job_compute(job);
        calc_engine_job_finish(engine, job);
    }
}

void
calc_engine_factorize(CalcEngine *engine)
{
    CalcJob *job = calc_engine_factorize_prepare(engine);

    if (job) {
        calc_job_compute(job);
        calc_engine_job_finish(engine, job);
    }
}

/* Programmer mode */

void
calc_engine_shift(CalcEngine *engine, gint count)
{
    MPNumber z = mp_new();

    g_return_if_fail(engine != NULL);

    if (!get_number(engine, &z)) {
        calc_engine_set_status(engine,
                               /* This message is displayed in the status bar when a bit
                                shift operation is performed and the display does not contain a number */
                               _("No sane value to bitwise shift"));
        mp_clear(&z);
        return;
    }

    mp_shift(&z, count, &z);
    set_number(engine, &z);
    end_operation(engine);
    mp_clear(&z);
}

static gboolean
get_unsigned_bits(CalcEngine *engine, uint64_t *bits, gboolean require_integer)
{
    MPNumber x = mp_new();
    gboolean result;

    result = get_number(engine, &x);
    if (result) {
        MPNumber max = mp_new();
        MPNumber fraction = mp_new();

        mp_set_from_unsigned_integer(UINT64_MAX, &max);
        mp_fractional_part(&x, &fraction);
        if (mp_is_negative(&x) || mp_is_greater_than(&x, &max) || (require_integer && !mp_is_zero(&fraction)))
            result = FALSE;
        else
            *bits = mp_to_unsigned_integer(&x);
        mp_clear(&max);
        mp_clear(&fraction);
    }
    mp_clear(&x);
    return result;
}

void
calc_engine_toggle_bit(CalcEngine *engine, guint bit)
{
    MPNumber x = mp_new();
    uint64_t bits = 0;

    g_return_if_fail(engine != NULL);
    g_return_if_fail(bit < 64);

    if (!get_unsigned_bits(engine, &bits, FALSE)) {
        calc_engine_set_status(engine,
                               /* Message displayed when cannot toggle bit in display*/
                               _("Displayed value not an integer"));
        mp_clear(&x);
        return;
    }

    bits ^= (1ULL << (63 - bit));

    mp_set_from_unsigned_integer(bits, &x);

    // FIXME: Only do this if in ans format, otherwise set text in same format as previous number
    set_number(engine, &x);
    end_operation(engine);
    mp_clear(&x);
}

gboolean
calc_engine_get_bits(CalcEngine *engine, uint64_t *bits, gchar **label)
{
    GString *text;
    gint base;

    if (!get_unsigned_bits(engine, bits, TRUE))
        return FALSE;

    base = calc_engine_get_base(engine);
    text = g_string_new("");
    if (base != 8) {
        if (text->len != 0)
            g_string_append(text, " = ");
        g_string_append_printf(text, "%llo", (unsigned long long)*bits);
        g_string_append(text, "₈");
    }
    if (base != 10) {
        if (text->len != 0)
            g_string_append(text, " = ");
        g_string_append_printf(text, "%llu", (unsigned long long)*bits);
        g_string_append(text, "₁₀");
    }
    if (base != 16) {
        if (text->len != 0)
            g_string_append(text, " = ");
        g_string_append_printf(text, "%llX", (unsigned long long)*bits);
        g_string_append(text, "₁₆");
    }
    *label = g_string_free(text, FALSE);
    return TRUE;
}

/* Variables */

void
calc_engine_store(CalcEngine *engine, const gchar *name)
{
    MPNumber t = mp_new();

    g_return_if_fail(engine != NULL);
    g_return_if_fail(name != NULL);

    if (!get_number(engine, &t))
        calc_engine_set_status(engine, _("No sane value to store"));
    else
        math_variables_set(engine->variables, name, &t);
    mp_clear(&t);
}

void
calc_engine_recall(CalcEngine *engine, const gchar *name)
{
    g_return_if_fail(engine != NULL);
    g_return_if_fail(name != NULL);
    calc_engine_insert(engine, name);
}

gchar **
calc_engine_get_variable_names(CalcEngine *engine)
{
    return math_variables_get_names(engine->variables);
}

gchar *
calc_engine_get_variable_text(CalcEngine *engine, const gchar *name)
{
    MPNumber *value = math_variables_get(engine->variables, name);

    if (!value)
        return NULL;
    return mp_serializer_to_string(engine->serializer, value);
}

void
calc_engine_delete_variable(CalcEngine *engine, const gchar *name)
{
    math_variables_delete(engine->variables, name);
}

gchar *
calc_engine_get_answer_text(CalcEngine *engine)
{
    return mp_serializer_to_string(engine->serializer, &engine->state.ans);
}

/* History */

guint
calc_engine_get_history_count(CalcEngine *engine)
{
    return engine->history_count;
}

const gchar *
calc_engine_get_history_equation(CalcEngine *engine, guint index)
{
    g_return_val_if_fail(index < engine->history_count, NULL);
    return engine->history[index].equation;
}

gchar *
calc_engine_get_history_answer(CalcEngine *engine, guint index)
{
    g_return_val_if_fail(index < engine->history_count, NULL);
    return mp_serializer_to_string(engine->serializer, &engine->history[index].answer);
}

void
calc_engine_history_insert_answer(CalcEngine *engine, guint index)
{
    gchar *answer;

    g_return_if_fail(index < engine->history_count);
    answer = calc_engine_get_history_answer(engine, index);
    calc_engine_insert(engine, answer);
    g_free(answer);
}

void
calc_engine_history_set_equation(CalcEngine *engine, guint index)
{
    g_return_if_fail(index < engine->history_count);
    calc_engine_set(engine, engine->history[index].equation);
    place_cursor(engine, char_count(engine));
}

void
calc_engine_clear_history(CalcEngine *engine)
{
    guint i;

    for (i = 0; i < engine->history_count; i++) {
        g_free(engine->history[i].equation);
        mp_clear(&engine->history[i].answer);
    }
    engine->history_count = 0;
}

/* Converter */

static gboolean
convert_units(const gchar *category_name, const gchar *source_name, const gchar *target_name,
              const MPNumber *x, MPNumber *z, Unit **source_unit, Unit **target_unit)
{
    UnitManager *manager = unit_manager_get_default();
    UnitCategory *category = unit_manager_get_category(manager, category_name);

    if (!category)
        return FALSE;
    *source_unit = unit_category_get_unit_by_name(category, source_name);
    *target_unit = unit_category_get_unit_by_name(category, target_name);
    if (!*source_unit || !*target_unit)
        return FALSE;
    return unit_category_convert(category, x, *source_unit, *target_unit, z);
}

gboolean
calc_engine_convert(CalcEngine *engine, const gchar *category, const gchar *source_unit,
                    const gchar *target_unit, gchar **source_text, gchar **target_text)
{
    MPNumber x = mp_new(), z = mp_new();
    Unit *source, *target;
    gboolean result = FALSE;

    if (get_number(engine, &x) && convert_units(category, source_unit, target_unit, &x, &z, &source, &target)) {
        *source_text = unit_format(source, &x);
        *target_text = unit_format(target, &z);
        result = TRUE;
    }
    mp_clear(&x);
    mp_clear(&z);
    return result;
}

gboolean
calc_engine_apply_conversion(CalcEngine *engine, const gchar *category, const gchar *source_unit,
                             const gchar *target_unit)
{
    MPNumber x = mp_new(), z = mp_new();
    Unit *source, *target;
    gboolean result = FALSE;

    if (get_number(engine, &x) && convert_units(category, source_unit, target_unit, &x, &z, &source, &target)) {
        set_number(engine, &z);
        end_operation(engine);
        result = TRUE;
    }
    mp_clear(&x);
    mp_clear(&z);
    return result;
}

void
calc_engine_load_currency_rates(CalcEngine *engine, const gchar *imf_data, const gchar *ecb_data)
{
    currency_manager_load_rates(currency_manager_get_default(), imf_data, ecb_data);
    reformat_display(engine);
}

/* Financial */

void
calc_engine_financial(CalcEngine *engine, gint function, const gchar *args[4])
{
    MPNumber arg[4], result = mp_new();
    const gchar *error;
    int i;

    for (i = 0; i < 4; i++) {
        arg[i] = mp_new();
        if (args[i])
            mp_set_from_string(args[i], 10, &arg[i]);
    }

    error = do_finc_expression(function, &arg[0], &arg[1], &arg[2], &arg[3], &result);
    set_number(engine, &result);
    end_operation(engine);
    /* Set the status after the display changed so it is not cleared by the
     * undo push (upstream lost this message). */
    if (error)
        calc_engine_set_status(engine, error);

    for (i = 0; i < 4; i++)
        mp_clear(&arg[i]);
    mp_clear(&result);
}
