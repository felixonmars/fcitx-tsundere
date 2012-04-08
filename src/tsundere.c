/***************************************************************************
 *   Copyright (C) 2010~2010 by Felix Yan                                  *
 *   felixonmars@gmail.com                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "fcitx/fcitx.h"

#include <libintl.h>
#include <errno.h>

#include "fcitx/module.h"
#include "fcitx-utils/utf8.h"
#include "fcitx-utils/uthash.h"
#include "fcitx-config/xdg.h"
#include "fcitx/hook.h"
#include "fcitx/ui.h"
#include "fcitx-utils/log.h"
#include "fcitx/instance.h"
#include "fcitx/context.h"
#include "fcitx-utils/utils.h"
#include "tsundere.h"

#define _(x) dgettext("fcitx-tsundere", (x))

typedef struct _FcitxTsundere {
    FcitxGenericConfig gconfig;
    char *marker;
    boolean enabled;
    FcitxHotkey hkToggle[2];
    FcitxInstance* owner;
} FcitxTsundere;

void* TsundereCreate(FcitxInstance* instance);
static char* TsundereCommitFilter(void* arg, const char* strin);
static boolean GetTsundereEnabled(void* arg);
void ReloadTsundere(void* arg);
static void TsundereLanguageChanged(void* arg, const void* value);
boolean LoadTsundereConfig(FcitxTsundere* tsundereState);
static FcitxConfigFileDesc* GetTsundereConfigDesc();
static void SaveTsundereConfig(FcitxTsundere* tsundereState);
static INPUT_RETURN_VALUE HotkeyToggleTsundereState(void*);
static void ToggleTsundereState(void*);

CONFIG_BINDING_BEGIN(FcitxTsundere)
CONFIG_BINDING_REGISTER("Tsundere", "Enabled", enabled)
CONFIG_BINDING_REGISTER("Tsundere", "Marker", marker)
CONFIG_BINDING_REGISTER("Tsundere", "Hotkey", hkToggle)
CONFIG_BINDING_END()

FCITX_EXPORT_API
FcitxModule module = {
    TsundereCreate,
    NULL,
    NULL,
    NULL,
    ReloadTsundere
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

void* TsundereCreate(FcitxInstance* instance)
{
    FcitxTsundere* tsundereState = fcitx_utils_malloc0(sizeof(FcitxTsundere));
    tsundereState->owner = instance;
    if (!LoadTsundereConfig(tsundereState)) {
        free(tsundereState);
        return NULL;
    }

    FcitxHotkeyHook hk;
    hk.arg = tsundereState;
    hk.hotkey = tsundereState->hkToggle;
    hk.hotkeyhandle = HotkeyToggleTsundereState;

    FcitxStringFilterHook shk;
    shk.arg = tsundereState;
    shk.func = TsundereCommitFilter;

    FcitxInstanceRegisterHotkeyFilter(instance, hk);
    FcitxInstanceRegisterCommitFilter(instance, shk);
    FcitxUIRegisterStatus(instance, tsundereState, "tsundere", _("Tsundere Engine"), _("Tsundere Engine"), ToggleTsundereState, GetTsundereEnabled);

    FcitxInstanceWatchContext(instance, CONTEXT_IM_LANGUAGE, TsundereLanguageChanged, tsundereState);

    return tsundereState;
}


INPUT_RETURN_VALUE HotkeyToggleTsundereState(void* arg)
{
    FcitxTsundere* tsundereState = (FcitxTsundere*) arg;

    FcitxUIStatus *status = FcitxUIGetStatusByName(tsundereState->owner, "tsundere");
    if (status->visible){
        FcitxUIUpdateStatus(tsundereState->owner, "tsundere");
        return IRV_DO_NOTHING;
    }
    else
        return IRV_TO_PROCESS;
}

void ToggleTsundereState(void* arg)
{
    FcitxTsundere* tsundereState = (FcitxTsundere*) arg;
    tsundereState->enabled = !tsundereState->enabled;
    SaveTsundereConfig(tsundereState);
}

boolean GetTsundereEnabled(void* arg)
{
    FcitxTsundere* tsundereState = (FcitxTsundere*) arg;
    return tsundereState->enabled;
}

char* TsundereCommitFilter(void* arg, const char *strin)
{
    FcitxTsundere* tsundereState = (FcitxTsundere*) arg;
    FcitxIM* im = FcitxInstanceGetCurrentIM(tsundereState->owner);

    if (!im || !tsundereState->enabled)
        return NULL;

    int i = 0;
    int len = fcitx_utf8_strlen(strin);
    char* ps = strin;
    char* juhua = "\xd2\x89";
    char* marker;

    if (!strcmp(tsundereState->marker, "juhua"))
	marker = juhua;
    else
	marker = tsundereState->marker;

    char* ret = (char *) malloc(sizeof(char) * (len * UTF8_MAX_LENGTH * (1 + fcitx_utf8_strlen(marker)) + 1));

    ret[0] = '\0';

    for (; i < len; ++i) {
        int wc;
        int chr_len = fcitx_utf8_char_len(ps);
        char *nps = fcitx_utf8_get_char(ps, &wc);

        strncat(ret, ps, chr_len);
        strcat(ret, marker);
        
        ps = nps;
    }
    ret[strlen(ret)] = '\0';

    return ret;
}

boolean LoadTsundereConfig(FcitxTsundere* tsundereState)
{
    FcitxConfigFileDesc* configDesc = GetTsundereConfigDesc();
    if (configDesc == NULL)
        return false;

    FILE *fp;
    char *file;
    fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-tsundere.config", "r", &file);
    FcitxLog(DEBUG, "Load Config File %s", file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
            SaveTsundereConfig(tsundereState);
    }

    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);

    FcitxTsundereConfigBind(tsundereState, cfile, configDesc);
    FcitxConfigBindSync((FcitxGenericConfig*)tsundereState);

    if (fp)
        fclose(fp);

    return true;
}

CONFIG_DESC_DEFINE(GetTsundereConfigDesc, "fcitx-tsundere.desc")

void SaveTsundereConfig(FcitxTsundere* tsundereState)
{
    FcitxConfigFileDesc* configDesc = GetTsundereConfigDesc();
    char *file;
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-tsundere.config", "w", &file);
    FcitxLog(DEBUG, "Save Config to %s", file);
    FcitxConfigSaveConfigFileFp(fp, &tsundereState->gconfig, configDesc);
    free(file);
    if (fp)
        fclose(fp);
}

void ReloadTsundere(void* arg)
{
    FcitxTsundere* tsundereState = (FcitxTsundere*) arg;
    LoadTsundereConfig(tsundereState);
}

void TsundereLanguageChanged(void* arg, const void* value)
{
    FcitxTsundere* tsundereState = (FcitxTsundere*) arg;
    const char* lang = (const char*) value;
    boolean visible = false;
    if (lang && strncmp(lang, "zh", 2) == 0 && strlen(lang) > 2)
        visible = true;
    
    FcitxUISetStatusVisable(tsundereState->owner, "tsundere", visible);
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
