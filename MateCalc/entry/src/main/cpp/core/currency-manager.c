/*
 * Copyright (C) 2008-2011 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version. See http://www.gnu.org/copyleft/gpl.html the full text of the
 * license.
 */
#include <string.h>
#include <glib/gi18n.h>

#include "currency-manager.h"
#include "mp.h"

typedef struct {
    char *short_name;
    char *symbol;
    char *long_name;
} CurrencyInfo;
static const CurrencyInfo currency_info[] = {
    {"AED", "إ.د",  N_("UAE Dirham")},
    {"AUD", "$",    N_("Australian Dollar")},
    {"BGN", "лв",   N_("Bulgarian Lev")},
    {"BHD", ".ب.د", N_("Bahraini Dinar")},
    {"BND", "$",    N_("Brunei Dollar")},
    {"BRL", "R$",   N_("Brazilian Real")},
    {"BWP", "P",    N_("Botswana Pula")},
    {"CAD", "$",    N_("Canadian Dollar")},
    {"CFA", "Fr",   N_("CFA Franc")},
    {"CHF", "Fr",   N_("Swiss Franc")},
    {"CLP", "$",    N_("Chilean Peso")},
    {"CNY", "元",   N_("Chinese Yuan")},
    {"COP", "$",    N_("Colombian Peso")},
    {"CZK", "Kč",   N_("Czech Koruna")},
    {"DKK", "kr",   N_("Danish Krone")},
    {"DZD", "ج.د",  N_("Algerian Dinar")},
    {"EEK", "KR",   N_("Estonian Kroon")},
    {"EUR", "€",    N_("Euro")},
    {"GBP", "£",    N_("Pound Sterling")},
    {"HKD", "$",    N_("Hong Kong Dollar")},
    {"HRK", "kn",   N_("Croatian Kuna")},
    {"HUF", "Ft",   N_("Hungarian Forint")},
    {"IDR", "Rp",   N_("Indonesian Rupiah")},
    {"ILS", "₪",    N_("Israeli New Shekel")},
    {"INR", "₹",    N_("Indian Rupee")},
    {"IRR", "﷼",    N_("Iranian Rial")},
    {"ISK", "kr",   N_("Icelandic Krona")},
    {"JPY", "¥",    N_("Japanese Yen")},
    {"KRW", "₩",    N_("South Korean Won")},
    {"KWD", "ك.د",  N_("Kuwaiti Dinar")},
    {"KZT", "₸",    N_("Kazakhstani Tenge")},
    {"LKR", "Rs",   N_("Sri Lankan Rupee")},
    {"LTL", "Lt",   N_("Lithuanian Litas")},
    {"LVL", "Ls",   N_("Latvian Lats")},
    {"LYD", "د.ل",  N_("Libyan Dinar")},
    {"MUR", "Rs",   N_("Mauritian Rupee")},
    {"MXN", "$",    N_("Mexican Peso")},
    {"MYR", "RM",   N_("Malaysian Ringgit")},
    {"NOK", "kr",   N_("Norwegian Krone")},
    {"NPR", "Rs",   N_("Nepalese Rupee")},
    {"NZD", "$",    N_("New Zealand Dollar")},
    {"OMR", "ع.ر.", N_("Omani Rial")},
    {"PEN", "S/.",  N_("Peruvian Nuevo Sol")},
    {"PHP", "₱",    N_("Philippine Peso")},
    {"PKR", "Rs",   N_("Pakistani Rupee")},
    {"PLN", "zł",   N_("Polish Zloty")},
    {"QAR", "ق.ر",  N_("Qatari Riyal")},
    {"RON", "L",    N_("New Romanian Leu")},
    {"RUB", "руб.", N_("Russian Rouble")},
    {"SAR", "س.ر",  N_("Saudi Riyal")},
    {"SEK", "kr",   N_("Swedish Krona")},
    {"SGD", "$",    N_("Singapore Dollar")},
    {"THB", "฿",    N_("Thai Baht")},
    {"TND", "ت.د",  N_("Tunisian Dinar")},
    {"TRY", "TL",   N_("New Turkish Lira")},
    {"TTD", "$",    N_("T&T Dollar (TTD)")},
    {"USD", "$",    N_("US Dollar")},
    {"UYU", "$",    N_("Uruguayan Peso")},
    {"VEF", "Bs F", N_("Venezuelan Bolívar")},
    {"ZAR", "R",    N_("South African Rand")},
    {NULL, NULL}
};

/* HarmonyOS port: the rate files are downloaded by the ArkTS layer
 * (@kit.NetworkKit) and handed over with currency_manager_load_rates(). */
static gboolean loaded_rates = FALSE;

struct CurrencyManagerPrivate
{
    GList *currencies;
};

G_DEFINE_TYPE_WITH_PRIVATE (CurrencyManager, currency_manager, G_TYPE_OBJECT);

static CurrencyManager *default_currency_manager = NULL;

CurrencyManager *
currency_manager_get_default(void)
{
    int i;

    if (default_currency_manager)
        return default_currency_manager;

    default_currency_manager = currency_manager_compat_new();

    for (i = 0; currency_info[i].short_name; i++) {
        Currency *c = currency_new(currency_info[i].short_name,
                                   _(currency_info[i].long_name),
                                   currency_info[i].symbol);
        default_currency_manager->priv->currencies = g_list_append(default_currency_manager->priv->currencies, c);
    }

    return default_currency_manager;
}

GList *
currency_manager_get_currencies(CurrencyManager *manager)
{
    g_return_val_if_fail(manager != NULL, NULL);
    return manager->priv->currencies;
}

Currency *
currency_manager_get_currency(CurrencyManager *manager, const gchar *name)
{
    g_return_val_if_fail(manager != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);

    GList *link;
    for (link = manager->priv->currencies; link; link = link->next) {
        Currency *c = link->data;
        const MPNumber *value;

        value = currency_get_value(c);

        if (!strcmp(name, currency_get_name(c))) {
            if (mp_is_negative(value) ||
                mp_is_zero(value)) {
                return NULL;
            }
            else
                return c;
        }
    }
    return NULL;
}

static Currency *
add_currency(CurrencyManager *manager, const gchar *short_name)
{
    GList *iter;
    Currency *c;

    for (iter = manager->priv->currencies; iter; iter = iter->next) {
        c = iter->data;
        if (strcmp(short_name, currency_get_name(c)) == 0)
            return c;
    }

    g_warning("Currency %s is not in the currency table", short_name);
    c = currency_new(short_name, short_name, short_name);
    manager->priv->currencies = g_list_append(manager->priv->currencies, c);

    return c;
}

static void
load_imf_rates(CurrencyManager *manager, const gchar *data)
{
    gchar **lines;
    int i;
    gboolean in_data = FALSE;
    struct
    {
        const gchar *name, *symbol;
    } name_map[] =
    {
        {"Algerian dinar",	"DZD"},
        {"Australian dollar",   "AUD"},
        {"Bahrain Dinar",	"BHD"},
        {"Bolivar Fuerte",	"VEF"},
        {"Botswana pula",	"BWP"},
        {"Brazilian real",	"BRL"},
        {"Brunei dollar",	"BND"},
        {"Canadian dollar",     "CAD"},
        {"Chilean peso",        "CLP"},
        {"Chinese yuan",        "CNY"},
        {"Colombian peso",	"COP"},
        {"Czech koruna",        "CZK"},
        {"Danish krone",        "DKK"},
        {"Euro",                "EUR"},
        {"Hungarian Forint",    "HUF"},
        {"Icelandic Krona",     "ISK"},
        {"Indian rupee",        "INR"},
        {"Indonesian Rupiah",   "IDR"},
        {"Iranian Rial",        "IRR"},
        {"Israeli New Shekel",  "ILS"},
        {"Japanese yen",        "JPY"},
        {"Kazakhstani Tenge",   "KZT"},
        {"Korean won",          "KRW"},
        {"Kuwaiti dinar",	"KWD"},
        {"Libyan Dinar",        "LYD"},
        {"Malaysian ringgit",   "MYR"},
        {"Mauritian rupee",     "MUR"},
        {"Mexican peso",        "MXN"},
        {"Nepalese Rupee",	"NPR"},
        {"New Zealand dollar",  "NZD"},
        {"Norwegian krone",     "NOK"},
        {"Nuevo Sol",           "PEN"},
        {"Omani rial",		"OMR"},
        {"Pakistani Rupee",     "PKR"},
        {"Peruvian sol",	"PEN"},
        {"Philippine peso",     "PHP"},
        {"Polish zloty",        "PLN"},
        {"Qatari riyal",        "QAR"},
        {"Rial Omani",          "OMR"},
        {"Russian ruble",	"RUB"},
        {"Saudi Arabian riyal", "SAR"},
        {"Singapore dollar",    "SGD"},
        {"South African rand",  "ZAR"},
        {"Sri Lanka Rupee",     "LKR"},
        {"Swedish krona",	"SEK"},
        {"Swiss franc",         "CHF"},
        {"Thai baht",           "THB"},
        {"Trinidadian dollar",  "TTD"},
        {"Tunisian Dinar",	"TND"},
        {"U.A.E. dirham",	"AED"},
        {"U.K. pound",          "GBP"},
        {"Uruguayan peso",	"UYU"},
        {"U.S. dollar",         "USD"},
        {NULL, NULL}
    };

    if (!data)
        return;

    lines = g_strsplit(data, "\n", 0);

    for (i = 0; lines[i]; i++) {
        gchar *line, **tokens;

        line = g_strchug(lines[i]);

        /* Start after first blank line, stop on next */
        if (line[0] == '\0') {
            if (!in_data) {
               in_data = TRUE;
               continue;
            }
            else
               break;
        }
        if (!in_data)
            continue;

        tokens = g_strsplit(line, "\t", 0);
        if (strcmp(tokens[0], "Currency") != 0) {
            gint value_index, name_index;

            for (value_index = 1; tokens[value_index]; value_index++) {
                gchar *value = g_strchug (tokens[value_index]);
                if (value[0] != '\0')
                    break;
            }
            if (tokens[value_index]) {
                for (name_index = 0; name_map[name_index].name; name_index++) {
                    if (strcmp(name_map[name_index].name, tokens[0]) == 0)
                        break;
                }
                if (name_map[name_index].name) {
                    Currency *c = currency_manager_get_currency(manager, name_map[name_index].symbol);
                    MPNumber value = mp_new();

                    if (!c) {
                        g_debug ("Using IMF rate of %s for %s", tokens[value_index], name_map[name_index].symbol);
                        c = add_currency(manager, name_map[name_index].symbol);
                    }
                    mp_set_from_string(tokens[value_index], 10, &value);
                    mp_reciprocal(&value, &value);
                    currency_set_value(c, &value);
                    mp_clear(&value);
                }
                else
                    g_warning("Unknown currency '%s'", tokens[0]);
            }
        }
        g_strfreev(tokens);
    }
    g_strfreev(lines);
}

/* Extracts the value of attribute `name` (quoted with ' or ") from the tag
 * text [tag, tag_end). Returns a newly allocated string or NULL. */
static gchar *
get_xml_attribute(const gchar *tag, const gchar *tag_end, const gchar *name)
{
    size_t name_len = strlen(name);
    const gchar *p;

    for (p = tag; p + name_len < tag_end; p++) {
        const gchar *q;
        gchar quote;

        if (strncmp(p, name, name_len) != 0 || (p > tag && p[-1] != ' ' && p[-1] != '\t' && p[-1] != '\n'))
            continue;
        q = p + name_len;
        while (q < tag_end && (*q == ' ' || *q == '\t'))
            q++;
        if (q >= tag_end || *q != '=')
            continue;
        q++;
        while (q < tag_end && (*q == ' ' || *q == '\t'))
            q++;
        if (q >= tag_end || (*q != '\'' && *q != '"'))
            continue;
        quote = *q++;
        p = q;
        while (q < tag_end && *q != quote)
            q++;
        if (q >= tag_end)
            return NULL;
        return g_strndup(p, (gsize)(q - p));
    }
    return NULL;
}

static void
set_ecb_rate(CurrencyManager *manager, const gchar *name, const gchar *value, Currency *eur_rate)
{
    /* Use data if value and no rate currently defined */
    if (name && value && !currency_manager_get_currency(manager, name)) {
        Currency *c;
        MPNumber r = mp_new();
        MPNumber v = mp_new();

        g_debug ("Using ECB rate of %s for %s", value, name);
        c = add_currency(manager, name);
        mp_set_from_string(value, 10, &r);
        mp_set_from_mp(currency_get_value(eur_rate), &v);
        mp_multiply(&v, &r, &v);
        currency_set_value(c, &v);
        mp_clear(&r);
        mp_clear(&v);
    }
}

static void
set_ecb_fixed_rate(CurrencyManager *manager, const gchar *name, const gchar *value, Currency *eur_rate)
{
    Currency *c;
    MPNumber r = mp_new();
    MPNumber v = mp_new();

    g_debug ("Using ECB fixed rate of %s for %s", value, name);
    c = add_currency(manager, name);
    mp_set_from_string(value, 10, &r);
    mp_set_from_mp(currency_get_value(eur_rate), &v);
    mp_divide(&v, &r, &v);
    currency_set_value(c, &v);
    mp_clear(&r);
    mp_clear(&v);
}

static void
load_ecb_rates(CurrencyManager *manager, const gchar *data)
{
    Currency *eur_rate;
    const gchar *p;

    /* Scale rates to the EUR value */
    eur_rate = currency_manager_get_currency(manager, "EUR");
    if (!eur_rate) {
        g_warning("Cannot use ECB rates as don't have EUR rate");
        return;
    }

    /* Set some fixed rates */
    set_ecb_fixed_rate(manager, "EEK", "0.06391", eur_rate);
    set_ecb_fixed_rate(manager, "CFA", "0.152449", eur_rate);

    if (!data)
        return;

    /* Equivalent of the XPath query //Cube[@currency][@rate] */
    for (p = strstr(data, "<Cube"); p; p = strstr(p + 1, "<Cube")) {
        const gchar *tag_end = strchr(p, '>');
        gchar *name, *value;

        if (!tag_end)
            break;
        name = get_xml_attribute(p + 5, tag_end, "currency");
        value = get_xml_attribute(p + 5, tag_end, "rate");
        if (name && value)
            set_ecb_rate(manager, name, value, eur_rate);
        g_free(name);
        g_free(value);
    }
}

void
currency_manager_load_rates(CurrencyManager *manager, const gchar *imf_data, const gchar *ecb_data)
{
    GList *link;
    int i;

    g_return_if_fail(manager != NULL);

    /* Forget previously loaded rates so a refresh fully replaces them */
    for (link = manager->priv->currencies; link; link = link->next) {
        MPNumber zero = mp_new();
        currency_set_value(link->data, &zero);
        mp_clear(&zero);
    }

    /* Use the IMF provided values and top up with currencies tracked by the ECB and not the IMF */
    load_imf_rates(manager, imf_data);
    load_ecb_rates(manager, ecb_data);

    for (i = 0; currency_info[i].short_name; i++) {
       for (link = manager->priv->currencies; link; link = link->next) {
           Currency *c = link->data;
           if (strcmp(currency_get_name(c), currency_info[i].short_name) == 0)
              break;
       }
       if (!link)
           g_warning("Currency %s is not provided by IMF or ECB", currency_info[i].short_name);
    }

    g_debug("Rates loaded");
    loaded_rates = TRUE;
}

gboolean
currency_manager_has_rates(CurrencyManager *manager)
{
    g_return_val_if_fail(manager != NULL, FALSE);
    return loaded_rates;
}

const MPNumber *
currency_manager_get_value(CurrencyManager *manager, const gchar *currency)
{
    Currency *c;

    g_return_val_if_fail(manager != NULL, NULL);
    g_return_val_if_fail(currency != NULL, NULL);

    if (!loaded_rates)
        return NULL;

    c = currency_manager_get_currency(manager, currency);
    if (c)
        return currency_get_value(c);
    else
        return NULL;
}

static void
currency_manager_init(CurrencyManager *manager)
{
    manager->priv = currency_manager_get_instance_private (manager);
}
