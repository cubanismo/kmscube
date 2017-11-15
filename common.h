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

#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <gbm.h>
#include <drm_fourcc.h>

#ifdef HAVE_ALLOCATOR
#include <allocator/allocator.h>
#endif

#ifndef DRM_FORMAT_MOD_LINEAR
#define DRM_FORMAT_MOD_LINEAR 0
#endif

#ifndef DRM_FORMAT_MOD_INVALID
#define DRM_FORMAT_MOD_INVALID ((((__u64)0) << 56) | ((1ULL << 56) - 1))
#endif

#ifndef EGL_KHR_platform_gbm
#define EGL_KHR_platform_gbm 1
#define EGL_PLATFORM_GBM_KHR              0x31D7
#endif /* EGL_KHR_platform_gbm */

#ifndef EGL_EXT_platform_base
#define EGL_EXT_platform_base 1
typedef EGLDisplay (EGLAPIENTRYP PFNEGLGETPLATFORMDISPLAYEXTPROC) (EGLenum platform, void *native_display, const EGLint *attrib_list);
typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC) (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATEPLATFORMPIXMAPSURFACEEXTPROC) (EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLDisplay EGLAPIENTRY eglGetPlatformDisplayEXT (EGLenum platform, void *native_display, const EGLint *attrib_list);
EGLAPI EGLSurface EGLAPIENTRY eglCreatePlatformWindowSurfaceEXT (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
EGLAPI EGLSurface EGLAPIENTRY eglCreatePlatformPixmapSurfaceEXT (EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);
#endif
#endif /* EGL_EXT_platform_base */

#ifndef EGL_EXT_device_base
#define EGL_EXT_device_base 1
typedef void *EGLDeviceEXT;
#define EGL_NO_DEVICE_EXT                 EGL_CAST(EGLDeviceEXT,0)
#define EGL_BAD_DEVICE_EXT                0x322B
#define EGL_DEVICE_EXT                    0x322C
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYDEVICEATTRIBEXTPROC) (EGLDeviceEXT device, EGLint attribute, EGLAttrib *value);
typedef const char *(EGLAPIENTRYP PFNEGLQUERYDEVICESTRINGEXTPROC) (EGLDeviceEXT device, EGLint name);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYDEVICESEXTPROC) (EGLint max_devices, EGLDeviceEXT *devices, EGLint *num_devices);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYDISPLAYATTRIBEXTPROC) (EGLDisplay dpy, EGLint attribute, EGLAttrib *value);
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglQueryDeviceAttribEXT (EGLDeviceEXT device, EGLint attribute, EGLAttrib *value);
EGLAPI const char *EGLAPIENTRY eglQueryDeviceStringEXT (EGLDeviceEXT device, EGLint name);
EGLAPI EGLBoolean EGLAPIENTRY eglQueryDevicesEXT (EGLint max_devices, EGLDeviceEXT *devices, EGLint *num_devices);
EGLAPI EGLBoolean EGLAPIENTRY eglQueryDisplayAttribEXT (EGLDisplay dpy, EGLint attribute, EGLAttrib *value);
#endif
#endif /* EGL_EXT_device_base */

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct gbm {
	struct gbm_device *dev;
	struct gbm_surface *surface;
};

#ifdef HAVE_ALLOCATOR
struct allocation {
	allocation_t *alloc;
	struct drm_fb *fb;
};

struct allocator {
	device_t *dev;

	struct allocation allocations[2]; /* double-buffer */
};
#endif /* HAVE_ALLOCATOR */

struct surfmgr {
	const struct gbm * gbm;
#ifdef HAVE_ALLOCATOR
	const struct allocator * allocator;
#endif

	int width, height;
};

struct egl {
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;

	PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;
	PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT;
	PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
	PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
	PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR;
	PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR;
	PFNEGLWAITSYNCKHRPROC eglWaitSyncKHR;
	PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
	PFNEGLDUPNATIVEFENCEFDANDROIDPROC eglDupNativeFenceFDANDROID;

	void (*draw)(unsigned i);
};

static inline int __egl_check(void *ptr, const char *name)
{
	if (!ptr) {
		printf("no %s\n", name);
		return -1;
	}
	return 0;
}

#define egl_check(egl, name) __egl_check((egl)->name, #name)

int init_egl(struct egl *egl, const struct surfmgr *surfmgr);
int create_program(const char *vs_src, const char *fs_src);
int link_program(unsigned program);

enum mode {
	SMOOTH,        /* smooth-shaded */
	RGBA,          /* single-plane RGBA */
	NV12_2IMG,     /* NV12, handled as two textures and converted to RGB in shader */
	NV12_1IMG,     /* NV12, imported as planar YUV eglimg */
	VIDEO,         /* video textured cube */
};

const struct egl * init_cube_smooth(const struct surfmgr *surfmgr);
const struct egl * init_cube_tex(const struct surfmgr *surfmgr, enum mode mode);

#ifdef HAVE_GST

struct decoder;
struct decoder * video_init(const struct egl *egl, const struct gbm *gbm, const char *filename);
EGLImage video_frame(struct decoder *dec);
void video_deinit(struct decoder *dec);

const struct egl * init_cube_video(const struct surfmgr *surfmgr, const char *video);

#else
static inline const struct egl *
init_cube_video(const struct surfmgr *surfmgr, const char *video)
{
	(void)surfmgr; (void)video;
	printf("no GStreamer support!\n");
	return NULL;
}
#endif

#endif /* _COMMON_H */
