/*
 * Copyright (c) 2017 Rob Clark <rclark@redhat.com>
 * Copyright © 2013 Intel Corporation
 * Copyright (c) 2017 NVIDIA Corporation
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

#include "common.h"
#include "drm-common.h"
#include "surface-manager.h"

static struct gbm gbm;
#ifdef HAVE_ALLOCATOR
static struct allocator allocator;
#endif
static struct surfmgr surfmgr;

#ifdef HAVE_GBM_MODIFIERS
static int
get_modifiers(uint64_t **mods)
{
	/* Assumed LINEAR is supported everywhere */
	static uint64_t modifiers[] = {DRM_FORMAT_MOD_LINEAR};
	*mods = modifiers;
	return 1;
}
#endif

static const struct gbm * init_gbm(int drm_fd, int w, int h, uint64_t modifier)
{
	gbm.dev = gbm_create_device(drm_fd);

#ifndef HAVE_GBM_MODIFIERS
	if (modifier != DRM_FORMAT_MOD_INVALID) {
		fprintf(stderr, "Modifiers requested but support isn't available\n");
		return NULL;
	}
	gbm.surface = gbm_surface_create(gbm.dev, w, h,
			GBM_FORMAT_XRGB8888,
			GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
#else
	uint64_t *mods;
	int count;
	if (modifier != DRM_FORMAT_MOD_INVALID) {
		count = 1;
		mods = &modifier;
	} else {
		count = get_modifiers(&mods);
	}
	gbm.surface = gbm_surface_create_with_modifiers(gbm.dev, w, h,
			GBM_FORMAT_XRGB8888, mods, count);
#endif

	if (!gbm.surface) {
		printf("failed to create gbm surface\n");
		return NULL;
	}

	return &gbm;
}

#ifdef HAVE_ALLOCATOR
static const struct allocator * init_allocator(int drm_fd, int w, int h)
{
	assertion_t assertion = {
		w,			/* width */
		h,			/* height */
		NULL,		/* format */
		NULL		/* ext */
	};

	static const usage_display_t usage_display = {
		{
			VENDOR_BASE,							/* Usage vendor ID */
			USAGE_BASE_DISPLAY,						/* Per-vendor usage ID */
			USAGE_LENGTH_IN_WORDS(usage_display_t)	/* length_in_words */
		},

		USAGE_BASE_DISPLAY_ROTATION_0				/* rotation_types */
	};

	static const usage_texture_t usage_texture = {
		{
			VENDOR_BASE,							/* Usage vendor ID */
			USAGE_BASE_TEXTURE,						/* Per-vendor usage ID */
			USAGE_LENGTH_IN_WORDS(usage_texture_t)	/* length_in_words */
		}
	};

	usage_t uses[] = {
		{
			NULL,							/* dev (filled in below) */
			&usage_display.header			/* usage */
		},
		{
			NULL,							/* dev (filled in below) */
			&usage_texture.header			/* usage */
		}
	};

	capability_set_t *capability_sets;
	uint32_t num_capability_sets;
	uint32_t i;
	uint32_t allocs = 0;

	allocator.dev = device_create(drm_fd);

	if (!allocator.dev) {
		goto fail;
	}

	for (i = 0; i < ARRAY_SIZE(uses); i++) {
		uses[i].dev = allocator.dev;
	}

	if (device_get_capabilities(allocator.dev,
								&assertion,
								ARRAY_SIZE(uses),
								uses,
								&num_capability_sets,
								&capability_sets) ||
		num_capability_sets < 1) {
		printf("Failed to query capability sets\n");
		goto fail;
	}

	for (; allocs < ARRAY_SIZE(allocator.allocations); allocs++) {
		if (device_create_allocation(allocator.dev,
									 &assertion,
									 &capability_sets[0],
									 &allocator.allocations[allocs])) {
			printf("Failed to create an allocation\n");
			goto fail;
		}
	}

	return &allocator;

fail:
	for (i = 0; i < allocs; i++) {
		device_destroy_allocation(allocator.dev,
								  allocator.allocations[i]);
	}

	device_destroy(allocator.dev);
	allocator.dev = NULL;

	return NULL;
}
#endif /* HAVE_ALLOCATOR */

const struct surfmgr * init_surfmgr(int drm_fd, int w, int h, uint64_t modifier)
{
	surfmgr.width = w;
	surfmgr.height = h;

#ifdef HAVE_ALLOCATOR
	surfmgr.allocator = init_allocator(drm_fd, w, h);

	if (surfmgr.allocator)
		return &surfmgr;
#endif

	surfmgr.gbm = init_gbm(drm_fd, w, h, modifier);

	if (surfmgr.gbm)
		return &surfmgr;

	/* Initialization failed. */
	surfmgr.width = 0;
	surfmgr.height = 0;

	return NULL;
}

struct drm_fb *surfmgr_get_next_fb(const struct surfmgr *surfmgr)
{
	struct drm_fb *fb = NULL;

	if (surfmgr->gbm) {
		struct gbm_bo *bo;

		bo = gbm_surface_lock_front_buffer(surfmgr->gbm->surface);

		if (!bo) {
			printf("Failed to lock frontbuffer\n");
			return NULL;
		}

		fb = drm_fb_get_from_bo(bo);
	}

	return fb;
}

void surfmgr_release_fb(const struct surfmgr *surfmgr, struct drm_fb *fb)
{
	if (surfmgr->gbm) {
		gbm_surface_release_buffer(surfmgr->gbm->surface, fb->bo);
	}
}
