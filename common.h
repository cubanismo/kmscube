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

#ifndef GL_EXT_memory_object
#define GL_EXT_memory_object 1
#define GL_TEXTURE_TILING_EXT             0x9580
#define GL_DEDICATED_MEMORY_OBJECT_EXT    0x9581
#define GL_PROTECTED_MEMORY_OBJECT_EXT    0x959B
#define GL_NUM_TILING_TYPES_EXT           0x9582
#define GL_TILING_TYPES_EXT               0x9583
#define GL_OPTIMAL_TILING_EXT             0x9584
#define GL_LINEAR_TILING_EXT              0x9585
#define GL_NUM_DEVICE_UUIDS_EXT           0x9596
#define GL_DEVICE_UUID_EXT                0x9597
#define GL_DRIVER_UUID_EXT                0x9598
#define GL_UUID_SIZE_EXT                  16
typedef void (GL_APIENTRYP PFNGLGETUNSIGNEDBYTEVEXTPROC) (GLenum pname, GLubyte *data);
typedef void (GL_APIENTRYP PFNGLGETUNSIGNEDBYTEI_VEXTPROC) (GLenum target, GLuint index, GLubyte *data);
typedef void (GL_APIENTRYP PFNGLDELETEMEMORYOBJECTSEXTPROC) (GLsizei n, const GLuint *memoryObjects);
typedef GLboolean (GL_APIENTRYP PFNGLISMEMORYOBJECTEXTPROC) (GLuint memoryObject);
typedef void (GL_APIENTRYP PFNGLCREATEMEMORYOBJECTSEXTPROC) (GLsizei n, GLuint *memoryObjects);
typedef void (GL_APIENTRYP PFNGLMEMORYOBJECTPARAMETERIVEXTPROC) (GLuint memoryObject, GLenum pname, const GLint *params);
typedef void (GL_APIENTRYP PFNGLGETMEMORYOBJECTPARAMETERIVEXTPROC) (GLuint memoryObject, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLTEXSTORAGEMEM2DEXTPROC) (GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset);
typedef void (GL_APIENTRYP PFNGLTEXSTORAGEMEM2DMULTISAMPLEEXTPROC) (GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset);
typedef void (GL_APIENTRYP PFNGLTEXSTORAGEMEM3DEXTPROC) (GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset);
typedef void (GL_APIENTRYP PFNGLTEXSTORAGEMEM3DMULTISAMPLEEXTPROC) (GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset);
typedef void (GL_APIENTRYP PFNGLBUFFERSTORAGEMEMEXTPROC) (GLenum target, GLsizeiptr size, GLuint memory, GLuint64 offset);
typedef void (GL_APIENTRYP PFNGLTEXTURESTORAGEMEM2DEXTPROC) (GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset);
typedef void (GL_APIENTRYP PFNGLTEXTURESTORAGEMEM2DMULTISAMPLEEXTPROC) (GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset);
typedef void (GL_APIENTRYP PFNGLTEXTURESTORAGEMEM3DEXTPROC) (GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset);
typedef void (GL_APIENTRYP PFNGLTEXTURESTORAGEMEM3DMULTISAMPLEEXTPROC) (GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset);
typedef void (GL_APIENTRYP PFNGLNAMEDBUFFERSTORAGEMEMEXTPROC) (GLuint buffer, GLsizeiptr size, GLuint memory, GLuint64 offset);
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glGetUnsignedBytevEXT (GLenum pname, GLubyte *data);
GL_APICALL void GL_APIENTRY glGetUnsignedBytei_vEXT (GLenum target, GLuint index, GLubyte *data);
GL_APICALL void GL_APIENTRY glDeleteMemoryObjectsEXT (GLsizei n, const GLuint *memoryObjects);
GL_APICALL GLboolean GL_APIENTRY glIsMemoryObjectEXT (GLuint memoryObject);
GL_APICALL void GL_APIENTRY glCreateMemoryObjectsEXT (GLsizei n, GLuint *memoryObjects);
GL_APICALL void GL_APIENTRY glMemoryObjectParameterivEXT (GLuint memoryObject, GLenum pname, const GLint *params);
GL_APICALL void GL_APIENTRY glGetMemoryObjectParameterivEXT (GLuint memoryObject, GLenum pname, GLint *params);
GL_APICALL void GL_APIENTRY glTexStorageMem2DEXT (GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset);
GL_APICALL void GL_APIENTRY glTexStorageMem2DMultisampleEXT (GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset);
GL_APICALL void GL_APIENTRY glTexStorageMem3DEXT (GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset);
GL_APICALL void GL_APIENTRY glTexStorageMem3DMultisampleEXT (GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset);
GL_APICALL void GL_APIENTRY glBufferStorageMemEXT (GLenum target, GLsizeiptr size, GLuint memory, GLuint64 offset);
GL_APICALL void GL_APIENTRY glTextureStorageMem2DEXT (GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset);
GL_APICALL void GL_APIENTRY glTextureStorageMem2DMultisampleEXT (GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset);
GL_APICALL void GL_APIENTRY glTextureStorageMem3DEXT (GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset);
GL_APICALL void GL_APIENTRY glTextureStorageMem3DMultisampleEXT (GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset);
GL_APICALL void GL_APIENTRY glNamedBufferStorageMemEXT (GLuint buffer, GLsizeiptr size, GLuint memory, GLuint64 offset);
#endif
#endif /* GL_EXT_memory_object */

#ifndef GL_EXT_memory_object_fd
#define GL_EXT_memory_object_fd 1
#define GL_HANDLE_TYPE_OPAQUE_FD_EXT      0x9586
typedef void (GL_APIENTRYP PFNGLIMPORTMEMORYFDEXTPROC) (GLuint memory, GLuint64 size, GLenum handleType, GLint fd);
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glImportMemoryFdEXT (GLuint memory, GLuint64 size, GLenum handleType, GLint fd);
#endif
#endif /* GL_EXT_memory_object_fd */

#ifndef GL_NVX_unix_allocator_import
#define GL_NVX_unix_allocator_import 1
#define GL_SURFACE_METADATA_NVX                             0xF000
#define GL_HANDLE_TYPE_ALLOCATOR_FD_NVX                     0xF001
typedef void (GL_APIENTRYP PFNGLTEXPARAMETERVNVXPROC) (GLenum target, GLenum pname, GLsizeiptr size, const GLvoid *params);
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glTexParametervNVX (GLenum target, GLenum pname, GLsizeiptr size, const GLvoid *params);
#endif /* GL_GLEXT_PROTOTYPES */
#endif

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct gbm {
	struct gbm_device *dev;
	struct gbm_surface *surface;
};

#ifdef HAVE_ALLOCATOR
struct allocation {
	allocation_t *alloc;
	struct drm_fb *fb;
	GLuint memoryObject;
	GLuint texture;
	GLuint framebuffer;
};

struct allocator {
	device_t *dev;

	struct allocation allocations[2]; /* double-buffering */

	uint32_t next_allocation;
};
#endif /* HAVE_ALLOCATOR */

struct surfmgr {
	const struct gbm * gbm;
#ifdef HAVE_ALLOCATOR
	const struct allocator * allocator;
#endif

	int width, height;
};

enum backend {
	GBM,        /* Use GBM to create buffers */
#ifdef HAVE_ALLOCATOR
	ALLOCATOR,  /* Use Generic allocator to create buffers */
#endif
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
	PFNGLCREATEMEMORYOBJECTSEXTPROC glCreateMemoryObjectsEXT;
	PFNGLMEMORYOBJECTPARAMETERIVEXTPROC glMemoryObjectParameterivEXT;
	PFNGLTEXSTORAGEMEM2DEXTPROC glTexStorageMem2DEXT;
	PFNGLIMPORTMEMORYFDEXTPROC glImportMemoryFdEXT;
	PFNGLTEXPARAMETERVNVXPROC glTexParametervNVX;

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
EGLSyncKHR create_fence(const struct egl *egl, int fd);

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
