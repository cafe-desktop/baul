/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   eel-string.c: String routines to augment <string.h>.

   Copyright (C) 2000 Eazel, Inc.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Darin Adler <darin@eazel.com>
*/

#include <config.h>
#include "eel-glib-extensions.h"
#include "eel-string.h"

#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#if !defined (EEL_OMIT_SELF_CHECK)
#include "eel-lib-self-check-functions.h"
#endif

size_t
eel_strlen (const char *string)
{
    return string == NULL ? 0 : strlen (string);
}

char *
eel_strchr (const char *haystack, char needle)
{
    return haystack == NULL ? NULL : strchr (haystack, needle);
}

int
eel_strcmp (const char *string_a, const char *string_b)
{
    /* FIXME bugzilla.eazel.com 5450: Maybe we need to make this
     * treat 'NULL < ""', or have a flavor that does that. If we
     * didn't have code that already relies on 'NULL == ""', I
     * would change it right now.
     */
    return strcmp (string_a == NULL ? "" : string_a,
                   string_b == NULL ? "" : string_b);
}

gboolean
eel_str_is_empty (const char *string_or_null)
{
    return eel_strcmp (string_or_null, NULL) == 0;
}

gboolean
eel_str_has_prefix (const char *haystack, const char *needle)
{
    return g_str_has_prefix (haystack == NULL ? "" : haystack,
                             needle == NULL ? "" : needle);
}

gboolean
eel_istr_has_prefix (const char *haystack, const char *needle)
{
    const char *h, *n;
    char hc, nc;

    /* Eat one character at a time. */
    h = haystack == NULL ? "" : haystack;
    n = needle == NULL ? "" : needle;
    do
    {
        if (*n == '\0')
        {
            return TRUE;
        }
        if (*h == '\0')
        {
            return FALSE;
        }
        hc = *h++;
        nc = *n++;
        hc = g_ascii_tolower (hc);
        nc = g_ascii_tolower (nc);
    }
    while (hc == nc);
    return FALSE;
}

/**
 * eel_str_get_prefix:
 * Get a new string containing the first part of an existing string.
 *
 * @source: The string whose prefix should be extracted.
 * @delimiter: The string that marks the end of the prefix.
 *
 * Return value: A newly-allocated string that that matches the first part
 * of @source, up to but not including the first occurrence of
 * @delimiter. If @source is NULL, returns NULL. If
 * @delimiter is NULL, returns a copy of @source.
 * If @delimiter does not occur in @source, returns
 * a copy of @source.
 **/
char *
eel_str_get_prefix (const char *source,
                    const char *delimiter)
{
    char *prefix_start;

    if (source == NULL)
    {
        return NULL;
    }

    if (delimiter == NULL)
    {
        return g_strdup (source);
    }

    prefix_start = strstr (source, delimiter);

    if (prefix_start == NULL)
    {
        return g_strdup ("");
    }

    return g_strndup (source, prefix_start - source);
}

char *
eel_str_double_underscores (const char *string)
{
    int underscores;
    const char *p;
    char *q;
    char *escaped;

    if (string == NULL)
    {
        return NULL;
    }

    underscores = 0;
    for (p = string; *p != '\0'; p++)
    {
        underscores += (*p == '_');
    }

    if (underscores == 0)
    {
        return g_strdup (string);
    }

    escaped = g_new (char, strlen (string) + underscores + 1);
    for (p = string, q = escaped; *p != '\0'; p++, q++)
    {
        /* Add an extra underscore. */
        if (*p == '_')
        {
            *q++ = '_';
        }
        *q = *p;
    }
    *q = '\0';

    return escaped;
}

char *
eel_str_capitalize (const char *string)
{
    char *capitalized;

    if (string == NULL)
    {
        return NULL;
    }

    capitalized = g_strdup (string);

    capitalized[0] = g_ascii_toupper (capitalized[0]);

    return capitalized;
}

/* Note: eel_string_ellipsize_* that use a length in pixels
 * rather than characters can be found in eel_cdk_extensions.h
 *
 * FIXME bugzilla.eazel.com 5089:
 * we should coordinate the names of eel_string_ellipsize_*
 * and eel_str_*_truncate so that they match better and reflect
 * their different behavior.
 */
char *
eel_str_middle_truncate (const char *string,
                         guint truncate_length)
{
    char *truncated;
    guint length;
    guint num_left_chars;
    guint num_right_chars;

    const char delimter[] = "...";
    const guint delimter_length = strlen (delimter);
    const guint min_truncate_length = delimter_length + 2;

    if (string == NULL)
    {
        return NULL;
    }

    /* It doesnt make sense to truncate strings to less than
     * the size of the delimiter plus 2 characters (one on each
     * side)
     */
    if (truncate_length < min_truncate_length)
    {
        return g_strdup (string);
    }

    length = g_utf8_strlen (string, -1);

    /* Make sure the string is not already small enough. */
    if (length <= truncate_length)
    {
        return g_strdup (string);
    }

    /* Find the 'middle' where the truncation will occur. */
    num_left_chars = (truncate_length - delimter_length) / 2;
    num_right_chars = truncate_length - num_left_chars - delimter_length;

    truncated = g_new (char, strlen (string) + 1);

    g_utf8_strncpy (truncated, string, num_left_chars);
    g_strlcat (truncated, delimter, (truncate_length + 1));
    g_strlcat (truncated, g_utf8_offset_to_pointer  (string, length - num_right_chars), (truncate_length + 1));

    return truncated;
}

char *
eel_str_strip_substring_and_after (const char *string,
                                   const char *substring)
{
    const char *substring_position;

    g_return_val_if_fail (substring != NULL, g_strdup (string));
    g_return_val_if_fail (substring[0] != '\0', g_strdup (string));

    if (string == NULL)
    {
        return NULL;
    }

    substring_position = strstr (string, substring);
    if (substring_position == NULL)
    {
        return g_strdup (string);
    }

    return g_strndup (string,
                      substring_position - string);
}

char *
eel_str_replace_substring (const char *string,
                           const char *substring,
                           const char *replacement)
{
    int substring_length, replacement_length, result_length, remaining_length;
    const char *p, *substring_position;
    char *result, *result_position;

    g_return_val_if_fail (substring != NULL, g_strdup (string));
    g_return_val_if_fail (substring[0] != '\0', g_strdup (string));

    if (string == NULL)
    {
        return NULL;
    }

    if (replacement == NULL)
    {
        replacement = "";
    }

    substring_length = strlen (substring);
    replacement_length = eel_strlen (replacement);

    result_length = strlen (string);
    for (p = string; ; p = substring_position + substring_length)
    {
        substring_position = strstr (p, substring);
        if (substring_position == NULL)
        {
            break;
        }
        if (replacement_length > substring_length)
            result_length += replacement_length - substring_length;
    }

    result = g_malloc (result_length + 1);

    result_position = result;
    for (p = string; ; p = substring_position + substring_length)
    {
        substring_position = strstr (p, substring);
        if (substring_position == NULL)
        {
            remaining_length = strlen (p);
            memcpy (result_position, p, remaining_length);
            result_position += remaining_length;
            break;
        }
        memcpy (result_position, p, substring_position - p);
        result_position += substring_position - p;
        memcpy (result_position, replacement, replacement_length);
        result_position += replacement_length;
    }

    result_position[0] = '\0';

    return result;
}

/**************** Custom printf ***********/

typedef struct
{
    const char *start;
    const char *end;
    GString *format;
    int arg_pos;
    int width_pos;
    int width_format_index;
    int precision_pos;
    int precision_format_index;
} ConversionInfo;

enum
{
    ARG_TYPE_INVALID,
    ARG_TYPE_INT,
    ARG_TYPE_LONG,
    ARG_TYPE_LONG_LONG,
    ARG_TYPE_SIZE,
    ARG_TYPE_LONG_DOUBLE,
    ARG_TYPE_DOUBLE,
    ARG_TYPE_POINTER
};

typedef int ArgType; /* An int, because custom are < 0 */


static const char *
get_position (const char *format, int *i)
{
    const char *p;

    p = format;

    if (g_ascii_isdigit (*p))
    {
        p++;

        while (g_ascii_isdigit (*p))
        {
            p++;
        }

        if (*p == '$')
        {
            if (i != NULL)
            {
                *i = atoi (format) - 1;
            }
            return p + 1;
        }
    }

    return format;
}

static gboolean
is_flag (char c)
{
    return strchr ("#0- +'I", c) != NULL;
}

static gboolean
is_length_modifier (char c)
{
    return strchr ("hlLjzt", c) != NULL;
}


static ArgType
get_arg_type_from_format (EelPrintfHandler *custom_handlers,
                          const char *format,
                          int len)
{
    char c;

    c = format[len-1];

    if (custom_handlers != NULL)
    {
        int i;

        for (i = 0; custom_handlers[i].character != 0; i++)
        {
            if (custom_handlers[i].character == c)
            {
                return -(i + 1);
            }
        }
    }

    switch (c)
    {
    case 'd':
    case 'i':
    case 'o':
    case 'u':
    case 'x':
    case 'X':
        if (g_str_has_prefix (format, "ll"))
        {
            return ARG_TYPE_LONG_LONG;
        }
        if (g_str_has_prefix (format, "l"))
        {
            return ARG_TYPE_LONG;
        }
        if (g_str_has_prefix (format, "l"))
        {
            return ARG_TYPE_LONG;
        }
        if (g_str_has_prefix (format, "z"))
        {
            return ARG_TYPE_SIZE;
        }
        return ARG_TYPE_INT;
    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case 'a':
    case 'A':
        if (g_str_has_prefix (format, "L"))
        {
            return ARG_TYPE_LONG_DOUBLE;
        }
        return ARG_TYPE_DOUBLE;
    case 'c':
        return ARG_TYPE_INT;
    case 's':
    case 'p':
    case 'n':
        return ARG_TYPE_POINTER;
    }
    return ARG_TYPE_INVALID;
}

static void
skip_argv (va_list *va,
           ArgType type,
           EelPrintfHandler *custom_handlers)
{
    if (type < 0)
    {
        custom_handlers[-type - 1].skip (va);
        return;
    }

    switch (type)
    {
    default:
    case ARG_TYPE_INVALID:
        return;

    case ARG_TYPE_INT:
        (void) va_arg (*va, int);
        break;
    case ARG_TYPE_LONG:
        (void) va_arg (*va, long int);
        break;
    case ARG_TYPE_LONG_LONG:
        (void) va_arg (*va, long long int);
        break;
    case ARG_TYPE_SIZE:
        (void) va_arg (*va, gsize);
        break;
    case ARG_TYPE_LONG_DOUBLE:
        (void) va_arg (*va, long double);
        break;
    case ARG_TYPE_DOUBLE:
        (void) va_arg (*va, double);
        break;
    case ARG_TYPE_POINTER:
        (void) va_arg (*va, void *);
        break;
    }
}

static void
skip_to_arg (va_list *va,
             ArgType *types,
             EelPrintfHandler *custom_handlers,
             int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        skip_argv (va, types[i], custom_handlers);
    }
}

char *
eel_strdup_vprintf_with_custom (EelPrintfHandler *custom,
                                const char *format,
                                va_list va_orig)
{
    va_list va;
    const char *p;
    int num_args, i, j;
    ArgType *args;
    ConversionInfo *conversions;
    GString *f, *str;
    const char *flags, *width, *prec, *mod, *pos;
    char *s;

    num_args = 0;
    for (p = format; *p != 0; p++)
    {
        if (*p == '%')
        {
            p++;
            if (*p != '%')
            {
                num_args++;
            }
        }
    }

    args = g_new0 (ArgType, num_args * 3 + 1);
    conversions = g_new0 (ConversionInfo, num_args);

    /* i indexes conversions, j indexes args */
    i = 0;
    j = 0;
    p = format;
    while (*p != 0)
    {
        if (*p != '%')
        {
            p++;
            continue;
        }
        p++;
        if (*p == '%')
        {
            p++;
            continue;
        }

        /* We got a real conversion: */
        f = g_string_new ("%");
        conversions[i].start = p - 1;

        /* First comes the positional arg */

        pos = p;
        p = get_position (p, NULL);

        /* Then flags */
        flags = p;
        while (is_flag (*p))
        {
            p++;
        }
        g_string_append_len (f, flags, p - flags);

        /* Field width */

        if (*p == '*')
        {
            p++;
            p = get_position (p, &j);
            args[j] = ARG_TYPE_INT;
            conversions[i].width_pos = j++;
            conversions[i].width_format_index = f->len;
        }
        else
        {
            conversions[i].width_pos = -1;
            conversions[i].width_format_index = -1;
            width = p;
            while (g_ascii_isdigit (*p))
            {
                p++;
            }
            g_string_append_len (f, width, p - width);
        }

        /* Precision */
        conversions[i].precision_pos = -1;
        conversions[i].precision_format_index = -1;
        if (*p == '.')
        {
            g_string_append_c (f, '.');
            p++;

            if (*p == '*')
            {
                p++;
                p = get_position (p, &j);
                args[j] = ARG_TYPE_INT;
                conversions[i].precision_pos = j++;
                conversions[i].precision_format_index = f->len;
            }
            else
            {
                prec = p;
                while (g_ascii_isdigit (*p) || *p == '-')
                {
                    p++;
                }
                g_string_append_len (f, prec, p - prec);
            }
        }

        /* length modifier */

        mod = p;

        while (is_length_modifier (*p))
        {
            p++;
        }

        /* conversion specifier */
        if (*p != 0)
            p++;

        g_string_append_len (f, mod, p - mod);

        get_position (pos, &j);
        args[j] = get_arg_type_from_format (custom, mod, p - mod);
        conversions[i].arg_pos = j++;
        conversions[i].format = f;
        conversions[i].end = p;

        i++;
    }

    g_assert (i == num_args);

    str = g_string_new ("");

    p = format;
    for (i = 0; i < num_args; i++)
    {
        ArgType type;

        g_string_append_len (str, p, conversions[i].start - p);
        p = conversions[i].end;

        if (conversions[i].precision_pos != -1)
        {
            char *val;

            va_copy (va, va_orig);
            skip_to_arg (&va, args, custom, conversions[i].precision_pos);
            val = g_strdup_vprintf ("%d", va);
            va_end (va);

            g_string_insert (conversions[i].format,
                             conversions[i].precision_format_index,
                             val);

            g_free (val);
        }

        if (conversions[i].width_pos != -1)
        {
            char *val;

            va_copy (va, va_orig);
            skip_to_arg (&va, args, custom, conversions[i].width_pos);
            val = g_strdup_vprintf ("%d", va);
            va_end (va);

            g_string_insert (conversions[i].format,
                             conversions[i].width_format_index,
                             val);

            g_free (val);
        }

        va_copy (va, va_orig);
        skip_to_arg (&va, args, custom, conversions[i].arg_pos);
        type = args[conversions[i].arg_pos];
        if (type < 0)
        {
            s = custom[-type - 1].to_string (conversions[i].format->str, va);
            g_string_append (str, s);
            g_free (s);
        }
        else
        {
            g_string_append_vprintf (str, conversions[i].format->str, va);
        }
        va_end (va);

        g_string_free (conversions[i].format, TRUE);
    }
    g_string_append (str, p);

    g_free (args);
    g_free (conversions);

    return g_string_free (str, FALSE);
}

char *
eel_strdup_printf_with_custom (EelPrintfHandler *handlers,
                               const char *format,
                               ...)
{
    va_list va;
    char *res;

    va_start (va, format);
    res = eel_strdup_vprintf_with_custom (handlers, format, va);
    va_end (va);

    return res;
}

#if !defined (EEL_OMIT_SELF_CHECK)

static void
verify_printf (const char *format, ...)
{
    va_list va;
    char *orig, *new;

    va_start (va, format);
    orig = g_strdup_vprintf (format, va);
    va_end (va);

    va_start (va, format);
    new = eel_strdup_vprintf_with_custom (NULL, format, va);
    va_end (va);

    EEL_CHECK_STRING_RESULT (new, orig);

    g_free (orig);
}

static char *
custom1_to_string (char   *format G_GNUC_UNUSED,
		   va_list va)
{
    int i;

    i = va_arg (va, int);

    return g_strdup_printf ("c1-%d-", i);
}

static void
custom1_skip (va_list *va)
{
    (void) va_arg (*va, int);
}

static char *
custom2_to_string (char   *format G_GNUC_UNUSED,
		   va_list va)
{
    char *s;

    s = va_arg (va, char *);

    return g_strdup_printf ("c2-%s-", s);
}

static void
custom2_skip (va_list *va)
{
    (void) va_arg (*va, char *);
}

static EelPrintfHandler handlers[] =
{
    { 'N', custom1_to_string, custom1_skip },
    { 'Y', custom2_to_string, custom2_skip },
    { 0 }
};

static void
verify_custom (const char *orig, const char *format, ...)
{
    char *new;
    va_list va;

    va_start (va, format);
    new = eel_strdup_vprintf_with_custom (handlers, format, va);
    va_end (va);

    EEL_CHECK_STRING_RESULT (new, orig);
}

void
eel_self_check_string (void)
{
    EEL_CHECK_INTEGER_RESULT (eel_strlen (NULL), 0);
    EEL_CHECK_INTEGER_RESULT (eel_strlen (""), 0);
    EEL_CHECK_INTEGER_RESULT (eel_strlen ("abc"), 3);

    EEL_CHECK_INTEGER_RESULT (eel_strcmp (NULL, NULL), 0);
    EEL_CHECK_INTEGER_RESULT (eel_strcmp (NULL, ""), 0);
    EEL_CHECK_INTEGER_RESULT (eel_strcmp ("", NULL), 0);
    EEL_CHECK_INTEGER_RESULT (eel_strcmp ("a", "a"), 0);
    EEL_CHECK_INTEGER_RESULT (eel_strcmp ("aaab", "aaab"), 0);
    EEL_CHECK_BOOLEAN_RESULT (eel_strcmp (NULL, "a") < 0, TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_strcmp ("a", NULL) > 0, TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_strcmp ("", "a") < 0, TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_strcmp ("a", "") > 0, TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_strcmp ("a", "b") < 0, TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_strcmp ("a", "ab") < 0, TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_strcmp ("ab", "a") > 0, TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_strcmp ("aaa", "aaab") < 0, TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_strcmp ("aaab", "aaa") > 0, TRUE);

    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix (NULL, NULL), TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix (NULL, ""), TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix ("", NULL), TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix ("a", "a"), TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix ("aaab", "aaab"), TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix (NULL, "a"), FALSE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix ("a", NULL), TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix ("", "a"), FALSE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix ("a", ""), TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix ("a", "b"), FALSE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix ("a", "ab"), FALSE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix ("ab", "a"), TRUE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix ("aaa", "aaab"), FALSE);
    EEL_CHECK_BOOLEAN_RESULT (eel_str_has_prefix ("aaab", "aaa"), TRUE);

    EEL_CHECK_STRING_RESULT (eel_str_get_prefix (NULL, NULL), NULL);
    EEL_CHECK_STRING_RESULT (eel_str_get_prefix (NULL, "foo"), NULL);
    EEL_CHECK_STRING_RESULT (eel_str_get_prefix ("foo", NULL), "foo");
    EEL_CHECK_STRING_RESULT (eel_str_get_prefix ("", ""), "");
    EEL_CHECK_STRING_RESULT (eel_str_get_prefix ("", "foo"), "");
    EEL_CHECK_STRING_RESULT (eel_str_get_prefix ("foo", ""), "");
    EEL_CHECK_STRING_RESULT (eel_str_get_prefix ("foo", "foo"), "");
    EEL_CHECK_STRING_RESULT (eel_str_get_prefix ("foo:", ":"), "foo");
    EEL_CHECK_STRING_RESULT (eel_str_get_prefix ("foo:bar", ":"), "foo");
    EEL_CHECK_STRING_RESULT (eel_str_get_prefix ("footle:bar", "tle:"), "foo");

    EEL_CHECK_STRING_RESULT (eel_str_double_underscores (NULL), NULL);
    EEL_CHECK_STRING_RESULT (eel_str_double_underscores (""), "");
    EEL_CHECK_STRING_RESULT (eel_str_double_underscores ("_"), "__");
    EEL_CHECK_STRING_RESULT (eel_str_double_underscores ("foo"), "foo");
    EEL_CHECK_STRING_RESULT (eel_str_double_underscores ("foo_bar"), "foo__bar");
    EEL_CHECK_STRING_RESULT (eel_str_double_underscores ("foo_bar_2"), "foo__bar__2");
    EEL_CHECK_STRING_RESULT (eel_str_double_underscores ("_foo"), "__foo");
    EEL_CHECK_STRING_RESULT (eel_str_double_underscores ("foo_"), "foo__");

    EEL_CHECK_STRING_RESULT (eel_str_capitalize (NULL), NULL);
    EEL_CHECK_STRING_RESULT (eel_str_capitalize (""), "");
    EEL_CHECK_STRING_RESULT (eel_str_capitalize ("foo"), "Foo");
    EEL_CHECK_STRING_RESULT (eel_str_capitalize ("Foo"), "Foo");

    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("foo", 0), "foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("foo", 1), "foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("foo", 3), "foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("foo", 4), "foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("foo", 5), "foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("foo", 6), "foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("foo", 7), "foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("a_much_longer_foo", 0), "a_much_longer_foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("a_much_longer_foo", 1), "a_much_longer_foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("a_much_longer_foo", 2), "a_much_longer_foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("a_much_longer_foo", 3), "a_much_longer_foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("a_much_longer_foo", 4), "a_much_longer_foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("a_much_longer_foo", 5), "a...o");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("a_much_longer_foo", 6), "a...oo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("a_much_longer_foo", 7), "a_...oo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("a_much_longer_foo", 8), "a_...foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("a_much_longer_foo", 9), "a_m...foo");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_even", 8), "so...ven");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_odd", 8), "so...odd");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_even", 9), "som...ven");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_odd", 9), "som...odd");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_even", 10), "som...even");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_odd", 10), "som..._odd");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_even", 11), "some...even");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_odd", 11), "some..._odd");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_even", 12), "some..._even");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_odd", 12), "some...g_odd");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_even", 13), "somet..._even");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_odd", 13), "something_odd");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_even", 14), "something_even");
    EEL_CHECK_STRING_RESULT (eel_str_middle_truncate ("something_odd", 13), "something_odd");

    EEL_CHECK_STRING_RESULT (eel_str_strip_substring_and_after (NULL, "bar"), NULL);
    EEL_CHECK_STRING_RESULT (eel_str_strip_substring_and_after ("", "bar"), "");
    EEL_CHECK_STRING_RESULT (eel_str_strip_substring_and_after ("foo", "bar"), "foo");
    EEL_CHECK_STRING_RESULT (eel_str_strip_substring_and_after ("foo bar", "bar"), "foo ");
    EEL_CHECK_STRING_RESULT (eel_str_strip_substring_and_after ("foo bar xxx", "bar"), "foo ");
    EEL_CHECK_STRING_RESULT (eel_str_strip_substring_and_after ("bar", "bar"), "");

    EEL_CHECK_STRING_RESULT (eel_str_replace_substring (NULL, "foo", NULL), NULL);
    EEL_CHECK_STRING_RESULT (eel_str_replace_substring (NULL, "foo", "bar"), NULL);
    EEL_CHECK_STRING_RESULT (eel_str_replace_substring ("bar", "foo", NULL), "bar");
    EEL_CHECK_STRING_RESULT (eel_str_replace_substring ("", "foo", ""), "");
    EEL_CHECK_STRING_RESULT (eel_str_replace_substring ("", "foo", "bar"), "");
    EEL_CHECK_STRING_RESULT (eel_str_replace_substring ("bar", "foo", ""), "bar");
    EEL_CHECK_STRING_RESULT (eel_str_replace_substring ("xxx", "x", "foo"), "foofoofoo");
    EEL_CHECK_STRING_RESULT (eel_str_replace_substring ("fff", "f", "foo"), "foofoofoo");
    EEL_CHECK_STRING_RESULT (eel_str_replace_substring ("foofoofoo", "foo", "f"), "fff");
    EEL_CHECK_STRING_RESULT (eel_str_replace_substring ("foofoofoo", "f", ""), "oooooo");

    verify_printf ("%.*s", 2, "foo");
    verify_printf ("%*.*s", 2, 4, "foo");
    verify_printf ("before %5$*1$.*2$s between %6$*3$.*4$d after",
                   4, 5, 6, 7, "foo", G_PI);
    verify_custom ("c1-42- c2-foo-","%N %Y", 42 ,"foo");
    verify_custom ("c1-42- bar c2-foo-","%N %s %Y", 42, "bar" ,"foo");
    verify_custom ("c1-42- bar c2-foo-","%3$N %2$s %1$Y","foo", "bar", 42);

}

#endif /* !EEL_OMIT_SELF_CHECK */
