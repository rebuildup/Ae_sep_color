#include "sep_color.h"
#include "sep_color_Strings.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

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
	out_data->my_version = PF_VERSION(
		MAJOR_VERSION,
		MINOR_VERSION,
		BUG_VERSION,
		STAGE_VERSION,
		BUILD_VERSION);

	out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;

	out_data->out_flags2 |= PF_OutFlag2_SUPPORTS_THREADED_RENDERING;

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
				 1,				// Default selection (1: Line,2: Circle)
				 "Line|Circle", // Options
				 ID_MODE);

	PF_ADD_POPUP("Anti-Alias",
				 2,
				 2,
				 "Off|On",
				 ID_AA);

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

static PF_Err Render(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
	PF_Err err = PF_Err_NONE;

	PF_EffectWorld *input = &params[0]->u.ld;
	PF_Pixel *input_pixels = (PF_Pixel *)input->data;
	PF_Pixel *output_pixels = (PF_Pixel *)output->data;

	int width = output->width;
	int height = output->height;

	float downsample_x = static_cast<float>(in_data->downsample_x.den) / static_cast<float>(in_data->downsample_x.num);
	float downsample_y = static_cast<float>(in_data->downsample_y.den) / static_cast<float>(in_data->downsample_y.num);

	int anchor_x = (params[ID_ANCHOR_POINT]->u.td.x_value >> 16);
	int anchor_y = (params[ID_ANCHOR_POINT]->u.td.y_value >> 16);

	float angle_param_value = static_cast<float>(params[ID_ANGLE]->u.ad.value >> 16);

	angle_param_value = fmodf(angle_param_value, 360.0f);

	const float pi_f = 3.14159265358979323846f;
	float angle = angle_param_value * (pi_f / 180.0f);
	float radius = static_cast<float>(params[ID_RADIUS]->u.fs_d.value);

	int mode = params[ID_MODE]->u.pd.value;
	int aaPopup = params[ID_AA]->u.pd.value;
	bool aaEnabled = (aaPopup == 2);

	PF_Pixel color = params[ID_COLOR]->u.cd.value;

	const float subOffsets[4][2] = {
		{-0.25f, -0.25f},
		{0.25f, -0.25f},
		{-0.25f, 0.25f},
		{0.25f, 0.25f}};

	if (mode == 1)
	{
		float cs = cosf(angle);
		float sn = sinf(angle);
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				int input_index = y * (input->rowbytes / sizeof(PF_Pixel)) + x;
				int output_index = y * (output->rowbytes / sizeof(PF_Pixel)) + x;
				PF_Pixel input_pixel = input_pixels[input_index];
				PF_Pixel *output_pixel = &output_pixels[output_index];
				if (input_pixel.alpha == 0)
				{
					*output_pixel = input_pixel;
					continue;
				}
				float rx = (x - anchor_x) * downsample_x;
				float ry = (y - anchor_y) * downsample_y;
				if (!aaEnabled)
				{
					float rotated_x = rx * cs + ry * sn;
					if (rotated_x > 0.0f)
					{
						output_pixel->red = color.red;
						output_pixel->green = color.green;
						output_pixel->blue = color.blue;
						output_pixel->alpha = input_pixel.alpha;
					}
					else
					{
						*output_pixel = input_pixel;
					}
				}
				else
				{
					int hits = 0;
					for (int s = 0; s < 4; ++s)
					{
						float sx = rx + subOffsets[s][0];
						float sy = ry + subOffsets[s][1];
						float rxx = sx * cs + sy * sn;
						if (rxx > 0.0f)
							++hits;
					}
					float coverage = hits / 4.0f;
					if (coverage <= 0.0f)
					{
						*output_pixel = input_pixel;
					}
					else if (coverage >= 1.0f)
					{
						output_pixel->red = color.red;
						output_pixel->green = color.green;
						output_pixel->blue = color.blue;
						output_pixel->alpha = input_pixel.alpha;
					}
					else
					{
						output_pixel->red = static_cast<A_u_char>(input_pixel.red * (1.0f - coverage) + color.red * coverage + 0.5f);
						output_pixel->green = static_cast<A_u_char>(input_pixel.green * (1.0f - coverage) + color.green * coverage + 0.5f);
						output_pixel->blue = static_cast<A_u_char>(input_pixel.blue * (1.0f - coverage) + color.blue * coverage + 0.5f);
						output_pixel->alpha = input_pixel.alpha;
					}
				}
			}
		}
	}
	else
	{
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				int input_index = y * (input->rowbytes / sizeof(PF_Pixel)) + x;
				int output_index = y * (output->rowbytes / sizeof(PF_Pixel)) + x;
				PF_Pixel input_pixel = input_pixels[input_index];
				PF_Pixel *output_pixel = &output_pixels[output_index];
				if (input_pixel.alpha == 0)
				{
					*output_pixel = input_pixel;
					continue;
				}
				float rx = (x - anchor_x) * downsample_x;
				float ry = (y - anchor_y) * downsample_y;
				if (!aaEnabled)
				{
					float dist2 = rx * rx + ry * ry;
					if (dist2 <= radius * radius)
					{
						output_pixel->red = color.red;
						output_pixel->green = color.green;
						output_pixel->blue = color.blue;
						output_pixel->alpha = input_pixel.alpha;
					}
					else
					{
						*output_pixel = input_pixel;
					}
				}
				else
				{
					int hits = 0;
					float r2 = radius * radius;
					for (int s = 0; s < 4; ++s)
					{
						float sx = rx + subOffsets[s][0];
						float sy = ry + subOffsets[s][1];
						float d2 = sx * sx + sy * sy;
						if (d2 <= r2)
							++hits;
					}
					float coverage = hits / 4.0f;
					if (coverage <= 0.0f)
					{
						*output_pixel = input_pixel;
					}
					else if (coverage >= 1.0f)
					{
						output_pixel->red = color.red;
						output_pixel->green = color.green;
						output_pixel->blue = color.blue;
						output_pixel->alpha = input_pixel.alpha;
					}
					else
					{
						output_pixel->red = static_cast<A_u_char>(input_pixel.red * (1.0f - coverage) + color.red * coverage + 0.5f);
						output_pixel->green = static_cast<A_u_char>(input_pixel.green * (1.0f - coverage) + color.green * coverage + 0.5f);
						output_pixel->blue = static_cast<A_u_char>(input_pixel.blue * (1.0f - coverage) + color.blue * coverage + 0.5f);
						output_pixel->alpha = input_pixel.alpha;
					}
				}
			}
		}
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
