/*
 * Minimal GLib compatibility layer for the MATE Calculator core on HarmonyOS.
 *
 * Provides only the subset of GLib that the mate-calc math/parser/unit code
 * uses, so the upstream sources can be compiled with few modifications and
 * without porting GLib itself.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version. See http://www.gnu.org/copyleft/gpl.html the full text of the
 * license.
 */

#ifndef MATE_CALC_GLIB_COMPAT_H
#define MATE_CALC_GLIB_COMPAT_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#define G_BEGIN_DECLS extern "C" {
#define G_END_DECLS }
#else
#define G_BEGIN_DECLS
#define G_END_DECLS
#endif

G_BEGIN_DECLS

/* Basic types */
typedef char gchar;
typedef unsigned char guchar;
typedef int gint;
typedef unsigned int guint;
typedef long glong;
typedef unsigned long gulong;
typedef double gdouble;
typedef int gboolean;
typedef size_t gsize;
typedef ptrdiff_t gssize;
typedef uint32_t gunichar;
typedef void *gpointer;
typedef const void *gconstpointer;
typedef int (*GCompareFunc)(gconstpointer a, gconstpointer b);
typedef void (*GDestroyNotify)(gpointer data);

#ifndef ulong
typedef unsigned long ulong;
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Diagnostics */
void g_compat_log(const char *level, const char *format, ...) __attribute__((format(printf, 2, 3)));
#define g_warning(...) g_compat_log("WARNING", __VA_ARGS__)
#define g_debug(...) ((void)0)
#define g_return_val_if_fail(expr, val) do { if (!(expr)) { g_compat_log("CRITICAL", "%s: assertion '%s' failed", __func__, #expr); return (val); } } while (0)
#define g_return_if_fail(expr) do { if (!(expr)) { g_compat_log("CRITICAL", "%s: assertion '%s' failed", __func__, #expr); return; } } while (0)

/* Memory */
#define g_malloc(n) malloc(n)
#define g_malloc0(n) calloc(1, (n))
#define g_new0(type, n) ((type *)calloc((n), sizeof(type)))
#define g_slice_alloc0(n) calloc(1, (n))
#define g_slice_free(type, mem) free(mem)
#define g_free(p) free((void *)(p))

/* Strings */
gchar *g_strdup(const gchar *str);
gchar *g_strndup(const gchar *str, gsize n);
gchar *g_strdup_printf(const gchar *format, ...) __attribute__((format(printf, 1, 2)));
gchar *g_strdup_vprintf(const gchar *format, va_list args);
int g_strcmp0(const char *str1, const char *str2);
gchar **g_strsplit(const gchar *string, const gchar *delimiter, gint max_tokens);
void g_strfreev(gchar **str_array);
guint g_strv_length(gchar **str_array);
gchar *g_strchug(gchar *string);
gchar *g_strchomp(gchar *string);
#define g_strstrip(string) g_strchomp(g_strchug(string))
gchar *g_strjoin(const gchar *separator, ...);
gchar *g_ascii_strdown(const gchar *str, gssize len);
gchar *g_locale_to_utf8(const gchar *opsysstring, gssize len, gsize *bytes_read, gsize *bytes_written, void *error);

/* UTF-8 */
gunichar g_utf8_get_char(const gchar *p);
gunichar g_utf8_get_char_validated(const gchar *p, gssize max_len);
const gchar *g_compat_utf8_next_char(const gchar *p);
#define g_utf8_next_char(p) ((gchar *)g_compat_utf8_next_char(p))
gchar *g_utf8_find_prev_char(const gchar *str, const gchar *p);
gboolean g_utf8_validate(const gchar *str, gssize max_len, const gchar **end);
gint g_unichar_to_utf8(gunichar c, gchar *outbuf);
gboolean g_unichar_isdigit(gunichar c);
gboolean g_unichar_isalpha(gunichar c);
gboolean g_unichar_isxdigit(gunichar c);

/* GString */
typedef struct {
    gchar *str;
    gsize len;
    gsize allocated_len;
} GString;

GString *g_string_new(const gchar *init);
GString *g_string_sized_new(gsize dfl_size);
gchar *g_string_free(GString *string, gboolean free_segment);
GString *g_string_assign(GString *string, const gchar *rval);
GString *g_string_truncate(GString *string, gsize len);
GString *g_string_append(GString *string, const gchar *val);
GString *g_string_append_len(GString *string, const gchar *val, gssize len);
GString *g_string_append_c(GString *string, gchar c);
GString *g_string_append_unichar(GString *string, gunichar wc);
void g_string_append_printf(GString *string, const gchar *format, ...) __attribute__((format(printf, 2, 3)));
GString *g_string_prepend(GString *string, const gchar *val);
GString *g_string_prepend_c(GString *string, gchar c);
GString *g_string_prepend_unichar(GString *string, gunichar wc);

/* GList */
typedef struct _GList GList;
struct _GList {
    gpointer data;
    GList *next;
    GList *prev;
};

GList *g_list_append(GList *list, gpointer data);
GList *g_list_prepend(GList *list, gpointer data);
GList *g_list_copy(GList *list);
GList *g_list_sort(GList *list, GCompareFunc compare_func);
void g_list_free(GList *list);
void g_list_free_full(GList *list, GDestroyNotify free_func);
guint g_list_length(GList *list);
GList *g_list_last(GList *list);

/* GHashTable (string keys only, small tables) */
typedef struct _GHashTable GHashTable;
typedef struct {
    GHashTable *table;
    gpointer position;
} GHashTableIter;
typedef guint (*GHashFunc)(gconstpointer key);
typedef gboolean (*GEqualFunc)(gconstpointer a, gconstpointer b);

guint g_str_hash(gconstpointer v);
gboolean g_str_equal(gconstpointer v1, gconstpointer v2);
GHashTable *g_hash_table_new_full(GHashFunc hash_func, GEqualFunc key_equal_func,
                                  GDestroyNotify key_destroy_func, GDestroyNotify value_destroy_func);
void g_hash_table_destroy(GHashTable *hash_table);
gboolean g_hash_table_insert(GHashTable *hash_table, gpointer key, gpointer value);
gpointer g_hash_table_lookup(GHashTable *hash_table, gconstpointer key);
gboolean g_hash_table_remove(GHashTable *hash_table, gconstpointer key);
void g_hash_table_remove_all(GHashTable *hash_table);
guint g_hash_table_size(GHashTable *hash_table);
void g_hash_table_iter_init(GHashTableIter *iter, GHashTable *hash_table);
gboolean g_hash_table_iter_next(GHashTableIter *iter, gpointer *key, gpointer *value);

/* Translations: the ArkTS layer loads the upstream catalog for the system
 * language and installs it with g_compat_set_translation(); without a table
 * the msgids are returned unchanged. */
const char *g_compat_gettext(const char *msgid) __attribute__((format_arg(1)));
const char *g_compat_pgettext(const char *context, const char *msgid) __attribute__((format_arg(2)));
void g_compat_set_translation(const char *key, const char *value);
void g_compat_clear_translations(void);
#define _(s) ((char *)g_compat_gettext(s))
#define N_(s) (s)
#define NC_(ctx, s) (s)
#define C_(ctx, s) ((char *)g_compat_pgettext(ctx, s))
#define g_dpgettext2(domain, ctx, s) ((char *)g_compat_pgettext(ctx, s))

G_END_DECLS

#endif /* MATE_CALC_GLIB_COMPAT_H */
