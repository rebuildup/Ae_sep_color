#include "AEConfig.h"
#include "AE_EffectVers.h"

/* Include AE_General.r for resource definitions on Mac */
#ifdef AE_OS_MAC
	#include <AE_General.r>
#endif

#if defined(__MACH__) && !defined(AE_OS_MAC)
	#define AE_OS_MAC 1
	#include <AE_General.r>
#endif
	
resource 'PiPL' (16000) {
	{ /* array properties:12 elements */
		/* [1] */
		Kind {
			AEEffect
		},
		/* [2] */
		Name {
			"sep_color"
		},
		/* [3] */
		Category {
			"361do_plugins"
		},
#ifdef AE_OS_WIN
 #if defined(AE_PROC_INTELx64)
		CodeWin64X86 {"EffectMain"},
 #elif defined(AE_PROC_ARM64)
		CodeWinARM64 {"EffectMain"},
 #endif
#elif defined(AE_OS_MAC)
		CodeMacIntel64 {"EffectMain"},
		CodeMacARM64 {"EffectMain"},
#endif
		/* [6] */
		AE_PiPL_Version {
			2,
			0
		},
		/* [7] */
		AE_Effect_Spec_Version {
			PF_PLUG_IN_VERSION,
			PF_PLUG_IN_SUBVERS
		},
		/* [8] */
		AE_Effect_Version {
			557057	/*1.1 */
		},
		/* [9] */
		AE_Effect_Info_Flags {
			0
		},
		/* [10] */
		AE_Effect_Global_OutFlags {
			0x02000000 // PF_OutFlag_DEEP_COLOR_AWARE
		},
		AE_Effect_Global_OutFlags_2 {
			0x08000001 // PF_OutFlag2_SUPPORTS_THREADED_RENDERING | PF_OutFlag2_FLOAT_COLOR_AWARE
		},
		/* [11] */
		AE_Effect_Match_Name {
			"361do sep_color"
		},
		/* [12] */
		AE_Reserved_Info {
			0
		},
		/* [13] */
		AE_Effect_Support_URL {
			"https://x.com/361do_sleep"
		}
	}
};
