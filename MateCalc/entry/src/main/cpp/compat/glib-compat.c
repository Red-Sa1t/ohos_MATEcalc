/*
 * Minimal GLib compatibility layer for the MATE Calculator core on HarmonyOS.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version. See http://www.gnu.org/copyleft/gpl.html the full text of the
 * license.
 */

#include "glib.h"

#include <ctype.h>

#ifdef MATE_CALC_OHOS
#include <hilog/log.h>
#endif

void
g_compat_log(const char *level, const char *format, ...)
{
    va_list args;
    char buffer[512];

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
#ifdef MATE_CALC_OHOS
    OH_LOG_Print(LOG_APP, LOG_WARN, 0x0C1C, "MateCalcCore", "%{public}s: %{public}s", level, buffer);
#else
    fprintf(stderr, "mate-calc-core %s: %s\n", level, buffer);
#endif
}

/* Strings */

gchar *
g_strdup(const gchar *str)
{
    if (!str)
        return NULL;
    return strdup(str);
}

gchar *
g_strndup(const gchar *str, gsize n)
{
    gchar *result;

    if (!str)
        return NULL;
    result = malloc(n + 1);
    strncpy(result, str, n);
    result[n] = '\0';
    return result;
}

gchar *
g_strdup_vprintf(const gchar *format, va_list args)
{
    va_list copy;
    int length;
    gchar *result;

    va_copy(copy, args);
    length = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (length < 0)
        return NULL;
    result = malloc((size_t)length + 1);
    vsnprintf(result, (size_t)length + 1, format, args);
    return result;
}

gchar *
g_strdup_printf(const gchar *format, ...)
{
    va_list args;
    gchar *result;

    va_start(args, format);
    result = g_strdup_vprintf(format, args);
    va_end(args);
    return result;
}

int
g_strcmp0(const char *str1, const char *str2)
{
    if (!str1)
        return -(str1 != str2);
    if (!str2)
        return str1 != str2;
    return strcmp(str1, str2);
}

gchar **
g_strsplit(const gchar *string, const gchar *delimiter, gint max_tokens)
{
    gchar **result;
    guint n = 0, allocated = 8;
    const gchar *remainder = string, *s;
    size_t delimiter_len = strlen(delimiter);

    if (max_tokens < 1)
        max_tokens = INT32_MAX;

    result = malloc(sizeof(gchar *) * allocated);
    s = strstr(remainder, delimiter);
    while (s && --max_tokens) {
        if (n + 2 >= allocated) {
            allocated *= 2;
            result = realloc(result, sizeof(gchar *) * allocated);
        }
        result[n++] = g_strndup(remainder, (gsize)(s - remainder));
        remainder = s + delimiter_len;
        s = strstr(remainder, delimiter);
    }
    if (*string) {
        if (n + 2 >= allocated)
            result = realloc(result, sizeof(gchar *) * (allocated + 2));
        result[n++] = g_strdup(remainder);
    }
    result[n] = NULL;
    return result;
}

void
g_strfreev(gchar **str_array)
{
    gchar **s;

    if (!str_array)
        return;
    for (s = str_array; *s; s++)
        free(*s);
    free(str_array);
}

guint
g_strv_length(gchar **str_array)
{
    guint n = 0;

    while (str_array && str_array[n])
        n++;
    return n;
}

gchar *
g_strchug(gchar *string)
{
    guchar *start;

    for (start = (guchar *)string; *start && isspace(*start); start++)
        ;
    memmove(string, start, strlen((gchar *)start) + 1);
    return string;
}

gchar *
g_strchomp(gchar *string)
{
    gsize len = strlen(string);

    while (len--) {
        if (isspace((guchar)string[len]))
            string[len] = '\0';
        else
            break;
    }
    return string;
}

gchar *
g_strjoin(const gchar *separator, ...)
{
    va_list args;
    GString *result = g_string_new("");
    const gchar *s;
    gboolean first = TRUE;

    if (!separator)
        separator = "";
    va_start(args, separator);
    while ((s = va_arg(args, const gchar *)) != NULL) {
        if (!first)
            g_string_append(result, separator);
        g_string_append(result, s);
        first = FALSE;
    }
    va_end(args);
    return g_string_free(result, FALSE);
}

gchar *
g_ascii_strdown(const gchar *str, gssize len)
{
    gchar *result, *s;

    if (len < 0)
        len = (gssize)strlen(str);
    result = g_strndup(str, (gsize)len);
    for (s = result; *s; s++)
        *s = (gchar)tolower((guchar)*s);
    return result;
}

gchar *
g_locale_to_utf8(const gchar *opsysstring, gssize len, gsize *bytes_read, gsize *bytes_written, void *error)
{
    (void)error;
    if (!opsysstring)
        return NULL;
    if (len < 0)
        len = (gssize)strlen(opsysstring);
    if (bytes_read)
        *bytes_read = (gsize)len;
    if (bytes_written)
        *bytes_written = (gsize)len;
    /* HarmonyOS locales are always UTF-8 */
    return g_strndup(opsysstring, (gsize)len);
}

/* UTF-8 */

static int
utf8_sequence_length(guchar c)
{
    if (c < 0x80)
        return 1;
    if ((c & 0xE0) == 0xC0)
        return 2;
    if ((c & 0xF0) == 0xE0)
        return 3;
    if ((c & 0xF8) == 0xF0)
        return 4;
    return -1;
}

gunichar
g_utf8_get_char(const gchar *p)
{
    const guchar *s = (const guchar *)p;
    int len = utf8_sequence_length(s[0]);

    switch (len) {
    case 1:
        return s[0];
    case 2:
        return ((gunichar)(s[0] & 0x1F) << 6) | (s[1] & 0x3F);
    case 3:
        return ((gunichar)(s[0] & 0x0F) << 12) | ((gunichar)(s[1] & 0x3F) << 6) | (s[2] & 0x3F);
    case 4:
        return ((gunichar)(s[0] & 0x07) << 18) | ((gunichar)(s[1] & 0x3F) << 12) |
               ((gunichar)(s[2] & 0x3F) << 6) | (s[3] & 0x3F);
    default:
        return (gunichar)-1;
    }
}

gunichar
g_utf8_get_char_validated(const gchar *p, gssize max_len)
{
    const guchar *s = (const guchar *)p;
    int len, i;

    if (max_len == 0)
        return (gunichar)-2;
    len = utf8_sequence_length(s[0]);
    if (len < 0)
        return (gunichar)-1;
    for (i = 1; i < len; i++) {
        if (max_len > 0 && i >= max_len)
            return (gunichar)-2;
        if ((s[i] & 0xC0) != 0x80)
            return (gunichar)-1;
    }
    return g_utf8_get_char(p);
}

const gchar *
g_compat_utf8_next_char(const gchar *p)
{
    int len = utf8_sequence_length((guchar)*p);

    return p + (len > 0 ? len : 1);
}

gchar *
g_utf8_find_prev_char(const gchar *str, const gchar *p)
{
    while (p > str) {
        p--;
        if ((*(const guchar *)p & 0xC0) != 0x80)
            return (gchar *)p;
    }
    return NULL;
}

gboolean
g_utf8_validate(const gchar *str, gssize max_len, const gchar **end)
{
    const gchar *p = str;

    while ((max_len < 0 || p - str < max_len) && *p) {
        gssize remaining = max_len < 0 ? -1 : max_len - (p - str);
        gunichar c = g_utf8_get_char_validated(p, remaining);
        if (c == (gunichar)-1 || c == (gunichar)-2) {
            if (end)
                *end = p;
            return FALSE;
        }
        p = g_compat_utf8_next_char(p);
    }
    if (end)
        *end = p;
    return TRUE;
}

gint
g_unichar_to_utf8(gunichar c, gchar *outbuf)
{
    guchar buf[4];
    gint len;

    if (c < 0x80) {
        buf[0] = (guchar)c;
        len = 1;
    } else if (c < 0x800) {
        buf[0] = (guchar)(0xC0 | (c >> 6));
        buf[1] = (guchar)(0x80 | (c & 0x3F));
        len = 2;
    } else if (c < 0x10000) {
        buf[0] = (guchar)(0xE0 | (c >> 12));
        buf[1] = (guchar)(0x80 | ((c >> 6) & 0x3F));
        buf[2] = (guchar)(0x80 | (c & 0x3F));
        len = 3;
    } else {
        buf[0] = (guchar)(0xF0 | (c >> 18));
        buf[1] = (guchar)(0x80 | ((c >> 12) & 0x3F));
        buf[2] = (guchar)(0x80 | ((c >> 6) & 0x3F));
        buf[3] = (guchar)(0x80 | (c & 0x3F));
        len = 4;
    }
    if (outbuf)
        memcpy(outbuf, buf, (size_t)len);
    return len;
}

gboolean
g_unichar_isdigit(gunichar c)
{
    /* The parser only treats ASCII and fullwidth digits as decimal digits */
    return (c >= '0' && c <= '9') || (c >= 0xFF10 && c <= 0xFF19) ||
           (c >= 0x0660 && c <= 0x0669) || (c >= 0x06F0 && c <= 0x06F9);
}

gboolean
g_unichar_isalpha(gunichar c)
{
    if (c < 0x80)
        return isalpha((int)c) != 0;
    /* Treat non-ASCII letters (Greek, Cyrillic, CJK, ...) as alphabetic, but
     * exclude the math symbols, digits and punctuation the lexer handles. */
    if ((c >= 0x00C0 && c <= 0x024F && c != 0x00D7 && c != 0x00F7) ||   /* Latin-1 / Extended */
        (c >= 0x0370 && c <= 0x03FF && c != 0x037E) ||                 /* Greek */
        (c >= 0x0400 && c <= 0x052F) ||                                /* Cyrillic */
        (c >= 0x0590 && c <= 0x05FF) ||                                /* Hebrew */
        (c >= 0x0620 && c <= 0x064A) ||                                /* Arabic letters */
        (c >= 0x1D00 && c <= 0x1DBF) ||                                /* Phonetic ext. (modifier letters) */
        c == 0x2071 || c == 0x207F ||                                  /* superscript i, n */
        (c >= 0x2090 && c <= 0x209C) ||                                /* subscript letters (mₑ, Nₐ) */
        (c >= 0x3040 && c <= 0x30FF) ||                                /* Kana */
        (c >= 0x4E00 && c <= 0x9FFF) ||                                /* CJK */
        (c >= 0xAC00 && c <= 0xD7AF))                                  /* Hangul */
        return TRUE;
    return FALSE;
}

gboolean
g_unichar_isxdigit(gunichar c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') ||
           (c >= 0xFF10 && c <= 0xFF19) || (c >= 0xFF21 && c <= 0xFF26) || (c >= 0xFF41 && c <= 0xFF46);
}

/* GString */

static void
g_string_maybe_expand(GString *string, gsize extra)
{
    if (string->len + extra + 1 > string->allocated_len) {
        gsize size = string->allocated_len ? string->allocated_len : 16;
        while (string->len + extra + 1 > size)
            size *= 2;
        string->str = realloc(string->str, size);
        string->allocated_len = size;
    }
}

GString *
g_string_sized_new(gsize dfl_size)
{
    GString *string = calloc(1, sizeof(GString));

    g_string_maybe_expand(string, dfl_size > 0 ? dfl_size : 16);
    string->str[0] = '\0';
    return string;
}

GString *
g_string_new(const gchar *init)
{
    GString *string = g_string_sized_new(init ? strlen(init) + 2 : 16);

    if (init)
        g_string_append(string, init);
    return string;
}

gchar *
g_string_free(GString *string, gboolean free_segment)
{
    gchar *segment = NULL;

    if (!string)
        return NULL;
    if (free_segment)
        free(string->str);
    else
        segment = string->str;
    free(string);
    return segment;
}

GString *
g_string_assign(GString *string, const gchar *rval)
{
    g_string_truncate(string, 0);
    return g_string_append(string, rval);
}

GString *
g_string_truncate(GString *string, gsize len)
{
    if (len < string->len) {
        string->len = len;
        string->str[len] = '\0';
    }
    return string;
}

GString *
g_string_append_len(GString *string, const gchar *val, gssize len)
{
    if (len < 0)
        len = (gssize)strlen(val);
    g_string_maybe_expand(string, (gsize)len);
    memcpy(string->str + string->len, val, (size_t)len);
    string->len += (gsize)len;
    string->str[string->len] = '\0';
    return string;
}

GString *
g_string_append(GString *string, const gchar *val)
{
    return g_string_append_len(string, val, -1);
}

GString *
g_string_append_c(GString *string, gchar c)
{
    return g_string_append_len(string, &c, 1);
}

GString *
g_string_append_unichar(GString *string, gunichar wc)
{
    gchar buf[4];
    gint len = g_unichar_to_utf8(wc, buf);

    return g_string_append_len(string, buf, len);
}

void
g_string_append_printf(GString *string, const gchar *format, ...)
{
    va_list args;
    gchar *s;

    va_start(args, format);
    s = g_strdup_vprintf(format, args);
    va_end(args);
    if (s) {
        g_string_append(string, s);
        free(s);
    }
}

static GString *
g_string_prepend_len(GString *string, const gchar *val, gsize len)
{
    g_string_maybe_expand(string, len);
    memmove(string->str + len, string->str, string->len + 1);
    memcpy(string->str, val, len);
    string->len += len;
    return string;
}

GString *
g_string_prepend(GString *string, const gchar *val)
{
    return g_string_prepend_len(string, val, strlen(val));
}

GString *
g_string_prepend_c(GString *string, gchar c)
{
    return g_string_prepend_len(string, &c, 1);
}

GString *
g_string_prepend_unichar(GString *string, gunichar wc)
{
    gchar buf[4];
    gint len = g_unichar_to_utf8(wc, buf);

    return g_string_prepend_len(string, buf, (gsize)len);
}

/* GList */

GList *
g_list_last(GList *list)
{
    if (list) {
        while (list->next)
            list = list->next;
    }
    return list;
}

GList *
g_list_append(GList *list, gpointer data)
{
    GList *link = calloc(1, sizeof(GList)), *last;

    link->data = data;
    if (!list)
        return link;
    last = g_list_last(list);
    last->next = link;
    link->prev = last;
    return list;
}

GList *
g_list_prepend(GList *list, gpointer data)
{
    GList *link = calloc(1, sizeof(GList));

    link->data = data;
    link->next = list;
    if (list)
        list->prev = link;
    return link;
}

GList *
g_list_copy(GList *list)
{
    GList *result = NULL, *iter;

    for (iter = list; iter; iter = iter->next)
        result = g_list_prepend(result, iter->data);
    /* Reverse back into the original order */
    {
        GList *reversed = NULL;
        while (result) {
            GList *next = result->next;
            result->next = reversed;
            result->prev = next;
            reversed = result;
            result = next;
        }
        result = reversed;
    }
    return result;
}

GList *
g_list_sort(GList *list, GCompareFunc compare_func)
{
    guint n = g_list_length(list), i;
    gpointer *items;
    GList *iter;

    if (n < 2)
        return list;
    items = malloc(sizeof(gpointer) * n);
    for (iter = list, i = 0; iter; iter = iter->next, i++)
        items[i] = iter->data;
    /* Stable insertion sort; lists here are small (currencies, units) */
    for (i = 1; i < n; i++) {
        gpointer key = items[i];
        guint j = i;
        while (j > 0 && compare_func(items[j - 1], key) > 0) {
            items[j] = items[j - 1];
            j--;
        }
        items[j] = key;
    }
    for (iter = list, i = 0; iter; iter = iter->next, i++)
        iter->data = items[i];
    free(items);
    return list;
}

void
g_list_free(GList *list)
{
    while (list) {
        GList *next = list->next;
        free(list);
        list = next;
    }
}

void
g_list_free_full(GList *list, GDestroyNotify free_func)
{
    GList *iter;

    for (iter = list; iter; iter = iter->next)
        free_func(iter->data);
    g_list_free(list);
}

guint
g_list_length(GList *list)
{
    guint n = 0;

    for (; list; list = list->next)
        n++;
    return n;
}

/* GHashTable: insertion-ordered linked list, adequate for the small
 * variable/register tables used by the calculator. */

typedef struct _GHashNode GHashNode;
struct _GHashNode {
    gpointer key;
    gpointer value;
    GHashNode *next;
};

struct _GHashTable {
    GHashNode *head;
    guint size;
    GEqualFunc key_equal;
    GDestroyNotify key_destroy;
    GDestroyNotify value_destroy;
};

guint
g_str_hash(gconstpointer v)
{
    const signed char *p;
    uint32_t h = 5381;

    for (p = v; *p != '\0'; p++)
        h = (h << 5) + h + (uint32_t)*p;
    return h;
}

gboolean
g_str_equal(gconstpointer v1, gconstpointer v2)
{
    return strcmp((const gchar *)v1, (const gchar *)v2) == 0;
}

GHashTable *
g_hash_table_new_full(GHashFunc hash_func, GEqualFunc key_equal_func,
                      GDestroyNotify key_destroy_func, GDestroyNotify value_destroy_func)
{
    GHashTable *table = calloc(1, sizeof(GHashTable));

    (void)hash_func;
    table->key_equal = key_equal_func ? key_equal_func : g_str_equal;
    table->key_destroy = key_destroy_func;
    table->value_destroy = value_destroy_func;
    return table;
}

static void
g_hash_node_free(GHashTable *table, GHashNode *node)
{
    if (table->key_destroy)
        table->key_destroy(node->key);
    if (table->value_destroy)
        table->value_destroy(node->value);
    free(node);
}

void
g_hash_table_remove_all(GHashTable *hash_table)
{
    GHashNode *node = hash_table->head;

    while (node) {
        GHashNode *next = node->next;
        g_hash_node_free(hash_table, node);
        node = next;
    }
    hash_table->head = NULL;
    hash_table->size = 0;
}

void
g_hash_table_destroy(GHashTable *hash_table)
{
    if (!hash_table)
        return;
    g_hash_table_remove_all(hash_table);
    free(hash_table);
}

gboolean
g_hash_table_insert(GHashTable *hash_table, gpointer key, gpointer value)
{
    GHashNode *node, *last = NULL;

    for (node = hash_table->head; node; last = node, node = node->next) {
        if (hash_table->key_equal(node->key, key)) {
            /* GLib keeps the old key and frees the new one */
            if (hash_table->key_destroy)
                hash_table->key_destroy(key);
            if (hash_table->value_destroy)
                hash_table->value_destroy(node->value);
            node->value = value;
            return FALSE;
        }
    }
    node = calloc(1, sizeof(GHashNode));
    node->key = key;
    node->value = value;
    if (last)
        last->next = node;
    else
        hash_table->head = node;
    hash_table->size++;
    return TRUE;
}

gpointer
g_hash_table_lookup(GHashTable *hash_table, gconstpointer key)
{
    GHashNode *node;

    for (node = hash_table->head; node; node = node->next) {
        if (hash_table->key_equal(node->key, key))
            return node->value;
    }
    return NULL;
}

gboolean
g_hash_table_remove(GHashTable *hash_table, gconstpointer key)
{
    GHashNode *node, *prev = NULL;

    for (node = hash_table->head; node; prev = node, node = node->next) {
        if (hash_table->key_equal(node->key, key)) {
            if (prev)
                prev->next = node->next;
            else
                hash_table->head = node->next;
            g_hash_node_free(hash_table, node);
            hash_table->size--;
            return TRUE;
        }
    }
    return FALSE;
}

guint
g_hash_table_size(GHashTable *hash_table)
{
    return hash_table->size;
}

void
g_hash_table_iter_init(GHashTableIter *iter, GHashTable *hash_table)
{
    iter->table = hash_table;
    iter->position = hash_table->head;
}

gboolean
g_hash_table_iter_next(GHashTableIter *iter, gpointer *key, gpointer *value)
{
    GHashNode *node = iter->position;

    if (!node)
        return FALSE;
    if (key)
        *key = node->key;
    if (value)
        *value = node->value;
    iter->position = node->next;
    return TRUE;
}

/* Translations */

static GHashTable *translations = NULL;

void
g_compat_set_translation(const char *key, const char *value)
{
    if (!translations)
        translations = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
    g_hash_table_insert(translations, g_strdup(key), g_strdup(value));
}

void
g_compat_clear_translations(void)
{
    if (translations)
        g_hash_table_remove_all(translations);
}

const char *
g_compat_gettext(const char *msgid)
{
    const char *translated;

    if (!translations || !msgid)
        return msgid;
    translated = g_hash_table_lookup(translations, msgid);
    return translated ? translated : msgid;
}

const char *
g_compat_pgettext(const char *context, const char *msgid)
{
    const char *translated;
    char *key;

    if (!translations || !msgid)
        return msgid;
    key = g_strdup_printf("%s%s", context, msgid);
    translated = g_hash_table_lookup(translations, key);
    free(key);
    return translated ? translated : msgid;
}
