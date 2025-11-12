// Minimal Halide integration stubs for sep_color
#pragma once

#include "AE_Effect.h"

#ifdef __cplusplus
extern "C" {
#endif

// Return true on success. Safe to call when Halide is not enabled; it will return false.
bool SepColorHalide_Render8(PF_InData *in_data,
                            PF_OutData *out_data,
                            PF_ParamDef *params[],
                            PF_LayerDef *output,
                            PF_Pixel *input_pixels,
                            PF_Pixel *output_pixels);

bool SepColorHalide_Render16(PF_InData *in_data,
                             PF_OutData *out_data,
                             PF_ParamDef *params[],
                             PF_LayerDef *output,
                             PF_Pixel16 *input_pixels,
                             PF_Pixel16 *output_pixels);

bool SepColorHalide_Render32(PF_InData *in_data,
                             PF_OutData *out_data,
                             PF_ParamDef *params[],
                             PF_LayerDef *output,
                             PF_PixelFloat *input_pixels,
                             PF_PixelFloat *output_pixels);

#ifdef __cplusplus
}
#endif

