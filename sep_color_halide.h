#pragma once

#include "AE_Effect.h"

#ifdef __cplusplus
#include <cstdint>
#if SEP_COLOR_ENABLE_HALIDE
#include "third_party/halide/include/Halide.h"
#endif

struct SepColorHalideGlobalState
{
	bool runtime_loaded = false;
	bool gpu_enabled    = false;
#if SEP_COLOR_ENABLE_HALIDE
	Halide::Target target;
#endif
};

bool SepColorHalide_GlobalInit(PF_InData *in_data, SepColorHalideGlobalState &state);
void SepColorHalide_GlobalRelease(SepColorHalideGlobalState &state);

// Return true on success. Safe to call when Halide is not enabled; it will return false.
bool SepColorHalide_Render8(PF_InData *in_data,
                            PF_OutData *out_data,
                            PF_ParamDef *params[],
                            PF_LayerDef *output,
                            const SepColorHalideGlobalState &state,
                            PF_Pixel *input_pixels,
                            PF_Pixel *output_pixels);

bool SepColorHalide_Render16(PF_InData *in_data,
                             PF_OutData *out_data,
                             PF_ParamDef *params[],
                             PF_LayerDef *output,
                             const SepColorHalideGlobalState &state,
                             PF_Pixel16 *input_pixels,
                             PF_Pixel16 *output_pixels);

bool SepColorHalide_Render32(PF_InData *in_data,
                             PF_OutData *out_data,
                             PF_ParamDef *params[],
                             PF_LayerDef *output,
                             const SepColorHalideGlobalState &state,
                             PF_PixelFloat *input_pixels,
                             PF_PixelFloat *output_pixels);

#endif // __cplusplus

