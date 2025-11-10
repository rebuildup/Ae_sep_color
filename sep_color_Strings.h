#pragma once

typedef enum
{
    StrID_NONE = 0,
    StrID_Name,
    StrID_Description,
    StrID_Gain_Param_Name,
    StrID_Color_Param_Name,
    StrID_NUMTYPES
} StrIDType;

#ifdef __cplusplus
extern "C"
{
#endif

    char *GetStringPtr(int strNum);

#ifdef __cplusplus
}
#endif
