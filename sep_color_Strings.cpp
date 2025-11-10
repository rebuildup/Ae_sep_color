#include "sep_color_Strings.h"

struct TableString
{
    int id;
    char text[256];
};

static TableString g_strs[StrID_NUMTYPES] = {
    {StrID_NONE, ""},
    {StrID_Name, "sep_color"},
    {StrID_Description, "A plugin for coloring areas \rCopyright2025361do_sleep"},
    {StrID_Gain_Param_Name, "Gain"},
    {StrID_Color_Param_Name, "Color"}};

char *GetStringPtr(int strNum)
{
    if (strNum >= 0 && strNum < StrID_NUMTYPES)
    {
        return g_strs[strNum].text;
    }
    return g_strs[StrID_NONE].text;
}