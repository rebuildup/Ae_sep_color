#include "sep_color_halide.h"
#include "sep_color.h"
#include "halide_loader.h"

#include <cmath>
#include <exception>

#if SEP_COLOR_ENABLE_HALIDE
#include "third_party/halide/include/Halide.h"
#include "third_party/halide/include/HalideRuntime.h"
#endif

bool SepColorHalide_GlobalInit(PF_InData *in_data, SepColorHalideGlobalState &state)
{
	(void)in_data;
#if SEP_COLOR_ENABLE_HALIDE
	state = {};
	if (!EnsureHalideRuntimeLoaded())
	{
		return false;
	}

	state.runtime_loaded = true;

	Halide::Target base_target = Halide::get_host_target();
	state.target                 = base_target;

#ifdef SEP_COLOR_HALIDE_GPU
	try
	{
		Halide::Target gpu_target = base_target;
#if defined(_WIN32)
		gpu_target.set_feature(Halide::Target::D3D12Compute);
#elif defined(__APPLE__)
		gpu_target.set_feature(Halide::Target::Metal);
#endif

		// Run a tiny probe pipeline to ensure the GPU runtime is usable.
		Halide::Var x("probe_x");
		Halide::Func probe("sep_color_halide_probe");
		probe(x) = Halide::cast<uint8_t>(x);

		Halide::Buffer<uint8_t> tmp(1);
		probe.compile_jit(gpu_target);
		probe.realize(tmp, gpu_target);

		state.target      = gpu_target;
		state.gpu_enabled = true;
	}
	catch (...)
	{
		// Fallback to CPU target if GPU init fails.
		state.target      = base_target;
		state.gpu_enabled = false;
	}
#endif
	return true;
#else
	(void)state;
	return false;
#endif
}

void SepColorHalide_GlobalRelease(SepColorHalideGlobalState &state)
{
#if SEP_COLOR_ENABLE_HALIDE
	if (state.runtime_loaded)
	{
		Halide::JITSharedRuntime::release_all();
	}
	state = {};
#else
	(void)state;
#endif
}

// 8-bit implementation (CPU schedule). Returns false if Halide not enabled.
bool SepColorHalide_Render8(PF_InData *in_data,
                            PF_OutData *out_data,
                            PF_ParamDef *params[],
                            PF_LayerDef *output,
                            const SepColorHalideGlobalState &state,
                            PF_Pixel *input_pixels,
                            PF_Pixel *output_pixels)
{
#if SEP_COLOR_ENABLE_HALIDE
	if (!state.runtime_loaded)
	{
		return false;
	}

	try
	{
		const int width = output->width;
		const int height = output->height;
		if (width <= 0 || height <= 0)
			return true;

		const float dsx = static_cast<float>(in_data->downsample_x.den) / static_cast<float>(in_data->downsample_x.num);
		const float dsy = static_cast<float>(in_data->downsample_y.den) / static_cast<float>(in_data->downsample_y.num);
		const int ax = (params[ID_ANCHOR_POINT]->u.td.x_value >> 16);
		const int ay = (params[ID_ANCHOR_POINT]->u.td.y_value >> 16);
		float angle_param_value = static_cast<float>(params[ID_ANGLE]->u.ad.value >> 16);
		angle_param_value = std::fmod(angle_param_value, 360.0f);
		const float pi_f = 3.14159265358979323846f;
		const float ang = angle_param_value * (pi_f / 180.0f);
		const float radius = static_cast<float>(params[ID_RADIUS]->u.fs_d.value);
		const int mode = params[ID_MODE]->u.pd.value;
		PF_Pixel col = params[ID_COLOR]->u.cd.value;

		const int in_stride_px = output->rowbytes / sizeof(PF_Pixel);
		const int out_stride_px = output->rowbytes / sizeof(PF_Pixel);

		Halide::Var x("x"), y("y"), c("c");
		Halide::Func coverage("coverage"), out("out");

		Halide::Expr fx         = (Halide::cast<float>(x) - ax) * dsx;
		Halide::Expr fy         = (Halide::cast<float>(y) - ay) * dsy;
		Halide::Expr edge_width = 0.707f;
		Halide::Expr inv_edge   = 1.0f / edge_width;

		Halide::Expr cov;
		if (mode == 1)
		{
			Halide::Expr cs    = Halide::cos(ang);
			Halide::Expr sn    = Halide::sin(ang);
			Halide::Expr rot_x = fx * cs + fy * sn;
			Halide::Expr sd    = rot_x * inv_edge;
			cov                = Halide::clamp((sd + 1.0f) * 0.5f, 0.0f, 1.0f);
		}
		else
		{
			Halide::Expr dist = Halide::hypot(fx, fy);
			Halide::Expr sd   = (radius - dist) * inv_edge;
			cov               = Halide::clamp((sd + 1.0f) * 0.5f, 0.0f, 1.0f);
		}
		coverage(x, y) = cov;

		// Load input as interleaved buffer (RGBA)
		Halide::Buffer<uint8_t> in_buf((uint8_t *)input_pixels, width, height, 4);
		halide_buffer_t *in_raw = in_buf.raw_buffer();
		in_raw->dim[0].stride   = 4;		   // x dimension (next pixel)
		in_raw->dim[1].stride = in_stride_px; // y dimension (row stride in pixels)
		in_raw->dim[2].stride = 1;			// channel interleaved

		Halide::Buffer<uint8_t> out_buf((uint8_t *)output_pixels, width, height, 4);
		halide_buffer_t *out_raw = out_buf.raw_buffer();
		out_raw->dim[0].stride   = 4;
		out_raw->dim[1].stride   = out_stride_px;
		out_raw->dim[2].stride   = 1;

		// Blend per channel; preserve alpha
		Halide::Expr a     = Halide::cast<float>(in_buf(x, y, 3)) / 255.0f;
		Halide::Expr cov_a = coverage(x, y) * a; // coverage * input alpha (premultiplied)
		Halide::Expr r_dst = Halide::cast<float>(col.red);
		Halide::Expr g_dst = Halide::cast<float>(col.green);
		Halide::Expr b_dst = Halide::cast<float>(col.blue);

		Halide::Expr r_src = Halide::cast<float>(in_buf(x, y, 0));
		Halide::Expr g_src = Halide::cast<float>(in_buf(x, y, 1));
		Halide::Expr b_src = Halide::cast<float>(in_buf(x, y, 2));

		out(x, y, 0) = Halide::cast<uint8_t>(r_src + (r_dst - r_src) * cov_a + 0.5f);
		out(x, y, 1) = Halide::cast<uint8_t>(g_src + (g_dst - g_src) * cov_a + 0.5f);
		out(x, y, 2) = Halide::cast<uint8_t>(b_src + (b_dst - b_src) * cov_a + 0.5f);
		out(x, y, 3) = Halide::cast<uint8_t>(in_buf(x, y, 3));

		// Schedule
		const Halide::Target &target = state.target;
		if (state.gpu_enabled)
		{
			out.bound(c, 0, 4).reorder(c, x, y);
			Halide::Var xi("xi"), yi("yi");
			out.gpu_tile(x, y, xi, yi, 16, 16);
			coverage.compute_at(out, x).gpu_threads(x);
			out.compile_jit(target);
			out.realize(out_buf, target);
		}
		else
		{
		// CPU schedule: vectorize x, parallelize y
			out.bound(c, 0, 4).reorder(c, x, y);
			out.vectorize(x, 16).parallel(y, 8);
			coverage.compute_at(out, y).vectorize(x, 16);
			out.realize(out_buf, target);
		}
		return true;
	}
	catch (const Halide::Error &)
	{
		return false;
	}
	catch (const std::exception &)
	{
		return false;
	}
#else
	(void)in_data;
	(void)out_data;
	(void)params;
	(void)output;
	(void)input_pixels;
	(void)output_pixels;
	return false;
#endif
}

bool SepColorHalide_Render16(PF_InData *in_data,
                             PF_OutData *out_data,
                             PF_ParamDef *params[],
                             PF_LayerDef *output,
                             const SepColorHalideGlobalState &state,
                             PF_Pixel16 *input_pixels,
                             PF_Pixel16 *output_pixels)
{
#if SEP_COLOR_ENABLE_HALIDE
	(void)state;
    // TODO: 16-bit pipeline (u16 domain 0..32768) similar to 8-bit
    (void)in_data;
    (void)out_data;
    (void)params;
    (void)output;
    (void)input_pixels;
    (void)output_pixels;
    return false;
#else
    (void)in_data;
    (void)out_data;
    (void)params;
    (void)output;
    (void)input_pixels;
    (void)output_pixels;
    return false;
#endif
}

bool SepColorHalide_Render32(PF_InData *in_data,
                             PF_OutData *out_data,
                             PF_ParamDef *params[],
                             PF_LayerDef *output,
                             const SepColorHalideGlobalState &state,
                             PF_PixelFloat *input_pixels,
                             PF_PixelFloat *output_pixels)
{
#if SEP_COLOR_ENABLE_HALIDE
	(void)state;
    // TODO: float pipeline
    (void)in_data;
    (void)out_data;
    (void)params;
    (void)output;
    (void)input_pixels;
    (void)output_pixels;
    return false;
#else
    (void)in_data;
    (void)out_data;
    (void)params;
    (void)output;
    (void)input_pixels;
    (void)output_pixels;
    return false;
#endif
}
