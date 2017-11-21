/*
 * Copyright (c) 2017 Rob Clark <rclark@redhat.com>
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

#ifndef _DRM_COMMON_H
#define _DRM_COMMON_H

#include <xf86drm.h>
#include <xf86drmMode.h>

struct gbm;
struct egl;

struct plane {
	drmModePlane *plane;
	drmModeObjectProperties *props;
	drmModePropertyRes **props_info;
};

struct crtc {
	drmModeCrtc *crtc;
	drmModeObjectProperties *props;
	drmModePropertyRes **props_info;
};

struct connector {
	drmModeConnector *connector;
	drmModeObjectProperties *props;
	drmModePropertyRes **props_info;
};

struct drm {
	int fd;

	/* only used for atomic: */
	struct plane *plane;
	struct crtc *crtc;
	struct connector *connector;
	int crtc_index;
	int kms_in_fence_fd;
	int kms_out_fence_fd;

	drmModeModeInfo *mode;
	uint32_t crtc_id;
	uint32_t connector_id;

	int (*run)(const struct surfmgr *surfmgr, const struct egl *egl);
};

struct drm_fb {
	struct gbm_bo *bo;
	uint32_t fb_id;
};

struct drm_fb * drm_fb_get_from_bo(struct gbm_bo *bo);
#ifdef HAVE_ALLOCATOR
struct drm_fb * drm_fb_get_from_gem(int drm_fd,
									uint32_t gemHandle,
									uint32_t width,
									uint32_t height,
									size_t metadata_size,
									void *metadata);
#endif
void drm_fb_destroy(int drm_fd, struct drm_fb *fb);

int init_drm(struct drm *drm, const char *device);
const struct drm * init_drm_legacy(const char *device);
const struct drm * init_drm_atomic(const char *device);

#endif /* _DRM_COMMON_H */
