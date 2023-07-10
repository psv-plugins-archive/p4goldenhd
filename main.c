/*
This file is part of Persona 4 Golden HD Patch
Copyright 2020 浅倉麗子

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <psp2/display.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <fnblit.h>
#include <taihen.h>

int vsnprintf(char *buf, SceSize n, const char *fmt, va_list arg) {
	return sceClibVsnprintf(buf, n, fmt, arg);
}

extern char font_sfn[];
extern int font_sfn_len;

#define P4G_MOD_NAME "XRD654_libgmq_MASTER_INSTA"
#define P4G_JP_NID 0x8C503A79

#ifdef FB_FHD
	#define FB_PITCH 1920
	#define FB_WIDTH 1920
	#define FB_HEIGHT 1080
#else
	#define FB_PITCH 1280
	#define FB_WIDTH 1280
	#define FB_HEIGHT 720
#endif

#define GLZ(x) do {\
	if ((x) < 0) { goto fail; }\
} while (0)

__attribute__ ((__format__ (__printf__, 1, 2)))
static void LOG(const char *fmt, ...) {
	(void)fmt;

	#ifdef LOG_PRINTF
	sceClibPrintf("\033[0;36m[P4GoldenHD]\033[0m ");
	va_list args;
	va_start(args, fmt);
	sceClibVprintf(fmt, args);
	va_end(args);
	#endif
}

static struct {
	int n_back_buffers;
	int unk_4;
	float width;
	float height;
	int color_format;
	int pitch;
} *p4g_info;

#define N_INJECT 8
static SceUID inject_id[N_INJECT];

#define N_HOOK 5
static SceUID hook_id[N_HOOK];
static tai_hook_ref_t hook_ref[N_HOOK];

static SceUID INJECT_DATA(int idx, int mod, int seg, int ofs, void *data, int size) {
	inject_id[idx] = taiInjectData(mod, seg, ofs, data, size);
	LOG("Injected %d UID %08X\n", idx, inject_id[idx]);
	return inject_id[idx];
}

static SceUID hook_import(int idx, char *mod, int libnid, int funcnid, void *func) {
	hook_id[idx] = taiHookFunctionImport(hook_ref+idx, mod, libnid, funcnid, func);
	LOG("Hooked %d UID %08X\n", idx, hook_id[idx]);
	return hook_id[idx];
}
#define HOOK_IMPORT(idx, mod, libnid, funcnid, func)\
	hook_import(idx, mod, libnid, funcnid, func##_hook)

static int UNINJECT(int idx) {
	int ret = 0;
	if (inject_id[idx] >= 0) {
		ret = taiInjectRelease(inject_id[idx]);
		LOG("Uninjected %d UID %08X\n", idx, inject_id[idx]);
		inject_id[idx] = -1;
	}
	return ret;
}

static int UNHOOK(int idx) {
	int ret = 0;
	if (hook_id[idx] >= 0) {
		ret = taiHookRelease(hook_id[idx], hook_ref[idx]);
		LOG("Unhooked %d UID %08X\n", idx, hook_id[idx]);
		hook_id[idx] = -1;
		hook_ref[idx] = -1;
	}
	return ret;
}

static int sceDisplaySetFrameBuf_hook(SceDisplayFrameBuf *fb, int mode) {
	static uint32_t start_time = 0;
	static bool failed = false;

	if (!start_time) { start_time = sceKernelGetProcessTimeLow(); }

	if (fb && fb->base) {
		fb->pitch = FB_PITCH;
		fb->width = FB_WIDTH;
		fb->height = FB_HEIGHT;

		fnblit_set_fb(fb->base, fb->pitch, fb->width, fb->height);
		if (failed) {
			fb->width = 960;
			fb->height = 544;
			fnblit_printf(0, 0, "Persona 4 Golden HD Patch failed: %dx%d", FB_WIDTH, FB_HEIGHT);
			fnblit_printf(0, 28, "Install Sharpscale and turn on 'Enable Full HD'");
		} else if (sceKernelGetProcessTimeLow() - start_time < 15 * 1000 * 1000) {
			fnblit_printf(0, 0, "Persona 4 Golden HD Patch success: %dx%d", FB_WIDTH, FB_HEIGHT);
		}
	}

	int ret = TAI_NEXT(sceDisplaySetFrameBuf_hook, hook_ref[0], fb, mode);

	if (!failed && fb && fb->base && fb->pitch == FB_PITCH
			&& fb->width == FB_WIDTH && fb->height == FB_HEIGHT) {
		failed = ret < 0;
	}

	return ret;
}

static int sceGxmSetUniformDataF_hook(
		void *uniformBuffer,
		void *parameter,
		int componentOffset,
		int componentCount,
		float *sourceData) {

	float _sourceData[2] = {960.0, 960.0};

	if (componentCount == 2) {
		if (sourceData[0] == 1024.0 && sourceData[1] == 1024.0) {
			sourceData = _sourceData;
		} else if (sourceData[0] == (float)FB_PITCH && sourceData[1] == (float)FB_PITCH) {
			sourceData = _sourceData;
		} else if (sourceData[0] == 960.0 && sourceData[1] == 544.0) {
			_sourceData[1] = 540.0;
			sourceData = _sourceData;
		}
	}

	return TAI_NEXT(sceGxmSetUniformDataF_hook, hook_ref[1],
		uniformBuffer, parameter, componentOffset, componentCount, sourceData);
}

#ifdef FB_FHD

static SceUID sceKernelAllocMemBlock_hook(char *name, int type, int size, void *opt) {
	// Non-JP versions allocate memory differently when decoding pre-rendered videos.
	// The amount of memory required may exceed the remaining free CDRAM in 1920x1080
	// mode. Normally a block of 13 MiB will be allocated, but with the undub patch,
	// a block of 14 MiB will be allocated. Move blocks of size at least 8 MiB.
	if (size >= 0x800000 && 0 == sceClibStrcmp(name, "movie")) {
		type = SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW;
		opt = NULL;
		LOG("moved %d KB from CDRAM to PHYCONT\n", size / 1024);
	}
	LOG("allocate %08X %08X (%d KB) %s\n", type, size, size / 1024, name);
	return TAI_NEXT(sceKernelAllocMemBlock_hook, hook_ref[2], name, type, size, opt);
}

// When the number of backbuffers is equal to 2, the JP version has tearing
// in the top left corner in pre-rendered videos. This problem does not
// happen when the number of backbuffers is 1 or 3, nor in non-JP versions.

static int sceAvcdecCreateDecoder_hook(int type, void *ctrl, void *info) {
	LOG("called sceAvcdecCreateDecoder\n");
	p4g_info->n_back_buffers = 1;
	return TAI_NEXT(sceAvcdecCreateDecoder_hook, hook_ref[3], type, ctrl, info);
}

static int sceAvcdecDeleteDecoder_hook(void *ctrl) {
	LOG("called sceAvcdecDeleteDecoder\n");
	p4g_info->n_back_buffers = 2;
	return TAI_NEXT(sceAvcdecDeleteDecoder_hook, hook_ref[4], ctrl);
}

#endif // ifdef FB_FHD

static void startup(void) {
	sceClibMemset(inject_id, 0xFF, sizeof(inject_id));
	sceClibMemset(hook_id, 0xFF, sizeof(hook_id));
	sceClibMemset(hook_ref, 0xFF, sizeof(hook_ref));
}

static void cleanup(void) {
	for (int i = 0; i < N_INJECT; i++) { UNINJECT(i); }
	for (int i = 0; i < N_HOOK; i++) { UNHOOK(i); }
}

int _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize argc, const void *argv) { (void)argc; (void)argv;
	startup();

	tai_module_info_t minfo;
	minfo.size = sizeof(minfo);
	GLZ(taiGetModuleInfo(P4G_MOD_NAME, &minfo));

	int ofs_3dbuf, ofs_instr, ofs_info;

	LOG("Module NID %08X\n", (int)minfo.module_nid);

	switch (minfo.module_nid) {
		case P4G_JP_NID: // PCSG00004, PCSG00563
			ofs_3dbuf = 0xDBD9C;
			ofs_instr = 0x2B3554;
			ofs_info = 0x2E8D28;
			break;
		case 0xB8EBED65: // PCSE00120
			ofs_3dbuf = 0xDBCEC;
			ofs_instr = 0x2B23D4;
			ofs_info = 0x2EA1F8;
			break;
		case 0x4BB9AE7C: // PCSB00245
			ofs_3dbuf = 0xDBCFC;
			ofs_instr = 0x2B23F4;
			ofs_info = 0x2EA1F8;
			break;
		case 0x96BBD787: // PCSH00021
			ofs_3dbuf = 0xF1C50;
			ofs_instr = 0x2B8B68;
			ofs_info = 0x3059D8;
			break;
		default:
			goto fail;
	}

	SceUID p4g_modid = minfo.modid;

	SceKernelModuleInfo sce_minfo;
	sce_minfo.size = sizeof(sce_minfo);
	GLZ(sceKernelGetModuleInfo(p4g_modid, &sce_minfo));

	p4g_info = sce_minfo.segments[1].vaddr + ofs_info;

	// 3D offscreen buffer

	GLZ(INJECT_DATA(0, p4g_modid, 1, ofs_3dbuf + 0, (float[]){(float)FB_WIDTH}, 4));
	GLZ(INJECT_DATA(1, p4g_modid, 1, ofs_3dbuf + 4, (float[]){(float)FB_HEIGHT}, 4));
	GLZ(INJECT_DATA(2, p4g_modid, 1, ofs_3dbuf + 8, (float[]){(float)FB_PITCH}, 4));

	// main/UI buffer

	uint16_t nop_nop[2] = {0xBF00, 0xBF00};

	// disable set default pitch, width, height
	GLZ(INJECT_DATA(3, p4g_modid, 0, ofs_instr + 0x00, nop_nop, 4));
	GLZ(INJECT_DATA(4, p4g_modid, 0, ofs_instr + 0x06, nop_nop, 4));
	GLZ(INJECT_DATA(5, p4g_modid, 0, ofs_instr + 0x14, nop_nop, 4));

	// set custom values
	p4g_info->width = (float)FB_WIDTH;
	p4g_info->height = (float)FB_HEIGHT;
	p4g_info->pitch = FB_PITCH;

	// setup fnblit
	fnblit_set_font(font_sfn);
	fnblit_set_fg(0xFFFFFFFF);
	fnblit_set_bg(0x00000000);

	// hooks
	GLZ(HOOK_IMPORT(0, P4G_MOD_NAME, 0x4FAACD11, 0x7A410B64, sceDisplaySetFrameBuf));
	GLZ(HOOK_IMPORT(1, P4G_MOD_NAME, 0xF76B66BD, 0x65DD0C84, sceGxmSetUniformDataF));

#ifdef FB_FHD
	// disable set default number of backbuffers
	GLZ(INJECT_DATA(6, p4g_modid, 0, ofs_instr + 0x0C, nop_nop, 4));
	p4g_info->n_back_buffers = 2;

	// do not create unused buffer and render target
	GLZ(INJECT_DATA(7, p4g_modid, 0, ofs_instr + 0xCE, nop_nop, 4));

	// hooks
	if (minfo.module_nid == P4G_JP_NID) {
		GLZ(HOOK_IMPORT(3, P4G_MOD_NAME, 0x163C3727, 0xE82BB69B, sceAvcdecCreateDecoder));
		GLZ(HOOK_IMPORT(4, P4G_MOD_NAME, 0x163C3727, 0x8A0E359E, sceAvcdecDeleteDecoder));
	} else {
		GLZ(HOOK_IMPORT(2, P4G_MOD_NAME, 0x37FE725A, 0xB9D5EBDE, sceKernelAllocMemBlock));
	}
#endif

	return SCE_KERNEL_START_SUCCESS;

fail:
	cleanup();
	return SCE_KERNEL_START_FAILED;
}

int module_stop(SceSize argc, const void *argv) { (void)argc; (void)argv;
	cleanup();
	return SCE_KERNEL_STOP_SUCCESS;
}
