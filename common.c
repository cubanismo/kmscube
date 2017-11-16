/*
 * Copyright (c) 2017 Rob Clark <rclark@redhat.com>
 * Copyright © 2013 Intel Corporation
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
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "surface-manager.h"

static bool has_ext(const char *extension_list, const char *ext)
{
	const char *ptr = extension_list;
	int len = strlen(ext);

	if (ptr == NULL || *ptr == '\0')
		return false;

	while (true) {
		ptr = strstr(ptr, ext);
		if (!ptr)
			return false;

		if (ptr[len] == ' ' || ptr[len] == '\0')
			return true;

		ptr += len;
	}
}

int init_egl(struct egl *egl, const struct surfmgr *surfmgr)
{
	EGLint major, minor, n;

	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	static const EGLint win_config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	static const EGLint nowin_config_attribs[] = {
		EGL_SURFACE_TYPE, 0,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	const EGLint *config_attribs = surfmgr->gbm ?
		win_config_attribs : nowin_config_attribs;

	const char *egl_exts_client, *egl_exts_dpy, *gl_exts;

#define get_proc_client(ext, name) do { \
		if (has_ext(egl_exts_client, #ext)) \
			egl->name = (void *)eglGetProcAddress(#name); \
	} while (0)
#define get_proc_dpy(ext, name) do { \
		if (has_ext(egl_exts_dpy, #ext)) \
			egl->name = (void *)eglGetProcAddress(#name); \
	} while (0)

#define get_proc_gl(ext, name) do { \
		if (has_ext(gl_exts, #ext)) \
			egl->name = (void *)eglGetProcAddress(#name); \
	} while (0)

	egl_exts_client = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
	get_proc_client(EGL_EXT_platform_base, eglGetPlatformDisplayEXT);
	get_proc_client(EGL_EXT_device_base, eglQueryDevicesEXT);
	if (!egl->eglQueryDevicesEXT)
		get_proc_client(EGL_EXT_device_enumeration, eglQueryDevicesEXT);

	if (surfmgr->gbm) {
		if (egl->eglGetPlatformDisplayEXT) {
			egl->display = egl->eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR,
														 surfmgr->gbm->dev,
														 NULL);
		} else {
			egl->display = eglGetDisplay((void *)surfmgr->gbm->dev);
		}
	} else {
		EGLDeviceEXT device;
		EGLint num_devices;

		if (!egl->eglQueryDevicesEXT || !egl->eglGetPlatformDisplayEXT) {
			printf("EGLDevice or EGL platforms not supported\n");
			return -1;
		}

		/*
		 * TODO: This should correlate the EGLDevices with the device
		 * requested by the user some how, such as with the
		 * EGL_DRM_DEVICE_FILE_EXT query from EGL_EXT_device_drm or a new UUID-
		 * based mechanism similar to that used by GL_EXT_memory_objects.
		 * Right now, the first available EGLDevice is used for simplicity.
		 */
		egl->eglQueryDevicesEXT(1, &device, &num_devices);

		if (num_devices < 1) {
			printf("No EGL devices present\n");
			return -1;
		}

		egl->display = egl->eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT,
													 device, NULL);
	}

	if (!eglInitialize(egl->display, &major, &minor)) {
		printf("failed to initialize\n");
		return -1;
	}

	egl_exts_dpy = eglQueryString(egl->display, EGL_EXTENSIONS);
	get_proc_dpy(EGL_KHR_image_base, eglCreateImageKHR);
	get_proc_dpy(EGL_KHR_image_base, eglDestroyImageKHR);
	get_proc_dpy(EGL_KHR_fence_sync, eglCreateSyncKHR);
	get_proc_dpy(EGL_KHR_fence_sync, eglDestroySyncKHR);
	get_proc_dpy(EGL_KHR_fence_sync, eglWaitSyncKHR);
	get_proc_dpy(EGL_KHR_fence_sync, eglClientWaitSyncKHR);
	get_proc_dpy(EGL_ANDROID_native_fence_sync, eglDupNativeFenceFDANDROID);

	printf("Using display %p with EGL version %d.%d\n",
			egl->display, major, minor);

	printf("===================================\n");
	printf("EGL information:\n");
	printf("  version: \"%s\"\n", eglQueryString(egl->display, EGL_VERSION));
	printf("  vendor: \"%s\"\n", eglQueryString(egl->display, EGL_VENDOR));
	printf("  client extensions: \"%s\"\n", egl_exts_client);
	printf("  display extensions: \"%s\"\n", egl_exts_dpy);
	printf("===================================\n");

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		printf("failed to bind api EGL_OPENGL_ES_API\n");
		return -1;
	}

	if (!eglChooseConfig(egl->display, config_attribs, &egl->config, 1, &n) || n != 1) {
		printf("failed to choose config: %d\n", n);
		return -1;
	}

	egl->context = eglCreateContext(egl->display, egl->config,
			EGL_NO_CONTEXT, context_attribs);
	if (egl->context == NULL) {
		printf("failed to create context\n");
		return -1;
	}

	if (surfmgr->gbm) {
		egl->surface = eglCreateWindowSurface(egl->display, egl->config,
			(EGLNativeWindowType)surfmgr->gbm->surface, NULL);
		if (egl->surface == EGL_NO_SURFACE) {
			printf("failed to create egl surface\n");
			return -1;
		}
	} else {
		egl->surface = EGL_NO_SURFACE;
	}

	/* connect the context to the surface */
	eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context);

	gl_exts = (char *) glGetString(GL_EXTENSIONS);
	printf("OpenGL ES 2.x information:\n");
	printf("  version: \"%s\"\n", glGetString(GL_VERSION));
	printf("  shading language version: \"%s\"\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("  vendor: \"%s\"\n", glGetString(GL_VENDOR));
	printf("  renderer: \"%s\"\n", glGetString(GL_RENDERER));
	printf("  extensions: \"%s\"\n", gl_exts);
	printf("===================================\n");

	get_proc_gl(GL_OES_EGL_image, glEGLImageTargetTexture2DOES);
	get_proc_gl(GL_EXT_memory_object, glCreateMemoryObjectsEXT);
	get_proc_gl(GL_EXT_memory_object, glMemoryObjectParameterivEXT);
	get_proc_gl(GL_EXT_memory_object, glTexStorageMem2DEXT);
	get_proc_gl(GL_EXT_memory_object_fd, glImportMemoryFdEXT);
	get_proc_gl(GL_NVX_unix_allocator_import, glTexParametervNVX);

	if (init_surfmgr_egl(surfmgr, egl)) {
        printf("Failed to initialize surface manager EGL and GL state\n");
        return -1;
    }

	return 0;
}

int create_program(const char *vs_src, const char *fs_src)
{
	GLuint vertex_shader, fragment_shader, program;
	GLint ret;

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(vertex_shader, 1, &vs_src, NULL);
	glCompileShader(vertex_shader);

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("vertex shader compilation failed!:\n");
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &ret);
		if (ret > 1) {
			log = malloc(ret);
			glGetShaderInfoLog(vertex_shader, ret, NULL, log);
			printf("%s", log);
		}

		return -1;
	}

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(fragment_shader, 1, &fs_src, NULL);
	glCompileShader(fragment_shader);

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("fragment shader compilation failed!:\n");
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetShaderInfoLog(fragment_shader, ret, NULL, log);
			printf("%s", log);
		}

		return -1;
	}

	program = glCreateProgram();

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	return program;
}

int link_program(unsigned program)
{
	GLint ret;

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("program linking failed!:\n");
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetProgramInfoLog(program, ret, NULL, log);
			printf("%s", log);
		}

		return -1;
	}

	return 0;
}

EGLSyncKHR create_fence(const struct egl *egl, int fd)
{
	EGLSyncKHR fence = EGL_NO_SYNC_KHR;

	if (egl->eglDupNativeFenceFDANDROID) {
		EGLint attrib_list[] = {
			EGL_SYNC_NATIVE_FENCE_FD_ANDROID, fd,
			EGL_NONE,
		};
		EGLSyncKHR fence = egl->eglCreateSyncKHR(egl->display,
			EGL_SYNC_NATIVE_FENCE_ANDROID, attrib_list);
		assert(fence);
	}

	return fence;
}
