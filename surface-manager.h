/*
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

#ifndef _SURFACE_MANAGER_H
#define _SURFACE_MANAGER_H

const struct surfmgr * init_surfmgr(int drm_fd, int w, int h, uint64_t modifier);
int init_surfmgr_egl(const struct surfmgr *surfmgr, const struct egl *egl);
struct drm_fb *surfmgr_get_next_fb(const struct surfmgr *surfmgr);
void surfmgr_release_fb(const struct surfmgr *surfmgr, struct drm_fb *fb);
void surfmgr_end_frame(const struct surfmgr *surfmgr,
					   const struct egl *egl,
					   int *fence_fd);

#endif /* _SURFACE_MANAGER_H */
