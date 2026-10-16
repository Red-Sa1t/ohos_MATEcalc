/*
 * Minimal GObject replacement for the MATE Calculator core on HarmonyOS.
 *
 * The calculator core only uses GObject as a plain struct-with-private-data
 * container. G_DEFINE_TYPE_WITH_PRIVATE is expanded into a calloc based
 * constructor <type>_compat_new() that allocates the instance and its private
 * struct in one block and runs <type>_init().
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version. See http://www.gnu.org/copyleft/gpl.html the full text of the
 * license.
 */

#ifndef MATE_CALC_GLIB_OBJECT_COMPAT_H
#define MATE_CALC_GLIB_OBJECT_COMPAT_H

#include "glib.h"

G_BEGIN_DECLS

typedef struct {
    int compat_unused;
} GObject;

typedef struct {
    int compat_unused;
} GObjectClass;

#define G_TYPE_CHECK_INSTANCE_CAST(obj, type, c_type) ((c_type *)(obj))

#define G_DEFINE_TYPE_WITH_PRIVATE(TypeName, type_name, PARENT)                 \
    static void type_name##_init(TypeName *self);                               \
    static TypeName##Private *                                                  \
    type_name##_get_instance_private(TypeName *self)                            \
    {                                                                           \
        return (TypeName##Private *)(self + 1);                                 \
    }                                                                           \
    TypeName *                                                                  \
    type_name##_compat_new(void)                                                \
    {                                                                           \
        TypeName *self = calloc(1, sizeof(TypeName) + sizeof(TypeName##Private)); \
        type_name##_init(self);                                                 \
        return self;                                                            \
    }

#define g_object_ref(obj) (obj)
#define g_object_unref(obj) free(obj)

G_END_DECLS

#endif /* MATE_CALC_GLIB_OBJECT_COMPAT_H */
