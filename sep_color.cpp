#include "sep_color.h"

#include "sep_color_Strings.h"

#include <algorithm>

#include <cmath>

#include <cstring>

#include <vector>

// Named constants for magic numbers

namespace Constants {

	// Mathematical constants

	constexpr float PI = 3.14159265358979323846f;              // Pi constant

	constexpr float INV_SQRT_2 = 0.70710678118654752440f;       // 1/sqrt(2) for edge width

	constexpr float EDGE_WIDTH = INV_SQRT_2;                   // Anti-aliasing edge width (1/sqrt(2))

	// Coverage thresholds for early-outs

	constexpr float COVERAGE_EPSILON = 0.0001f;                // Below this: skip blending (use input)

	constexpr float COVERAGE_FULL = 0.9999f;                   // Above this: full coverage (use effect color)

	// Angle conversion

	constexpr float DEG_TO_RAD = PI / 180.0f;                  // Degrees to radians conversion

	// Color conversion constants

	constexpr float COLOR_8BIT_MAX = 255.0f;                   // 8-bit color maximum

	constexpr float COLOR_16BIT_MAX = 32768.0f;                // 16-bit color maximum

	constexpr float COLOR_SCALE_8_TO_16 = COLOR_16BIT_MAX / COLOR_8BIT_MAX;  // 32768/255

	constexpr float COLOR_SCALE_8_TO_FLOAT = 1.0f / COLOR_8BIT_MAX;          // 1/255

	constexpr float COLOR_ROUND_OFFSET_16 = 127.0f;            // Rounding offset for 16-bit conversion

}

// Feature switches removed - always use PF_Iterate suites for MFR safety
// Manual threading (std::thread) violates SDK guidelines

// (forward declarations moved below with other fast paths)

/**
 * Performance optimization overview for sep_color plugin
 *
 * 0. Bit depth support: 8/16/32-bit
 *    - 8-bit (PF_Pixel: 0-255)
 *    - 16-bit (PF_Pixel16: 0-32768)
 *    - 32-bit float (PF_PixelFloat: 0.0-1.0)
 *    - Template specialization (PixelTraits) for zero-overhead
 *
 * 1. MFR-safe threading using PF_Iterate suites
 *    - Uses AE's built-in thread pool (PF_OutFlag2_SUPPORTS_THREADED_RENDERING)
 *    - No manual std::thread creation - avoids SDK violations
 *    - AE handles abort checking and parallelism
 *
 * 2. Analytical anti-aliasing
 *    - Line mode: distance-based gradient
 *    - Circle mode: radial gradient
 *    - Smooth transitions compatible with AE standards
 *
 * 3. Memory access optimizations
 *    - Pointer references to avoid struct copies
 *    - Precomputed constants (edge_width, trig functions)
 *    - Early-outs for transparent pixels
 */

// ============================================================================

// PixelTraits: Type traits template for pixel depth specialization

// ============================================================================

template<typename PixelType>

struct PixelTraits;

// Specialization for 8-bit pixels (PF_Pixel)

template<>

struct PixelTraits<PF_Pixel>

{

	using ChannelType = A_u_char;

	using PixelType = PF_Pixel;

	static constexpr float INV_MAX = Constants::COLOR_SCALE_8_TO_FLOAT;  // 1.0f / 255.0f

	static constexpr ChannelType MAX_CHANNEL = static_cast<ChannelType>(Constants::COLOR_8BIT_MAX);  // 255

	static constexpr bool IsFloat = false;

	static inline ChannelType Blend(ChannelType src, ChannelType dst, float coverage)

	{

		return static_cast<ChannelType>(src + (dst - src) * coverage + 0.5f);

	}

	static inline bool IsTransparent(const PixelType& px)

	{

		return px.alpha == 0;

	}

	static inline void SetColor(PixelType& px, ChannelType r, ChannelType g, ChannelType b, ChannelType a)

	{

		px.red = r;

		px.green = g;

		px.blue = b;

		px.alpha = a;

	}

	static inline void CopyPixel(const PixelType& src, PixelType& dst)

	{

		dst = src;

	}

	static inline void ConvertColor8(const PF_Pixel& color8, PixelType& out)

	{

		out = color8;

	}

};

// Specialization for 16-bit pixels (PF_Pixel16)

template<>

struct PixelTraits<PF_Pixel16>

{

	using ChannelType = A_u_short;

	using PixelType = PF_Pixel16;

	static constexpr float INV_MAX = 1.0f / Constants::COLOR_16BIT_MAX;  // 1.0f / 32768.0f

	static constexpr ChannelType MAX_CHANNEL = static_cast<ChannelType>(Constants::COLOR_16BIT_MAX);  // 32768

	static constexpr bool IsFloat = false;

	static inline ChannelType Blend(ChannelType src, ChannelType dst, float coverage)

	{

		return static_cast<ChannelType>(src + (dst - src) * coverage + 0.5f);

	}

	static inline bool IsTransparent(const PixelType& px)

	{

		return px.alpha == 0;

	}

	static inline void SetColor(PixelType& px, ChannelType r, ChannelType g, ChannelType b, ChannelType a)

	{

		px.red = r;

		px.green = g;

		px.blue = b;

		px.alpha = a;

	}

	static inline void CopyPixel(const PixelType& src, PixelType& dst)

	{

		dst = src;

	}

	static inline void ConvertColor8(const PF_Pixel& color8, PixelType& out)

	{

		out.red = static_cast<A_u_short>((color8.red * Constants::COLOR_SCALE_8_TO_16 + Constants::COLOR_ROUND_OFFSET_16));

		out.green = static_cast<A_u_short>((color8.green * Constants::COLOR_SCALE_8_TO_16 + Constants::COLOR_ROUND_OFFSET_16));

		out.blue = static_cast<A_u_short>((color8.blue * Constants::COLOR_SCALE_8_TO_16 + Constants::COLOR_ROUND_OFFSET_16));

		out.alpha = MAX_CHANNEL;

	}

};

// Specialization for 32-bit float pixels (PF_PixelFloat)

template<>

struct PixelTraits<PF_PixelFloat>

{

	using ChannelType = float;

	using PixelType = PF_PixelFloat;

	static constexpr float INV_MAX = 1.0f;

	static constexpr ChannelType MAX_CHANNEL = 1.0f;

	static constexpr bool IsFloat = true;

	static inline ChannelType Blend(ChannelType src, ChannelType dst, float coverage)

	{

		return src + (dst - src) * coverage;

	}

	static inline bool IsTransparent(const PixelType& px)

	{

		return px.alpha <= 0.0f;

	}

	static inline void SetColor(PixelType& px, ChannelType r, ChannelType g, ChannelType b, ChannelType a)

	{

		px.red = r;

		px.green = g;

		px.blue = b;

		px.alpha = a;

	}

	static inline void CopyPixel(const PixelType& src, PixelType& dst)

	{

		dst = src;

	}

	static inline void ConvertColor8(const PF_Pixel& color8, PixelType& out)

	{

		out.red = static_cast<float>(color8.red) * Constants::COLOR_SCALE_8_TO_FLOAT;

		out.green = static_cast<float>(color8.green) * Constants::COLOR_SCALE_8_TO_FLOAT;

		out.blue = static_cast<float>(color8.blue) * Constants::COLOR_SCALE_8_TO_FLOAT;

		out.alpha = 1.0f;

	}

};

// Legacy wrappers for backward compatibility

static inline A_u_char FastBlend(A_u_char src, A_u_char dst, float coverage_alpha)

{

	return PixelTraits<PF_Pixel>::Blend(src, dst, coverage_alpha);

}

static inline A_u_short FastBlend16(A_u_short src, A_u_short dst, float coverage_alpha)

{

	return PixelTraits<PF_Pixel16>::Blend(src, dst, coverage_alpha);

}

static inline float FastBlendFloat(float src, float dst, float coverage_alpha)

{

	return PixelTraits<PF_PixelFloat>::Blend(src, dst, coverage_alpha);

}

struct SepColorGlobalData

{

};

// -------------------------------------------------------------

// IterateRefcon structure definition for PF_Iterate callbacks

// -------------------------------------------------------------

struct IterateRefcon {
    int width;
    int height;
    float downsample_x;
    float downsample_y;
    int anchor_x;
    int anchor_y;
    float angle;
    float radius;
    int mode;
    float edge_width;
    float inv_edge_width;
    PF_Pixel color8;
    A_u_short r16, g16, b16;
    float cs, sn;
    float r_minus2, r_plus2;
};

// -------------------------------------------------------------

// PF_Iterate callback for 8-bit pixels

// -------------------------------------------------------------

static PF_Err IteratePix8(void *refcon, A_long x, A_long y, PF_Pixel *in, PF_Pixel *out)

{

	const IterateRefcon *rc = reinterpret_cast<const IterateRefcon *>(refcon);

	const float fx = (static_cast<float>(x) - rc->anchor_x) * rc->downsample_x;

	const float fy = (static_cast<float>(y) - rc->anchor_y) * rc->downsample_y;

	float coverage = 0.0f;

	if (rc->mode == 1)

	{

		const float rot_x = fx * rc->cs + fy * rc->sn;

		if (rot_x <= -rc->edge_width)

		{

			*out = *in;

			return PF_Err_NONE;

		}

		if (rot_x >= rc->edge_width)

		{

			out->red = rc->color8.red;

			out->green = rc->color8.green;

			out->blue = rc->color8.blue;

			out->alpha = in->alpha;

			return PF_Err_NONE;

		}

		const float sd = rot_x * rc->inv_edge_width;

		coverage = (sd + 1.0f) * 0.5f;

	}

	else

	{

		const float dist2 = fx * fx + fy * fy;

		if (dist2 >= rc->r_plus2)

		{

			*out = *in;

			return PF_Err_NONE;

		}

		if (dist2 <= rc->r_minus2)

		{

			out->red = rc->color8.red;

			out->green = rc->color8.green;

			out->blue = rc->color8.blue;

			out->alpha = in->alpha;

			return PF_Err_NONE;

		}

		const float dist = sqrtf(dist2);

		const float sd = (rc->radius - dist) * rc->inv_edge_width;

		coverage = (sd + 1.0f) * 0.5f;

	}

	if (coverage <= Constants::COVERAGE_EPSILON)

	{

		*out = *in;

		return PF_Err_NONE;

	}

	if (coverage >= Constants::COVERAGE_FULL)

	{

		out->red = rc->color8.red;

		out->green = rc->color8.green;

		out->blue = rc->color8.blue;

		out->alpha = in->alpha;

		return PF_Err_NONE;

	}

	out->red = FastBlend(in->red, rc->color8.red, coverage);

	out->green = FastBlend(in->green, rc->color8.green, coverage);

	out->blue = FastBlend(in->blue, rc->color8.blue, coverage);

	out->alpha = in->alpha;

	return PF_Err_NONE;

}

static PF_Err Render8Iterate(

	PF_InData *in_data,

	PF_OutData *out_data,

	PF_ParamDef *params[],

	PF_LayerDef *output,

	PF_Pixel *input_pixels,

	PF_Pixel *output_pixels)

{

	PF_Err err = PF_Err_NONE;

	(void)out_data;

	(void)input_pixels;

	(void)output_pixels; // AE passes worlds in iterate call

	IterateRefcon rc{};

	rc.width = output->width;

	rc.height = output->height;

	rc.downsample_x = static_cast<float>(in_data->downsample_x.den) / static_cast<float>(in_data->downsample_x.num);

	rc.downsample_y = static_cast<float>(in_data->downsample_y.den) / static_cast<float>(in_data->downsample_y.num);

	rc.anchor_x = (params[ID_ANCHOR_POINT]->u.td.x_value >> 16);

	rc.anchor_y = (params[ID_ANCHOR_POINT]->u.td.y_value >> 16);

	rc.angle = static_cast<float>(params[ID_ANGLE]->u.ad.value >> 16) * Constants::DEG_TO_RAD;

	rc.radius = static_cast<float>(params[ID_RADIUS]->u.fs_d.value);

	rc.mode = params[ID_MODE]->u.pd.value;

	rc.edge_width = Constants::EDGE_WIDTH;

	rc.inv_edge_width = 1.0f / rc.edge_width;

	rc.color8 = params[ID_COLOR]->u.cd.value;

	// Precompute for speed

	rc.cs = cosf(rc.angle);

	rc.sn = sinf(rc.angle);

	const float r_minus8 = rc.radius - rc.edge_width;

	const float r_plus8 = rc.radius + rc.edge_width;

	rc.r_minus2 = r_minus8 * r_minus8;

	rc.r_plus2 = r_plus8 * r_plus8;

	AEGP_SuiteHandler suites(in_data->pica_basicP);

	PF_Rect area{0, 0, output->width, output->height};

	if (rc.mode != 1)

	{

		// Circle mode only affects a bounded region; copy input -> output once

		// so pixels outside the circle keep their original color/alpha.

		err = PF_COPY(&params[ID_INPUT]->u.ld, output, nullptr, nullptr);

		if (err != PF_Err_NONE)

		{

			return err;

		}

		const float ex = (rc.radius + rc.edge_width) / std::max(rc.downsample_x, 1e-6f);

		const float ey = (rc.radius + rc.edge_width) / std::max(rc.downsample_y, 1e-6f);

		const int x0 = std::max(0, static_cast<int>(std::floor(rc.anchor_x - ex)));

		const int x1 = std::min(rc.width, static_cast<int>(std::ceil(rc.anchor_x + ex)) + 1);

		const int y0 = std::max(0, static_cast<int>(std::floor(rc.anchor_y - ey)));

		const int y1 = std::min(rc.height, static_cast<int>(std::ceil(rc.anchor_y + ey)) + 1);

		area.left = x0;

		area.right = x1;

		area.top = y0;

		area.bottom = y1;

	}

	PF_EffectWorld *src = &params[0]->u.ld;

	err = suites.Iterate8Suite1()->iterate(

		in_data,

		0,

		output->height,

		src,

		&area,

		&rc,

		IteratePix8,

		output);

	return err;

}

// PF_Iterate callback for 16-bit pixels

static PF_Err IteratePix16(void *refcon, A_long x, A_long y, PF_Pixel16 *in, PF_Pixel16 *out)

{

	const IterateRefcon *rc = reinterpret_cast<const IterateRefcon *>(refcon);

	const float fx = (static_cast<float>(x) - rc->anchor_x) * rc->downsample_x;

	const float fy = (static_cast<float>(y) - rc->anchor_y) * rc->downsample_y;

	float coverage;

	if (rc->mode == 1)

	{

		const float rot_x = fx * rc->cs + fy * rc->sn;

		if (rot_x <= -rc->edge_width)

		{

			*out = *in;

			return PF_Err_NONE;

		}

		if (rot_x >= rc->edge_width)

		{

			out->red = rc->r16;

			out->green = rc->g16;

			out->blue = rc->b16;

			out->alpha = in->alpha;

			return PF_Err_NONE;

		}

		const float sd = rot_x * rc->inv_edge_width;

		coverage = (sd + 1.0f) * 0.5f;

	}

	else

	{

		const float dist2 = fx * fx + fy * fy;

		if (dist2 >= rc->r_plus2)

		{

			*out = *in;

			return PF_Err_NONE;

		}

		if (dist2 <= rc->r_minus2)

		{

			out->red = rc->r16;

			out->green = rc->g16;

			out->blue = rc->b16;

			out->alpha = in->alpha;

			return PF_Err_NONE;

		}

		const float dist = sqrtf(dist2);

		const float sd = (rc->radius - dist) * rc->inv_edge_width;

		coverage = (sd + 1.0f) * 0.5f;

	}

	if (coverage <= Constants::COVERAGE_EPSILON)

	{

		*out = *in;

		return PF_Err_NONE;

	}

	if (coverage >= Constants::COVERAGE_FULL)

	{

		out->red = rc->r16;

		out->green = rc->g16;

		out->blue = rc->b16;

		out->alpha = in->alpha;

		return PF_Err_NONE;

	}

	out->red = FastBlend16(in->red, rc->r16, coverage);

	out->green = FastBlend16(in->green, rc->g16, coverage);

	out->blue = FastBlend16(in->blue, rc->b16, coverage);

	out->alpha = in->alpha;

	return PF_Err_NONE;

}

static PF_Err Render16Iterate(

	PF_InData *in_data,

	PF_OutData *out_data,

	PF_ParamDef *params[],

	PF_LayerDef *output,

	PF_Pixel16 *input_pixels,

	PF_Pixel16 *output_pixels)

{

	PF_Err err = PF_Err_NONE;

	(void)out_data;

	(void)input_pixels;

	(void)output_pixels;

	IterateRefcon rc{};

	rc.width = output->width;

	rc.height = output->height;

	rc.downsample_x = static_cast<float>(in_data->downsample_x.den) / static_cast<float>(in_data->downsample_x.num);

	rc.downsample_y = static_cast<float>(in_data->downsample_y.den) / static_cast<float>(in_data->downsample_y.num);

	rc.anchor_x = (params[ID_ANCHOR_POINT]->u.td.x_value >> 16);

	rc.anchor_y = (params[ID_ANCHOR_POINT]->u.td.y_value >> 16);

	rc.angle = static_cast<float>(params[ID_ANGLE]->u.ad.value >> 16) * Constants::DEG_TO_RAD;

	rc.radius = static_cast<float>(params[ID_RADIUS]->u.fs_d.value);

	rc.mode = params[ID_MODE]->u.pd.value;

	rc.edge_width = Constants::EDGE_WIDTH;

	rc.inv_edge_width = 1.0f / rc.edge_width;

	rc.color8 = params[ID_COLOR]->u.cd.value;

	// Precompute for speed

	rc.cs = cosf(rc.angle);

	rc.sn = sinf(rc.angle);

	const float r_minus8 = rc.radius - rc.edge_width;

	const float r_plus8 = rc.radius + rc.edge_width;

	rc.r_minus2 = r_minus8 * r_minus8;

	rc.r_plus2 = r_plus8 * r_plus8;

	rc.r16 = static_cast<A_u_short>((rc.color8.red * Constants::COLOR_SCALE_8_TO_16 + Constants::COLOR_ROUND_OFFSET_16));

	rc.g16 = static_cast<A_u_short>((rc.color8.green * Constants::COLOR_SCALE_8_TO_16 + Constants::COLOR_ROUND_OFFSET_16));

	rc.b16 = static_cast<A_u_short>((rc.color8.blue * Constants::COLOR_SCALE_8_TO_16 + Constants::COLOR_ROUND_OFFSET_16));

	AEGP_SuiteHandler suites(in_data->pica_basicP);

	PF_Rect area{0, 0, output->width, output->height};

	if (rc.mode != 1)

	{

		// Circle mode: copy full frame once, then iterate only over the

		// affected bounding box so alpha outside remains unchanged.

		err = PF_COPY(&params[ID_INPUT]->u.ld, output, nullptr, nullptr);

		if (err != PF_Err_NONE)

		{

			return err;

		}

		const float ex = (rc.radius + rc.edge_width) / std::max(rc.downsample_x, 1e-6f);

		const float ey = (rc.radius + rc.edge_width) / std::max(rc.downsample_y, 1e-6f);

		const int x0 = std::max(0, static_cast<int>(std::floor(rc.anchor_x - ex)));

		const int x1 = std::min(rc.width, static_cast<int>(std::ceil(rc.anchor_x + ex)) + 1);

		const int y0 = std::max(0, static_cast<int>(std::floor(rc.anchor_y - ey)));

		const int y1 = std::min(rc.height, static_cast<int>(std::ceil(rc.anchor_y + ey)) + 1);

		area.left = x0;

		area.right = x1;

		area.top = y0;

		area.bottom = y1;

	}

	PF_EffectWorld *src = &params[0]->u.ld;

	err = suites.Iterate16Suite1()->iterate(

		in_data,

		0,

		output->height,

		src,

		&area,

		&rc,

		IteratePix16,

		output);

	return err;

}

static PF_Err IteratePix32(void *refcon, A_long x, A_long y, PF_PixelFloat *in, PF_PixelFloat *out)

{

	const IterateRefcon *rc = reinterpret_cast<const IterateRefcon *>(refcon);

	const float fx = (static_cast<float>(x) - rc->anchor_x) * rc->downsample_x;

	const float fy = (static_cast<float>(y) - rc->anchor_y) * rc->downsample_y;

	float coverage;

	if (rc->mode == 1)

	{

		const float cs = cosf(rc.angle), sn = sinf(rc.angle);

		const float rot_x = fx * cs + fy * sn;

		const float sd = rot_x * rc->inv_edge_width;

		coverage = std::max(0.0f, std::min(1.0f, (sd + 1.0f) * 0.5f));

	}

	else

	{

		const float dist = sqrtf(fx * fx + fy * fy);

		const float sd = (rc->radius - dist) * rc->inv_edge_width;

		coverage = std::max(0.0f, std::min(1.0f, (sd + 1.0f) * 0.5f));

	}

	if (coverage <= Constants::COVERAGE_EPSILON)

	{

		*out = *in;

		return PF_Err_NONE;

	}

	const float r = static_cast<float>(rc->color8.red) * Constants::COLOR_SCALE_8_TO_FLOAT;

	const float g = static_cast<float>(rc->color8.green) * Constants::COLOR_SCALE_8_TO_FLOAT;

	const float b = static_cast<float>(rc->color8.blue) * Constants::COLOR_SCALE_8_TO_FLOAT;

	if (coverage >= Constants::COVERAGE_FULL)

	{

		out->red = r;

		out->green = g;

		out->blue = b;

		out->alpha = in->alpha;

		return PF_Err_NONE;

	}

	out->red = FastBlendFloat(in->red, r, coverage);

	out->green = FastBlendFloat(in->green, g, coverage);

	out->blue = FastBlendFloat(in->blue, b, coverage);

	out->alpha = in->alpha;

	return PF_Err_NONE;

}

static PF_Err Render32Iterate(

	PF_InData *in_data,

	PF_OutData *out_data,

	PF_ParamDef *params[],

	PF_LayerDef *output,

	PF_PixelFloat *input_pixels,

	PF_PixelFloat *output_pixels)

{

	PF_Err err = PF_Err_NONE;

	(void)out_data;

	(void)input_pixels;

	(void)output_pixels;

	IterateRefcon rc{};

	rc.width = output->width;

	rc.height = output->height;

	rc.downsample_x = static_cast<float>(in_data->downsample_x.den) / static_cast<float>(in_data->downsample_x.num);

	rc.downsample_y = static_cast<float>(in_data->downsample_y.den) / static_cast<float>(in_data->downsample_y.num);

	rc.anchor_x = (params[ID_ANCHOR_POINT]->u.td.x_value >> 16);

	rc.anchor_y = (params[ID_ANCHOR_POINT]->u.td.y_value >> 16);

	rc.angle = static_cast<float>(params[ID_ANGLE]->u.ad.value >> 16) * Constants::DEG_TO_RAD;

	rc.radius = static_cast<float>(params[ID_RADIUS]->u.fs_d.value);

	rc.mode = params[ID_MODE]->u.pd.value;

	rc.edge_width = Constants::EDGE_WIDTH;

	rc.inv_edge_width = 1.0f / rc.edge_width;

	rc.color8 = params[ID_COLOR]->u.cd.value;

	AEGP_SuiteHandler suites(in_data->pica_basicP);

	PF_Rect area{0, 0, output->width, output->height};

	if (rc.mode != 1)

	{

		// Circle mode: copy full frame once, then iterate only over the

		// affected bounding box so alpha outside remains unchanged.

		err = PF_COPY(&params[ID_INPUT]->u.ld, output, nullptr, nullptr);

		if (err != PF_Err_NONE)

		{

			return err;

		}

		const float ex = (rc.radius + rc.edge_width) / std::max(rc.downsample_x, 1e-6f);

		const float ey = (rc.radius + rc.edge_width) / std::max(rc.downsample_y, 1e-6f);

		const int x0 = std::max(0, static_cast<int>(std::floor(rc.anchor_x - ex)));

		const int x1 = std::min(rc.width, static_cast<int>(std::ceil(rc.anchor_x + ex)) + 1);

		const int y0 = std::max(0, static_cast<int>(std::floor(rc.anchor_y - ey)));

		const int y1 = std::min(rc.height, static_cast<int>(std::ceil(rc.anchor_y + ey)) + 1);

		area.left = x0;

		area.right = x1;

		area.top = y0;

		area.bottom = y1;

	}

	PF_EffectWorld *src = &params[0]->u.ld;

	err = suites.IterateFloatSuite1()->iterate(

		in_data,

		0,

		output->height,

		src,

		&area,

		&rc,

		IteratePix32,

		output);

	return err;

}

static PF_Err

About(

	PF_InData *in_data,

	PF_OutData *out_data,

	PF_ParamDef *params[],

	PF_LayerDef *output)

{

	AEGP_SuiteHandler suites(in_data->pica_basicP);

	suites.ANSICallbacksSuite1()->sprintf(

		out_data->return_msg,

		"%s v%d.%d\r%s",

		GetStringPtr(StrID_Name),

		MAJOR_VERSION,

		MINOR_VERSION,

		GetStringPtr(StrID_Description));

	return PF_Err_NONE;

}

static PF_Err

GlobalSetup(

	PF_InData *in_data,

	PF_OutData *out_data,

	PF_ParamDef *params[],

	PF_LayerDef *output)

{

	(void)params;

	(void)output;

	PF_Err err = PF_Err_NONE;

	out_data->my_version = PF_VERSION(

		MAJOR_VERSION,

		MINOR_VERSION,

		BUG_VERSION,

		STAGE_VERSION,

		BUILD_VERSION);

	// Deep Color aware: 16-bit support

	// PF_OutFlag_DEEP_COLOR_AWARE = 0x02000000

	out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;

	// 32-bit float support and Multi-Frame Rendering support flags

	// PF_OutFlag2_FLOAT_COLOR_AWARE = 0x00000001 (32-bit float support)

	// PF_OutFlag2_SUPPORTS_THREADED_RENDERING = 0x08000000 (MFR support)

	// Setting both enables 32-bit float projects without warnings

	out_data->out_flags2 = 0x08000001; // PF_OutFlag2_SUPPORTS_THREADED_RENDERING | PF_OutFlag2_FLOAT_COLOR_AWARE

	return err;

}

static PF_Err

GlobalSetdown(

	PF_InData *in_data,

	PF_OutData *out_data,

	PF_ParamDef *params[],

	PF_LayerDef *output)

{

	(void)params;

	(void)output;

	(void)in_data;

	(void)out_data;

	return PF_Err_NONE;

}

static PF_Err ParamsSetup(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)

{

	PF_Err err = PF_Err_NONE;

	PF_ParamDef def;

	AEFX_CLR_STRUCT(def);

	PF_ADD_POINT("Anchor Point",

				 50, 50,

				 false,

				 ID_ANCHOR_POINT);

	PF_ADD_POPUP("Mode",

				 2,				// Number of options

				 1,				// Default selection (1: Line, 2: Circle)

				 "Line|Circle", // Options

				 ID_MODE);

	// Anti-aliasing is always ON, removed from UI

	PF_ADD_ANGLE("Angle", 0, ID_ANGLE);

	PF_ADD_FLOAT_SLIDERX(

		"Radius",

		0,

		3000,

		0,

		500,

		100,

		PF_Precision_INTEGER,

		0,

		0,

		ID_RADIUS);

	PF_ADD_COLOR("Color", 255, 0, 0, ID_COLOR);

	out_data->num_params = SKELETON_NUM_PARAMS;

	return PF_Err_NONE;

}

// Main render function with bit-depth detection
// Always uses PF_Iterate suites for MFR-safe threading

static PF_Err Render(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)

{

	PF_Err err = PF_Err_NONE;

	(void)out_data;

	// Determine bit depth using SDK-standard approach
	// Priority: 32-bit float -> 16-bit -> 8-bit

	bool is_32bit_float = false;

	// Check world_flags for 32-bit float (PF_OutFlag2_FLOAT_COLOR_AWARE)
	// PF_WORLD_IS_DEEP checks for 16-bit, so if it's not deep but world_flags indicate float
	if (!PF_WORLD_IS_DEEP(output))

	{
		// Check world_flags for 32-bit float format
		// output->world_flags & PF_WorldFlag_FLOAT indicates 32-bit float
		is_32bit_float = (output->world_flags & PF_WorldFlag_FLOAT) != 0;
	}

	if (is_32bit_float)

	{
		// 32-bit float rendering - always use PF_Iterate suite (MFR-safe)
		err = Render32Iterate(in_data, out_data, params, output, nullptr, nullptr);
	}

	// 16-bit detection
	else if (PF_WORLD_IS_DEEP(output))

	{
		// 16-bit rendering - always use PF_Iterate suite (MFR-safe)
		err = Render16Iterate(in_data, out_data, params, output, nullptr, nullptr);
	}

	// 8-bit rendering (default)
	else

	{
		// 8-bit rendering - always use PF_Iterate suite (MFR-safe)
		err = Render8Iterate(in_data, out_data, params, output, nullptr, nullptr);
	}

	return err;

}

extern "C" DllExport

	PF_Err

	PluginDataEntryFunction2(

		PF_PluginDataPtr inPtr,

		PF_PluginDataCB2 inPluginDataCallBackPtr,

		SPBasicSuite *inSPBasicSuitePtr,

		const char *inHostName,

		const char *inHostVersion)

{

	PF_Err result = PF_Err_INVALID_CALLBACK;

	result = PF_REGISTER_EFFECT_EXT2(

		inPtr,

		inPluginDataCallBackPtr,

		"sep_color",				  // Name

		"361do sep_color",			  // Match Name

		"361do_plugins",			  // Category

		AE_RESERVED_INFO,			  // Reserved Info

		"EffectMain",				  // Entry point

		"https://x.com/361do_sleep"); // support URL

	return result;

}

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

			void *extra)

	{

		PF_Err err = PF_Err_NONE;

		try

		{

			switch (cmd)

			{

			case PF_Cmd_ABOUT:

				err = About(in_data, out_data, params, output);

				break;

			case PF_Cmd_GLOBAL_SETUP:

				err = GlobalSetup(in_data, out_data, params, output);

				break;

			case PF_Cmd_GLOBAL_SETDOWN:

				err = GlobalSetdown(in_data, out_data, params, output);

				break;

			case PF_Cmd_PARAMS_SETUP:

				err = ParamsSetup(in_data, out_data, params, output);

				break;

			case PF_Cmd_RENDER:

				err = Render(in_data, out_data, params, output);

				break;

			}

		}

		catch (PF_Err &thrown_err)

		{

			err = thrown_err;

		}

		return err;

	}

} // extern "C"
