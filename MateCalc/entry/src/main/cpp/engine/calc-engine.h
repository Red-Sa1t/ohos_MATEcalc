/*
 * Copyright (C) 2008-2011 Robert Ancell
 * HarmonyOS port: GTK-free calculator state machine.
 *
 * This is a port of mate-calc's MathEquation (math-equation.c) with the
 * GtkTextBuffer replaced by a plain UTF-8 buffer. It owns the display text,
 * cursor/selection, the "ans" span, undo/redo, number modes, the serializer
 * settings, variables, history and the programmer/converter/financial helpers.
 * The ArkTS UI renders the state and forwards user input.
 *
 * All text offsets are in Unicode code points.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version. See http://www.gnu.org/copyleft/gpl.html the full text of the
 * license.
 */

#ifndef CALC_ENGINE_H
#define CALC_ENGINE_H

#include <stdint.h>

#include "mp.h"
#include "mp-serializer.h"

G_BEGIN_DECLS

typedef enum {
    NORMAL,
    SUPERSCRIPT,
    SUBSCRIPT
} NumberMode;

typedef struct CalcEngine CalcEngine;

/* variables_file: where user variables are persisted (NULL = memory only) */
CalcEngine *calc_engine_new(const char *variables_file);
void calc_engine_free(CalcEngine *engine);

/* Display state */
const gchar *calc_engine_get_display(CalcEngine *engine);
void calc_engine_get_selection(CalcEngine *engine, gint *start, gint *end);
void calc_engine_set_selection(CalcEngine *engine, gint start, gint end);
void calc_engine_get_ans_range(CalcEngine *engine, gint *start, gint *end);
const gchar *calc_engine_get_status(CalcEngine *engine);
void calc_engine_set_status(CalcEngine *engine, const gchar *status);
gboolean calc_engine_is_empty(CalcEngine *engine);
gboolean calc_engine_is_result(CalcEngine *engine);
/* Monotonic counter, incremented whenever the display text changes */
guint calc_engine_get_revision(CalcEngine *engine);

NumberMode calc_engine_get_number_mode(CalcEngine *engine);
void calc_engine_set_number_mode(CalcEngine *engine, NumberMode mode);

/* Settings (each reformats the display like the upstream property setters) */
void calc_engine_set_accuracy(CalcEngine *engine, gint accuracy);
gint calc_engine_get_accuracy(CalcEngine *engine);
void calc_engine_set_show_thousands_separators(CalcEngine *engine, gboolean visible);
gboolean calc_engine_get_show_thousands_separators(CalcEngine *engine);
void calc_engine_set_show_trailing_zeroes(CalcEngine *engine, gboolean visible);
gboolean calc_engine_get_show_trailing_zeroes(CalcEngine *engine);
void calc_engine_set_number_format(CalcEngine *engine, MpDisplayFormat format);
MpDisplayFormat calc_engine_get_number_format(CalcEngine *engine);
void calc_engine_set_base(CalcEngine *engine, gint base);
gint calc_engine_get_base(CalcEngine *engine);
void calc_engine_set_word_size(CalcEngine *engine, gint word_size);
gint calc_engine_get_word_size(CalcEngine *engine);
void calc_engine_set_angle_units(CalcEngine *engine, MPAngleUnit angle_units);
MPAngleUnit calc_engine_get_angle_units(CalcEngine *engine);
/* Locale specific characters */
void calc_engine_set_radix(CalcEngine *engine, gunichar radix);
gunichar calc_engine_get_radix(CalcEngine *engine);
void calc_engine_set_thousands_separator(CalcEngine *engine, gunichar separator);

/* Editing */
void calc_engine_set(CalcEngine *engine, const gchar *text);
void calc_engine_insert(CalcEngine *engine, const gchar *text);
void calc_engine_insert_digit(CalcEngine *engine, guint digit);
void calc_engine_insert_numeric_point(CalcEngine *engine);
void calc_engine_insert_exponent(CalcEngine *engine);
void calc_engine_insert_subtract(CalcEngine *engine);
void calc_engine_insert_character_code(CalcEngine *engine, const gchar *text);
/* Raw edit coming from the text field: replace [start, end) with text */
void calc_engine_replace(CalcEngine *engine, gint start, gint end, const gchar *text);
void calc_engine_backspace(CalcEngine *engine);
void calc_engine_delete(CalcEngine *engine);
void calc_engine_clear(CalcEngine *engine);
void calc_engine_undo(CalcEngine *engine);
void calc_engine_redo(CalcEngine *engine);
/* Text to put on the clipboard: the selection, or the whole display */
gchar *calc_engine_copy(CalcEngine *engine);

/* Solving. The *_prepare/_compute/_finish split lets the N-API layer run the
 * expensive part on a worker thread while the UI thread waits. */
typedef struct CalcJob CalcJob;
CalcJob *calc_engine_solve_prepare(CalcEngine *engine);
CalcJob *calc_engine_factorize_prepare(CalcEngine *engine);
void calc_job_compute(CalcJob *job);
void calc_engine_job_finish(CalcEngine *engine, CalcJob *job);
gboolean calc_engine_in_solve(CalcEngine *engine);
/* Synchronous helpers */
void calc_engine_solve(CalcEngine *engine);
void calc_engine_factorize(CalcEngine *engine);

/* Programmer mode */
void calc_engine_shift(CalcEngine *engine, gint count);
void calc_engine_toggle_bit(CalcEngine *engine, guint bit);
/* Returns TRUE if the current value fits in 64 unsigned bits; label receives the
 * "123₈ = 83₁₀ = 53₁₆" text (caller frees). */
gboolean calc_engine_get_bits(CalcEngine *engine, uint64_t *bits, gchar **label);

/* Variables */
void calc_engine_store(CalcEngine *engine, const gchar *name);
void calc_engine_recall(CalcEngine *engine, const gchar *name);
gchar **calc_engine_get_variable_names(CalcEngine *engine);
gchar *calc_engine_get_variable_text(CalcEngine *engine, const gchar *name);
void calc_engine_delete_variable(CalcEngine *engine, const gchar *name);
/* Formatted value of the "ans" variable (caller frees) */
gchar *calc_engine_get_answer_text(CalcEngine *engine);

/* History */
guint calc_engine_get_history_count(CalcEngine *engine);
const gchar *calc_engine_get_history_equation(CalcEngine *engine, guint index);
gchar *calc_engine_get_history_answer(CalcEngine *engine, guint index);
void calc_engine_history_insert_answer(CalcEngine *engine, guint index);
void calc_engine_history_set_equation(CalcEngine *engine, guint index);
void calc_engine_clear_history(CalcEngine *engine);

/* Unit / currency converter. Returns FALSE if the current value cannot be
 * converted; otherwise source_text/target_text receive formatted values. */
gboolean calc_engine_convert(CalcEngine *engine, const gchar *category, const gchar *source_unit,
                             const gchar *target_unit, gchar **source_text, gchar **target_text);
/* Replace the display with the converted value */
gboolean calc_engine_apply_conversion(CalcEngine *engine, const gchar *category, const gchar *source_unit,
                                      const gchar *target_unit);
void calc_engine_load_currency_rates(CalcEngine *engine, const gchar *imf_data, const gchar *ecb_data);

/* Financial dialogs: args are decimal strings (unused ones may be NULL) */
void calc_engine_financial(CalcEngine *engine, gint function, const gchar *args[4]);

G_END_DECLS

#endif /* CALC_ENGINE_H */
