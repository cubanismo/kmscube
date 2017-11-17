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

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

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
static const struct allocator * init_allocator(int dev_fd, int drm_fd, int w, int h)
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

	allocator.dev = device_create(dev_fd);

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

	memset(allocator.allocations, 0, sizeof(allocator.allocations));
	for (; allocs < ARRAY_SIZE(allocator.allocations); allocs++) {
		struct allocation *alloc = &allocator.allocations[allocs];
		struct drm_gem_close closeParams;
		uint64_t allocation_size;
		void *metadata;
		size_t metadata_size;
		int fd, res;
		uint32_t gemHandle;

		if (device_create_allocation(allocator.dev,
									 &assertion,
									 &capability_sets[0],
									 &alloc->alloc)) {
			printf("Failed to create an allocation\n");
			goto fail;
		}

		if (device_export_allocation(allocator.dev,
									 alloc->alloc,
									 &allocation_size,
									 &metadata_size,
									 &metadata,
									 &fd)) {
			printf("Failed to export an allocation\n");
			goto fail;
		}

		res = drmPrimeFDToHandle(drm_fd,
								 fd,
								 &gemHandle);

		close(fd);

		if (res) {
			free(metadata);
			goto fail;
		}

		alloc->fb = drm_fb_get_from_gem(drm_fd,
										gemHandle,
										w, h,
										metadata_size,
										metadata);

		memset(&closeParams, 0, sizeof(closeParams));
		closeParams.handle = gemHandle;
		drmIoctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &closeParams);
		free(metadata);

		if (!alloc->fb) {
			goto fail;
		}
	}

	allocator.next_allocation = 0;

	return &allocator;

fail:
	if (allocator.dev) {
		for (i = 0; i <= allocs; i++) {
			struct allocation *alloc = &allocator.allocations[i];

			if (alloc->fb) {
				drm_fb_destroy(drm_fd, alloc->fb);
			}

			device_destroy_allocation(allocator.dev,
									  alloc->alloc);
		}
	}

	device_destroy(allocator.dev);
	allocator.dev = NULL;

	return NULL;
}
#endif /* HAVE_ALLOCATOR */

const struct surfmgr * init_surfmgr(int dev_fd, int drm_fd,
									int w, int h, uint64_t modifier)
{
	surfmgr.width = w;
	surfmgr.height = h;

#ifdef HAVE_ALLOCATOR
	surfmgr.allocator = init_allocator(dev_fd, drm_fd, w, h);

	if (surfmgr.allocator)
		return &surfmgr;
#else
	(void)dev_fd;
#endif

	surfmgr.gbm = init_gbm(drm_fd, w, h, modifier);

	if (surfmgr.gbm)
		return &surfmgr;

	/* Initialization failed. */
	surfmgr.width = 0;
	surfmgr.height = 0;

	return NULL;
}

int init_surfmgr_egl(const struct surfmgr *surfmgr, const struct egl *egl)
{
#ifdef HAVE_ALLOCATOR
	if (surfmgr->allocator) {
		uint32_t i;
		for (i = 0; i < ARRAY_SIZE(surfmgr->allocator->allocations); i++) {
			const struct allocation *alloc = &surfmgr->allocator->allocations[i];
			void *metadata;
			size_t metadata_size;
			uint64_t size;
			int fd;

			static const GLint trueParam = GL_TRUE;

			if (device_export_allocation(allocator.dev,
										 alloc->alloc,
										 &size,
										 &metadata_size,
										 &metadata,
										 &fd)) {
				printf("Failed to export allocator allocation\n");
				return -1;
			}

			egl->glCreateMemoryObjectsEXT(1,
                                          &allocator.allocations[i].memoryObject);
			egl->glMemoryObjectParameterivEXT(alloc->memoryObject,
											  GL_DEDICATED_MEMORY_OBJECT_EXT,
											  &trueParam);
			egl->glImportMemoryFdEXT(alloc->memoryObject,
									 size,
									 GL_HANDLE_TYPE_ALLOCATOR_FD_NVX,
									 fd);

			glGenTextures(1, &allocator.allocations[i].texture);
			glBindTexture(GL_TEXTURE_2D,
                          alloc->texture);
			glTexParameteri(GL_TEXTURE_2D,
							GL_TEXTURE_TILING_EXT,
							GL_OPTIMAL_TILING_EXT);
			egl->glTexParametervNVX(GL_TEXTURE_2D,
									GL_SURFACE_METADATA_NVX,
									metadata_size,
									metadata);
			egl->glTexStorageMem2DEXT(GL_TEXTURE_2D,
									  1, GL_RGBA8_OES,
									  surfmgr->width, surfmgr->height,
									  alloc->memoryObject, 0);

			glGenFramebuffers(1, &allocator.allocations[i].framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, alloc->framebuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER,
								   GL_COLOR_ATTACHMENT0,
								   GL_TEXTURE_2D,
								   alloc->texture,
								   0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER,
						  surfmgr->allocator->allocations[0].framebuffer);
	}
#else
	(void)surfmgr;
	(void)egl;
#endif /* HAVE_ALLOCATOR */

	return 0;
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
#ifdef HAVE_ALLOCATOR
	else if (surfmgr->allocator) {
		uint32_t n = surfmgr->allocator->next_allocation;
		fb = surfmgr->allocator->allocations[n].fb;
		allocator.next_allocation =
			(n + 1) % ARRAY_SIZE(surfmgr->allocator->allocations);
	}
#endif

	return fb;
}

void surfmgr_release_fb(const struct surfmgr *surfmgr, struct drm_fb *fb)
{
	if (surfmgr->gbm) {
		gbm_surface_release_buffer(surfmgr->gbm->surface, fb->bo);
	}
#ifdef HAVE_ALLOCATOR
	else if (surfmgr->allocator) {
		/* Nothing to do for the allocator case */
	}
#endif
}

void surfmgr_end_frame(const struct surfmgr *surfmgr,
					   const struct egl *egl,
					   int *fence_fd)
{
	/* insert fence to be signaled in cmdstream.. this fence will be
	 * signaled when gpu rendering done
	 */
	EGLSyncKHR gpu_fence = create_fence(egl, EGL_NO_NATIVE_FENCE_FD_ANDROID);

	if (surfmgr->gbm) {
		eglSwapBuffers(egl->display, egl->surface);
	}
#ifdef HAVE_ALLOCATOR
	else if (surfmgr->allocator) {
		uint32_t slot = (surfmgr->allocator->next_allocation + 1) %
			ARRAY_SIZE(surfmgr->allocator->allocations);

		glBindFramebuffer(GL_FRAMEBUFFER,
						  surfmgr->allocator->allocations[slot].framebuffer);
		glFlush();
	}
#endif

	if (gpu_fence) {
		/* after swapbuffers, gpu_fence should be flushed, so safe
		 * to get fd:
		 */
		*fence_fd = egl->eglDupNativeFenceFDANDROID(egl->display, gpu_fence);
		egl->eglDestroySyncKHR(egl->display, gpu_fence);
		assert(*fence_fd != -1);
	} else {
		glFinish();
		*fence_fd = -1;
	}
}
