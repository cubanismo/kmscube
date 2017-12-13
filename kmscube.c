/*
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
 * Copyright (c) 2012 Rob Clark <rob@ti.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Based on a egl cube test app originally written by Arvin Schnell */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "common.h"
#include "surface-manager.h"
#include "drm-common.h"

#ifdef HAVE_GST
#include <gst/gst.h>
GST_DEBUG_CATEGORY(kmscube_debug);
#endif

static const struct egl *egl;
static const struct surfmgr *surfmgr;
static const struct drm *drm;

static const char *shortopts = "AD:S:B:M:m:V:";

static const struct option longopts[] = {
	{"atomic",     no_argument,       0, 'A'},
	{"device",     required_argument, 0, 'D'},
	{"surfmgrdev", required_argument, 0, 'S'},
	{"backend",    required_argument, 0, 'B'},
	{"mode",       required_argument, 0, 'M'},
	{"modifier",   required_argument, 0, 'm'},
	{"video",      required_argument, 0, 'V'},
	{0, 0, 0, 0}
};

static void usage(const char *name)
{
	printf("Usage: %s [-ADMmV]\n"
			"\n"
			"options:\n"
			"    -A, --atomic             use atomic modesetting and fencing\n"
			"    -D, --device=DEVICE      use the given device\n"
			"    -S, --surfmgrdev=DEVICE  use the given device for surface mgr\n"
			"    -B, --backend=BACKEND    specify backend, one of:\n"
			"        gbm       - Create buffers using GBM (default)\n"
#ifdef HAVE_ALLOCATOR
			"        allocator - Create buffers using the Generic allocator\n"
#endif
			"    -M, --mode=MODE          specify mode, one of:\n"
			"        smooth    -  smooth shaded cube (default)\n"
			"        rgba      -  rgba textured cube\n"
			"        nv12-2img -  yuv textured (color conversion in shader)\n"
			"        nv12-1img -  yuv textured (single nv12 texture)\n"
			"    -m, --modifier=MODIFIER  hardcode the selected modifier\n"
			"    -V, --video=FILE         video textured cube\n",
			name);
}

int main(int argc, char *argv[])
{
	const char *device = "/dev/dri/card0";
	const char *surfmgrdev = NULL;
	enum backend backend = GBM;
	const char *video = NULL;
	enum mode mode = SMOOTH;
	uint64_t modifier = DRM_FORMAT_MOD_INVALID;
	int surfmgrfd;
	int atomic = 0;
	int opt;

#ifdef HAVE_GST
	gst_init(&argc, &argv);
	GST_DEBUG_CATEGORY_INIT(kmscube_debug, "kmscube", 0, "kmscube video pipeline");
#endif

	while ((opt = getopt_long_only(argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (opt) {
		case 'A':
			atomic = 1;
			break;
		case 'D':
			device = optarg;
			break;
		case 'S':
			surfmgrdev = optarg;
			break;
		case 'B':
			if (strcmp(optarg, "gbm") == 0) {
				backend = GBM;
#ifdef HAVE_ALLOCATOR
			} else if (strcmp(optarg, "allocator") == 0) {
				backend = ALLOCATOR;
#endif
			} else {
				printf("invalid backend: %s\n", optarg);
				usage(argv[0]);
				return -1;
			}
			break;
		case 'M':
			if (strcmp(optarg, "smooth") == 0) {
				mode = SMOOTH;
			} else if (strcmp(optarg, "rgba") == 0) {
				mode = RGBA;
			} else if (strcmp(optarg, "nv12-2img") == 0) {
				mode = NV12_2IMG;
			} else if (strcmp(optarg, "nv12-1img") == 0) {
				mode = NV12_1IMG;
			} else {
				printf("invalid mode: %s\n", optarg);
				usage(argv[0]);
				return -1;
			}
			break;
		case 'm':
			modifier = strtoull(optarg, NULL, 0);
			break;
		case 'V':
			mode = VIDEO;
			video = optarg;
			break;
		default:
			usage(argv[0]);
			return -1;
		}
	}

	if (atomic)
		drm = init_drm_atomic(device);
	else
		drm = init_drm_legacy(device);
	if (!drm) {
		printf("failed to initialize %s DRM\n", atomic ? "atomic" : "legacy");
		return -1;
	}

	if (!surfmgrdev) {
		surfmgrfd = drm->fd;
	} else {
		surfmgrfd = open(surfmgrdev, O_RDWR);

		if (surfmgrfd < 0) {
			printf("could not open surface manager device\n");
			return -1;
		}
	}

	surfmgr = init_surfmgr(surfmgrfd, drm->fd, backend,
						   drm->mode->hdisplay, drm->mode->vdisplay,
						   modifier);
	if (!surfmgr) {
		printf("failed to initialize any surface manager APIs\n");
		return -1;
	}

	if (!atomic && !surfmgr->gbm) {
		printf("Legacy DRM requires GBM\n");
		return -1;
	}

	if (mode == SMOOTH)
		egl = init_cube_smooth(surfmgr);
	else if (mode == VIDEO)
		egl = init_cube_video(surfmgr, video);
	else
		egl = init_cube_tex(surfmgr, mode);

	if (!egl) {
		printf("failed to initialize EGL\n");
		return -1;
	}

	/* clear the color buffer */
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	return drm->run(surfmgr, egl);
}
