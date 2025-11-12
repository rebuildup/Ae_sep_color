#pragma once

#ifndef SKELETON_H
#define SKELETON_H

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned short u_int16;
typedef unsigned long u_long;
typedef short int int16;
#define PF_TABLE_BITS 12
#define PF_TABLE_SZ_16 4096

#define PF_DEEP_COLOR_AWARE1 // make sure we get16bpc pixels; \
								// AE_Effect.h checks for this.

#include "AEConfig.h"

#ifdef AE_OS_WIN
#define NOMINMAX // prevent Windows headers from defining min/max macros
typedef unsigned short PixelType;
#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#include "sep_color_Strings.h"

/* Versioning information */

#define MAJOR_VERSION 1
#define MINOR_VERSION 1
#define BUG_VERSION 0
#define STAGE_VERSION PF_Stage_DEVELOP
#define BUILD_VERSION 1

// Parameter IDs (match sep_color.cpp usage)
enum
{
	ID_INPUT = 0,		// 0: input layer
	ID_ANCHOR_POINT,	// 1: Anchor Point
	ID_MODE,			// 2: Popup Line|Circle
	ID_AA,				// 3: (未使用) アンチエイリアスは常時ONのため削除
	ID_ANGLE,			// 4: Angle
	ID_RADIUS,			// 5: Radius
	ID_COLOR,			// 6: Color
	SKELETON_NUM_PARAMS // total count (ID_INPUTを除く実際のパラメータ数は6)
};

extern "C"
{

	DllExport
		PF_Err
		EffectMain(
			PF_Cmd cmd,
			PF_InData *in_data,
			PF_OutData *out_data,
			PF_ParamDef *params[],
			PF_LayerDef *output,
			void *extra);
}

#endif // SKELETON_H