#include "sep_color_halide.h"
#include "sep_color.h"
#include "halide_loader.h"

#include <cmath>

#if SEP_COLOR_ENABLE_HALIDE
#include "third_party/halide/include/Halide.h"
using namespace Halide;
#endif

// 8-bit implementation (CPU schedule). Returns false if Halide not enabled.
bool SepColorHalide_Render8(PF_InData *in_data,
                            PF_OutData *out_data,
                            PF_ParamDef *params[],
                            PF_LayerDef *output,
                            PF_Pixel *input_pixels,
                            PF_Pixel *output_pixels)
{
#if SEP_COLOR_ENABLE_HALIDE
	if (!EnsureHalideRuntimeLoaded())
	{
		return false;
	}

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

    Var x("x"), y("y"), c("c");
    Func coverage("coverage"), out("out");

    Expr fx = (cast<float>(x) - ax) * dsx;
    Expr fy = (cast<float>(y) - ay) * dsy;
    Expr edge_width = 0.707f;
    Expr inv_edge = 1.0f / edge_width;

    Expr cov;
    if (mode == 1)
    {
        Expr cs = cos(ang);
        Expr sn = sin(ang);
        Expr rot_x = fx * cs + fy * sn;
        Expr sd = rot_x * inv_edge;
        cov = clamp((sd + 1.0f) * 0.5f, 0.0f, 1.0f);
    }
    else
    {
        Expr dist = hypot(fx, fy);
        Expr sd = (radius - dist) * inv_edge;
        cov = clamp((sd + 1.0f) * 0.5f, 0.0f, 1.0f);
    }
    coverage(x, y) = cov;

    // Load input as interleaved buffer (RGBA)
    Halide::Buffer<uint8_t> in_buf((uint8_t *)input_pixels, width, height, 4);
    halide_buffer_t *in_raw = in_buf.raw_buffer();
    in_raw->dim[0].stride = 4;            // x dimension (next pixel)
    in_raw->dim[1].stride = in_stride_px; // y dimension (row stride in pixels)
    in_raw->dim[2].stride = 1;            // channel interleaved

    Halide::Buffer<uint8_t> out_buf((uint8_t *)output_pixels, width, height, 4);
    halide_buffer_t *out_raw = out_buf.raw_buffer();
    out_raw->dim[0].stride = 4;
    out_raw->dim[1].stride = out_stride_px;
    out_raw->dim[2].stride = 1;

    // Blend per channel; preserve alpha
    Expr a = cast<float>(in_buf(x, y, 3)) / 255.0f;
    Expr cov_a = coverage(x, y) * a; // coverage * input alpha (premultiplied)
    Expr r_dst = cast<float>(col.red);
    Expr g_dst = cast<float>(col.green);
    Expr b_dst = cast<float>(col.blue);

    Expr r_src = cast<float>(in_buf(x, y, 0));
    Expr g_src = cast<float>(in_buf(x, y, 1));
    Expr b_src = cast<float>(in_buf(x, y, 2));

    out(x, y, 0) = cast<uint8_t>(r_src + (r_dst - r_src) * cov_a + 0.5f);
    out(x, y, 1) = cast<uint8_t>(g_src + (g_dst - g_src) * cov_a + 0.5f);
    out(x, y, 2) = cast<uint8_t>(b_src + (b_dst - b_src) * cov_a + 0.5f);
    out(x, y, 3) = cast<uint8_t>(in_buf(x, y, 3));

    // Schedule
#ifdef SEP_COLOR_HALIDE_GPU
    Target t = get_host_target();
#if defined(_WIN32)
    t.set_feature(Target::D3D12Compute);
#else
    t.set_feature(Target::Metal);
#endif
    out.bound(c, 0, 4).reorder(c, x, y);
    Var xi("xi"), yi("yi");
    out.gpu_tile(x, y, xi, yi, 16, 16);
    coverage.compute_at(out, x).gpu_threads(x);
    out.compile_jit(t);
    out.realize(out_buf, t);
#else
    // CPU schedule: vectorize x, parallelize y
    out.bound(c, 0, 4).reorder(c, x, y);
    out.vectorize(x, 16).parallel(y, 8);
    coverage.compute_at(out, y).vectorize(x, 16);
    out.realize(out_buf);
#endif
    return true;
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
                             PF_Pixel16 *input_pixels,
                             PF_Pixel16 *output_pixels)
{
#if SEP_COLOR_ENABLE_HALIDE
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
                             PF_PixelFloat *input_pixels,
                             PF_PixelFloat *output_pixels)
{
#if SEP_COLOR_ENABLE_HALIDE
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
