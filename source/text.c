/*
 * Copyright 2017-2018 nx-hbmenu Authors
 *
 * Permission to use, copy, modify, and/or distribute this
 * software for any purpose with or without fee is hereby
 * granted, provided that the above copyright notice and
 * this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <switch.h>

#include "text.h"
#include "settings.h"
#include "apps.h"

#define STR_JP(_str) [SetLanguage_JA] = _str
#define STR_EN(_str) [SetLanguage_ENUS] = _str, [SetLanguage_ENGB] = _str
#define STR_FR(_str) [SetLanguage_FR] = _str, [SetLanguage_FRCA] = _str
#define STR_DE(_str) [SetLanguage_DE] = _str
#define STR_IT(_str) [SetLanguage_IT] = _str
#define STR_ES(_str) [SetLanguage_ES] = _str, [SetLanguage_ES419] = _str
#define STR_ZH_HANS(_str) [SetLanguage_ZHCN] = _str, [SetLanguage_ZHHANS] = _str
#define STR_KO(_str) [SetLanguage_KO] = _str
#define STR_NL(_str) [SetLanguage_NL] = _str
#define STR_PT(_str) [SetLanguage_PT] = _str
#define STR_RU(_str) [SetLanguage_RU] = _str
#define STR_ZH_HANT(_str) [SetLanguage_ZHTW] = _str, [SetLanguage_ZHHANT] = _str

static const char *g_strings[StrId_max][SetLanguage_Total] = {
    [StrId_limit_warn] = {
        STR_EN("Applet Mode"),
        STR_DE("Applet Modus"),
        STR_ES("Modo Applet"),
    },

    [StrId_version] = {
        STR_EN("Version: %s"),
        STR_DE("Version: %s"),
        STR_ES("Versi\xF3n: %s"),
    },

    [StrId_author] = {
        STR_EN("Author: "),
        STR_DE("Autor: "),
        STR_ES("Autor: "),
    },

    [StrId_receiving] = {
        STR_EN("Receiving: %s"),
        STR_DE("Empfangen: %s"),
        STR_ES("Recibiendo: %s"),
    },

    [StrId_error] = {
        STR_EN("An error has\noccured"),
        STR_DE("Ein Fehler ist\naufgetreten"),
        STR_ES("Ha ocurrido\nun error"),
    },

    [StrId_ok] = {
        STR_EN("OK"),
        STR_DE("OK"),
        STR_ES("OK"),
    },

    [StrId_no_apps] = {
        STR_EN(
            "You have no apps!\n"
            "Please put your apps under\n"
            "\"" APP_DIR "\""
        ),
        STR_DE(
            "Du hast keine Apps!\n"
            "Bitte packe deine Apps in\n"
            "\"" APP_DIR "\""
        ),
        STR_ES(
            "\xA1No tienes aplicaciones!\n"
            "Por favor, gu\xE1rdalas en\n"
            "\"" APP_DIR "\""
        ),
    },

    [StrId_delete] = {
        STR_EN("Delete"),
        STR_DE("Löschen"),
        STR_ES("Eliminar"),
    },

    [StrId_load] = {
        STR_EN("Load"),
        STR_DE("Laden"),
        STR_ES("Cargar"),
    },

    [StrId_star] = {
        STR_EN("Star"),
        STR_DE("Favorisieren"),
        STR_ES("Agregar a favoritos"),
    },

    [StrId_back] = {
        STR_EN("Back"),
        STR_DE("Zurück"),
        STR_ES("Regresar"),
    },

    [StrId_unstar] = {
        STR_EN("Unstar"),
        STR_DE("Unfavorisieren"),
        STR_ES("Remover de favoritos"),
    },
};

const char *text_get(StrId id) {
    const char *str = g_strings[id][curr_settings()->lang_id];
    if (str == NULL)
        str = g_strings[id][SetLanguage_ENUS];

    return str;
}
