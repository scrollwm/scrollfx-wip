#include <assert.h>
#include <drm_fourcc.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gbm.h>
#include <wlr/render/egl.h>
#include <wlr/util/log.h>
#include <wlr/util/region.h>
#include <xf86drm.h>
#include "render/egl.h"
#include "util/env.h"

static enum wlr_log_importance egl_log_importance_to_wlr(EGLint type) {
	switch (type) {
	case EGL_DEBUG_MSG_CRITICAL_KHR: return WLR_ERROR;
	case EGL_DEBUG_MSG_ERROR_KHR:    return WLR_ERROR;
	case EGL_DEBUG_MSG_WARN_KHR:     return WLR_ERROR;
	case EGL_DEBUG_MSG_INFO_KHR:     return WLR_INFO;
	default:                         return WLR_INFO;
	}
}

static const char *egl_error_str(EGLint error) {
	switch (error) {
	case EGL_SUCCESS:
		return "EGL_SUCCESS";
	case EGL_NOT_INITIALIZED:
		return "EGL_NOT_INITIALIZED";
	case EGL_BAD_ACCESS:
		return "EGL_BAD_ACCESS";
	case EGL_BAD_ALLOC:
		return "EGL_BAD_ALLOC";
	case EGL_BAD_ATTRIBUTE:
		return "EGL_BAD_ATTRIBUTE";
	case EGL_BAD_CONTEXT:
		return "EGL_BAD_CONTEXT";
	case EGL_BAD_CONFIG:
		return "EGL_BAD_CONFIG";
	case EGL_BAD_CURRENT_SURFACE:
		return "EGL_BAD_CURRENT_SURFACE";
	case EGL_BAD_DISPLAY:
		return "EGL_BAD_DISPLAY";
	case EGL_BAD_DEVICE_EXT:
		return "EGL_BAD_DEVICE_EXT";
	case EGL_BAD_SURFACE:
		return "EGL_BAD_SURFACE";
	case EGL_BAD_MATCH:
		return "EGL_BAD_MATCH";
	case EGL_BAD_PARAMETER:
		return "EGL_BAD_PARAMETER";
	case EGL_BAD_NATIVE_PIXMAP:
		return "EGL_BAD_NATIVE_PIXMAP";
	case EGL_BAD_NATIVE_WINDOW:
		return "EGL_BAD_NATIVE_WINDOW";
	case EGL_CONTEXT_LOST:
		return "EGL_CONTEXT_LOST";
	}
	return "unknown error";
}

static void egl_log(EGLenum error, const char *command, EGLint msg_type,
		EGLLabelKHR thread, EGLLabelKHR obj, const char *msg) {
	_wlr_log(egl_log_importance_to_wlr(msg_type),
		"[EGL] command: %s, error: %s (0x%x), message: \"%s\"",
		command, egl_error_str(error), error, msg);
}

static bool check_egl_ext(const char *exts, const char *ext) {
	size_t extlen = strlen(ext);
	const char *end = exts + strlen(exts);

	while (exts < end) {
		if (*exts == ' ') {
			exts++;
			continue;
		}
		size_t n = strcspn(exts, " ");
		if (n == extlen && strncmp(ext, exts, n) == 0) {
			return true;
		}
		exts += n;
	}
	return false;
}

static void load_egl_proc(void *proc_ptr, const char *name) {
	void *proc = (void *)eglGetProcAddress(name);
	if (proc == NULL) {
		wlr_log(WLR_ERROR, "eglGetProcAddress(%s) failed", name);
		abort();
	}
	*(void **)proc_ptr = proc;
}

static int get_egl_dmabuf_formats(struct wlr_egl *egl, EGLint **formats);
static int get_egl_dmabuf_modifiers(struct wlr_egl *egl, EGLint format,
	uint64_t **modifiers, EGLBoolean **external_only);

static void log_modifier(uint64_t modifier, bool external_only) {
	char *mod_name = drmGetFormatModifierName(modifier);
	wlr_log(WLR_DEBUG, "    %s (0x%016"PRIX64"): ✓ texture  %s render",
		mod_name ? mod_name : "<unknown>", modifier, external_only ? "✗" : "✓");
	free(mod_name);
}

static void init_dmabuf_formats(struct wlr_egl *egl) {
	bool no_modifiers = env_parse_bool("WLR_EGL_NO_MODIFIERS");
	if (no_modifiers) {
		wlr_log(WLR_INFO, "WLR_EGL_NO_MODIFIERS set, disabling modifiers for EGL");
	}

	EGLint *formats;
	int formats_len = get_egl_dmabuf_formats(egl, &formats);
	if (formats_len < 0) {
		return;
	}

	wlr_log(WLR_DEBUG, "Supported DMA-BUF formats:");

	bool has_modifiers = false;
	for (int i = 0; i < formats_len; i++) {
		EGLint fmt = formats[i];

		uint64_t *modifiers = NULL;
		EGLBoolean *external_only = NULL;
		int modifiers_len = 0;
		if (!no_modifiers) {
			modifiers_len = get_egl_dmabuf_modifiers(egl, fmt, &modifiers, &external_only);
		}
		if (modifiers_len < 0) {
			continue;
		}

		has_modifiers = has_modifiers || modifiers_len > 0;

		bool all_external_only = true;
		for (int j = 0; j < modifiers_len; j++) {
			wlr_drm_format_set_add(&egl->dmabuf_texture_formats, fmt,
				modifiers[j]);
			if (!external_only[j]) {
				wlr_drm_format_set_add(&egl->dmabuf_render_formats, fmt,
					modifiers[j]);
				all_external_only = false;
			}
		}

		// EGL always supports implicit modifiers. If at least one modifier supports rendering,
		// assume the implicit modifier supports rendering too.
		wlr_drm_format_set_add(&egl->dmabuf_texture_formats, fmt,
			DRM_FORMAT_MOD_INVALID);
		if (modifiers_len == 0 || !all_external_only) {
			wlr_drm_format_set_add(&egl->dmabuf_render_formats, fmt,
				DRM_FORMAT_MOD_INVALID);
		}

		if (modifiers_len == 0) {
			// Asume the linear layout is supported if the driver doesn't
			// explicitly say otherwise
			wlr_drm_format_set_add(&egl->dmabuf_texture_formats, fmt,
				DRM_FORMAT_MOD_LINEAR);
			wlr_drm_format_set_add(&egl->dmabuf_render_formats, fmt,
				DRM_FORMAT_MOD_LINEAR);
		}

		if (wlr_log_get_verbosity() >= WLR_DEBUG) {
			char *fmt_name = drmGetFormatName(fmt);
			wlr_log(WLR_DEBUG, "  %s (0x%08"PRIX32")",
				fmt_name ? fmt_name : "<unknown>", fmt);
			free(fmt_name);

			log_modifier(DRM_FORMAT_MOD_INVALID, false);
			if (modifiers_len == 0) {
				log_modifier(DRM_FORMAT_MOD_LINEAR, false);
			}
			for (int j = 0; j < modifiers_len; j++) {
				log_modifier(modifiers[j], external_only[j]);
			}
		}

		free(modifiers);
		free(external_only);
	}
	free(formats);

	egl->has_modifiers = has_modifiers;
	if (!no_modifiers) {
		wlr_log(WLR_DEBUG, "EGL DMA-BUF format modifiers %s",
			has_modifiers ? "supported" : "unsupported");
	}
}

static struct wlr_egl *egl_create(void) {
	const char *client_exts_str = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
	if (client_exts_str == NULL) {
		if (eglGetError() == EGL_BAD_DISPLAY) {
			wlr_log(WLR_ERROR, "EGL_EXT_client_extensions not supported");
		} else {
			wlr_log(WLR_ERROR, "Failed to query EGL client extensions");
		}
		return NULL;
	}

	wlr_log(WLR_INFO, "Supported EGL client extensions: %s", client_exts_str);

	if (!check_egl_ext(client_exts_str, "EGL_EXT_platform_base")) {
		wlr_log(WLR_ERROR, "EGL_EXT_platform_base not supported");
		return NULL;
	}

	struct wlr_egl *egl = calloc(1, sizeof(*egl));
	if (egl == NULL) {
		wlr_log_errno(WLR_ERROR, "Allocation failed");
		return NULL;
	}

	load_egl_proc(&egl->procs.eglGetPlatformDisplayEXT,
		"eglGetPlatformDisplayEXT");

	egl->exts.KHR_platform_gbm = check_egl_ext(client_exts_str,
			"EGL_KHR_platform_gbm");
	egl->exts.EXT_platform_device = check_egl_ext(client_exts_str,
			"EGL_EXT_platform_device");
	egl->exts.KHR_display_reference = check_egl_ext(client_exts_str,
			"EGL_KHR_display_reference");

	if (check_egl_ext(client_exts_str, "EGL_EXT_device_base") || check_egl_ext(client_exts_str, "EGL_EXT_device_enumeration")) {
		load_egl_proc(&egl->procs.eglQueryDevicesEXT, "eglQueryDevicesEXT");
	}

	if (check_egl_ext(client_exts_str, "EGL_EXT_device_base") || check_egl_ext(client_exts_str, "EGL_EXT_device_query")) {
		egl->exts.EXT_device_query = true;
		load_egl_proc(&egl->procs.eglQueryDeviceStringEXT,
			"eglQueryDeviceStringEXT");
		load_egl_proc(&egl->procs.eglQueryDisplayAttribEXT,
			"eglQueryDisplayAttribEXT");
	}

	if (check_egl_ext(client_exts_str, "EGL_KHR_debug")) {
		load_egl_proc(&egl->procs.eglDebugMessageControlKHR,
			"eglDebugMessageControlKHR");

		static const EGLAttrib debug_attribs[] = {
			EGL_DEBUG_MSG_CRITICAL_KHR, EGL_TRUE,
			EGL_DEBUG_MSG_ERROR_KHR, EGL_TRUE,
			EGL_DEBUG_MSG_WARN_KHR, EGL_TRUE,
			EGL_DEBUG_MSG_INFO_KHR, EGL_TRUE,
			EGL_NONE,
		};
		egl->procs.eglDebugMessageControlKHR(egl_log, debug_attribs);
	}

	if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
		wlr_log(WLR_ERROR, "Failed to bind to the OpenGL ES API");
		free(egl);
		return NULL;
	}

	return egl;
}

static bool egl_init_display(struct wlr_egl *egl, EGLDisplay display) {
	egl->display = display;

	EGLint major, minor;
	if (eglInitialize(egl->display, &major, &minor) == EGL_FALSE) {
		wlr_log(WLR_ERROR, "Failed to initialize EGL");
		return false;
	}

	const char *display_exts_str = eglQueryString(egl->display, EGL_EXTENSIONS);
	if (display_exts_str == NULL) {
		wlr_log(WLR_ERROR, "Failed to query EGL display extensions");
		return false;
	}

	if (check_egl_ext(display_exts_str, "EGL_KHR_image_base")) {
		egl->exts.KHR_image_base = true;
		load_egl_proc(&egl->procs.eglCreateImageKHR, "eglCreateImageKHR");
		load_egl_proc(&egl->procs.eglDestroyImageKHR, "eglDestroyImageKHR");
	}

	egl->exts.EXT_image_dma_buf_import =
		check_egl_ext(display_exts_str, "EGL_EXT_image_dma_buf_import");
	if (check_egl_ext(display_exts_str,
			"EGL_EXT_image_dma_buf_import_modifiers")) {
		egl->exts.EXT_image_dma_buf_import_modifiers = true;
		load_egl_proc(&egl->procs.eglQueryDmaBufFormatsEXT,
			"eglQueryDmaBufFormatsEXT");
		load_egl_proc(&egl->procs.eglQueryDmaBufModifiersEXT,
			"eglQueryDmaBufModifiersEXT");
	}

	egl->exts.EXT_create_context_robustness =
		check_egl_ext(display_exts_str, "EGL_EXT_create_context_robustness");

	const char *device_exts_str = NULL, *driver_name = NULL;
	if (egl->exts.EXT_device_query) {
		EGLAttrib device_attrib;
		if (!egl->procs.eglQueryDisplayAttribEXT(egl->display,
				EGL_DEVICE_EXT, &device_attrib)) {
			wlr_log(WLR_ERROR, "eglQueryDisplayAttribEXT(EGL_DEVICE_EXT) failed");
			return false;
		}
		egl->device = (EGLDeviceEXT)device_attrib;

		device_exts_str =
			egl->procs.eglQueryDeviceStringEXT(egl->device, EGL_EXTENSIONS);
		if (device_exts_str == NULL) {
			wlr_log(WLR_ERROR, "eglQueryDeviceStringEXT(EGL_EXTENSIONS) failed");
			return false;
		}

#ifdef EGL_DRIVER_NAME_EXT
		if (check_egl_ext(device_exts_str, "EGL_EXT_device_persistent_id")) {
			driver_name = egl->procs.eglQueryDeviceStringEXT(egl->device,
				EGL_DRIVER_NAME_EXT);
		}
#endif

		egl->exts.EXT_device_drm =
			check_egl_ext(device_exts_str, "EGL_EXT_device_drm");
		egl->exts.EXT_device_drm_render_node =
			check_egl_ext(device_exts_str, "EGL_EXT_device_drm_render_node");

		// The only way a non-DRM device is selected is when the user
		// explicitly picks software rendering
		if (check_egl_ext(device_exts_str, "EGL_MESA_device_software") &&
				egl->exts.EXT_device_drm) {
			if (env_parse_bool("WLR_RENDERER_ALLOW_SOFTWARE")) {
				wlr_log(WLR_INFO, "Using software rendering");
			} else {
				wlr_log(WLR_ERROR, "Software rendering detected, please use "
					"the WLR_RENDERER_ALLOW_SOFTWARE environment variable "
					"to proceed");
				return false;
			}
		}
	}

	if (!check_egl_ext(display_exts_str, "EGL_KHR_no_config_context") &&
			!check_egl_ext(display_exts_str, "EGL_MESA_configless_context")) {
		wlr_log(WLR_ERROR, "EGL_KHR_no_config_context or "
			"EGL_MESA_configless_context not supported");
		return false;
	}

	if (!check_egl_ext(display_exts_str, "EGL_KHR_surfaceless_context")) {
		wlr_log(WLR_ERROR, "EGL_KHR_surfaceless_context not supported");
		return false;
	}

	if (check_egl_ext(display_exts_str, "EGL_KHR_fence_sync") &&
			check_egl_ext(display_exts_str, "EGL_ANDROID_native_fence_sync")) {
		load_egl_proc(&egl->procs.eglCreateSyncKHR, "eglCreateSyncKHR");
		load_egl_proc(&egl->procs.eglDestroySyncKHR, "eglDestroySyncKHR");
		load_egl_proc(&egl->procs.eglDupNativeFenceFDANDROID,
			"eglDupNativeFenceFDANDROID");
	}

	if (check_egl_ext(display_exts_str, "EGL_KHR_wait_sync")) {
		load_egl_proc(&egl->procs.eglWaitSyncKHR, "eglWaitSyncKHR");
	}

	egl->exts.IMG_context_priority =
		check_egl_ext(display_exts_str, "EGL_IMG_context_priority");

	wlr_log(WLR_INFO, "Using EGL %d.%d", (int)major, (int)minor);
	wlr_log(WLR_INFO, "Supported EGL display extensions: %s", display_exts_str);
	if (device_exts_str != NULL) {
		wlr_log(WLR_INFO, "Supported EGL device extensions: %s", device_exts_str);
	}
	wlr_log(WLR_INFO, "EGL vendor: %s", eglQueryString(egl->display, EGL_VENDOR));
	if (driver_name != NULL) {
		wlr_log(WLR_INFO, "EGL driver name: %s", driver_name);
	}

	init_dmabuf_formats(egl);

	return true;
}

static bool egl_init(struct wlr_egl *egl, EGLenum platform,
		void *remote_display) {
	EGLint display_attribs[3] = {0};
	size_t display_attribs_len = 0;

	if (egl->exts.KHR_display_reference) {
		display_attribs[display_attribs_len++] = EGL_TRACK_REFERENCES_KHR;
		display_attribs[display_attribs_len++] = EGL_TRUE;
	}

	display_attribs[display_attribs_len++] = EGL_NONE;
	assert(display_attribs_len <= sizeof(display_attribs) / sizeof(display_attribs[0]));

	EGLDisplay display = egl->procs.eglGetPlatformDisplayEXT(platform,
		remote_display, display_attribs);
	if (display == EGL_NO_DISPLAY) {
		wlr_log(WLR_ERROR, "Failed to create EGL display");
		return false;
	}

	if (!egl_init_display(egl, display)) {
		if (egl->exts.KHR_display_reference) {
			eglTerminate(display);
		}
		return false;
	}

	size_t atti = 0;
	EGLint attribs[7];

	// Try with gles3 first to check if the graphics drivers support it.
	// If it fails, it will fallback to gles2
	attribs[atti++] = EGL_CONTEXT_CLIENT_VERSION;
	attribs[atti++] = 3;

	// Request a high priority context if possible
	// TODO: only do this if we're running as the DRM master
	bool request_high_priority = egl->exts.IMG_context_priority;

	// Try to reschedule all of our rendering to be completed first. If it
	// fails, it will fallback to the default priority (MEDIUM).
	if (request_high_priority) {
		attribs[atti++] = EGL_CONTEXT_PRIORITY_LEVEL_IMG;
		attribs[atti++] = EGL_CONTEXT_PRIORITY_HIGH_IMG;
	}

	if (egl->exts.EXT_create_context_robustness) {
		attribs[atti++] = EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT;
		attribs[atti++] = EGL_LOSE_CONTEXT_ON_RESET_EXT;
	}

	attribs[atti++] = EGL_NONE;
	assert(atti <= sizeof(attribs)/sizeof(attribs[0]));

	// Attempt to create a context with gles3
	egl->context = eglCreateContext(egl->display, EGL_NO_CONFIG_KHR,
		EGL_NO_CONTEXT, attribs);
	if (egl->context != EGL_NO_CONTEXT) {
		wlr_log(WLR_DEBUG, "Created EGL context using OpenGL ES 3.0");
	} else {
		wlr_log(WLR_INFO, "Failed to create EGL context using OpenGL ES 3.0");

		// Retry with gles2
		attribs[1] = 2;
		egl->context = eglCreateContext(egl->display, EGL_NO_CONFIG_KHR,
			EGL_NO_CONTEXT, attribs);
		if (egl->context != EGL_NO_CONTEXT) {
			wlr_log(WLR_DEBUG, "Created EGL context using OpenGL ES 2.0");
		} else {
			wlr_log(WLR_ERROR, "Failed to create EGL context using OpenGL ES 2.0");
			return false;
		}
	}

	if (request_high_priority) {
		EGLint priority = EGL_CONTEXT_PRIORITY_MEDIUM_IMG;
		eglQueryContext(egl->display, egl->context,
			EGL_CONTEXT_PRIORITY_LEVEL_IMG, &priority);
		if (priority != EGL_CONTEXT_PRIORITY_HIGH_IMG) {
			wlr_log(WLR_INFO, "Failed to obtain a high priority context");
		} else {
			wlr_log(WLR_DEBUG, "Obtained high priority context");
		}
	}

	return true;
}

static bool device_has_name(const drmDevice *device, const char *name);

static EGLDeviceEXT get_egl_device_from_drm_fd(struct wlr_egl *egl,
		int drm_fd) {
	if (egl->procs.eglQueryDevicesEXT == NULL) {
		wlr_log(WLR_DEBUG, "EGL_EXT_device_enumeration not supported");
		return EGL_NO_DEVICE_EXT;
	} else if (!egl->exts.EXT_device_query) {
		wlr_log(WLR_DEBUG, "EGL_EXT_device_query not supported");
		return EGL_NO_DEVICE_EXT;
	}

	EGLint nb_devices = 0;
	if (!egl->procs.eglQueryDevicesEXT(0, NULL, &nb_devices)) {
		wlr_log(WLR_ERROR, "Failed to query EGL devices");
		return EGL_NO_DEVICE_EXT;
	}

	EGLDeviceEXT *devices = calloc(nb_devices, sizeof(*devices));
	if (devices == NULL) {
		wlr_log_errno(WLR_ERROR, "Failed to allocate EGL device list");
		return EGL_NO_DEVICE_EXT;
	}

	if (!egl->procs.eglQueryDevicesEXT(nb_devices, devices, &nb_devices)) {
		wlr_log(WLR_ERROR, "Failed to query EGL devices");
		free(devices);
		return EGL_NO_DEVICE_EXT;
	}

	drmDevice *selected_drm_device = NULL;
	if (drm_fd >= 0) {
		int ret = drmGetDevice(drm_fd, &selected_drm_device);
		if (ret < 0) {
			wlr_log(WLR_ERROR, "Failed to get DRM device: %s", strerror(-ret));
			free(devices);
			return EGL_NO_DEVICE_EXT;
		}
	}

	EGLDeviceEXT egl_device = NULL;
	for (int i = 0; i < nb_devices; i++) {
		const char *device_exts_str = egl->procs.eglQueryDeviceStringEXT(devices[i], EGL_EXTENSIONS);
		if (device_exts_str == NULL) {
			wlr_log(WLR_ERROR, "eglQueryDeviceStringEXT(EGL_EXTENSIONS) failed");
			continue;
		}

		const char *egl_device_name = NULL;
		if (check_egl_ext(device_exts_str, "EGL_EXT_device_drm")) {
			egl_device_name = egl->procs.eglQueryDeviceStringEXT(devices[i], EGL_DRM_DEVICE_FILE_EXT);
			if (egl_device_name == NULL) {
				wlr_log(WLR_ERROR, "eglQueryDeviceStringEXT(EGL_DRM_DEVICE_FILE_EXT) failed");
				continue;
			}
		}

		bool is_software = check_egl_ext(device_exts_str, "EGL_MESA_device_software");

		bool found;
		if (selected_drm_device != NULL) {
			found = egl_device_name != NULL && device_has_name(selected_drm_device, egl_device_name);
		} else {
			found = is_software;
		}
		if (found) {
			if (egl_device_name != NULL) {
				wlr_log(WLR_DEBUG, "Using EGL device %s", egl_device_name);
			}
			egl_device = devices[i];
			break;
		}
	}

	drmFreeDevice(&selected_drm_device);
	free(devices);

	return egl_device;
}

static int open_render_node(int drm_fd) {
	char *render_name = drmGetRenderDeviceNameFromFd(drm_fd);
	if (render_name == NULL) {
		// This can happen on split render/display platforms, fallback to
		// primary node
		render_name = drmGetPrimaryDeviceNameFromFd(drm_fd);
		if (render_name == NULL) {
			wlr_log_errno(WLR_ERROR, "drmGetPrimaryDeviceNameFromFd failed");
			return -1;
		}
		wlr_log(WLR_DEBUG, "DRM device '%s' has no render node, "
			"falling back to primary node", render_name);
	}

	int render_fd = open(render_name, O_RDWR | O_CLOEXEC);
	if (render_fd < 0) {
		wlr_log_errno(WLR_ERROR, "Failed to open DRM node '%s'", render_name);
	}
	free(render_name);
	return render_fd;
}

struct wlr_egl *wlr_egl_create_with_drm_fd(int drm_fd) {
	struct wlr_egl *egl = egl_create();
	if (egl == NULL) {
		wlr_log(WLR_ERROR, "Failed to create EGL context");
		return NULL;
	}

	if (egl->exts.EXT_platform_device) {
		/*
		 * Search for the EGL device matching the DRM fd using the
		 * EXT_device_enumeration extension.
		 */
		EGLDeviceEXT egl_device = get_egl_device_from_drm_fd(egl, drm_fd);
		if (egl_device != EGL_NO_DEVICE_EXT) {
			if (egl_init(egl, EGL_PLATFORM_DEVICE_EXT, egl_device)) {
				wlr_log(WLR_DEBUG, "Using EGL_PLATFORM_DEVICE_EXT");
				return egl;
			}
			goto error;
		}
		/* Falls back on GBM in case the device was not found */
	} else {
		wlr_log(WLR_DEBUG, "EXT_platform_device not supported");
	}

	if (egl->exts.KHR_platform_gbm && drm_fd >= 0) {
		int gbm_fd = open_render_node(drm_fd);
		if (gbm_fd < 0) {
			wlr_log(WLR_ERROR, "Failed to open DRM render node");
			goto error;
		}

		egl->gbm_device = gbm_create_device(gbm_fd);
		if (!egl->gbm_device) {
			close(gbm_fd);
			wlr_log(WLR_ERROR, "Failed to create GBM device");
			goto error;
		}

		if (egl_init(egl, EGL_PLATFORM_GBM_KHR, egl->gbm_device)) {
			wlr_log(WLR_DEBUG, "Using EGL_PLATFORM_GBM_KHR");
			return egl;
		}

		gbm_device_destroy(egl->gbm_device);
		close(gbm_fd);
	} else {
		wlr_log(WLR_DEBUG, "KHR_platform_gbm not supported");
	}

error:
	wlr_log(WLR_ERROR, "Failed to initialize EGL context");
	free(egl);
	eglReleaseThread();
	return NULL;
}

struct wlr_egl *wlr_egl_create_with_context(EGLDisplay display,
		EGLContext context) {
	EGLint client_type;
	if (!eglQueryContext(display, context, EGL_CONTEXT_CLIENT_TYPE, &client_type) ||
			client_type != EGL_OPENGL_ES_API) {
		wlr_log(WLR_ERROR, "Unsupported EGL context client type (need OpenGL ES)");
		return NULL;
	}

	EGLint client_version;
	if (!eglQueryContext(display, context, EGL_CONTEXT_CLIENT_VERSION, &client_version) ||
			client_version < 2) {
		wlr_log(WLR_ERROR, "Unsupported EGL context client version (need OpenGL ES >= 2)");
		return NULL;
	}

	struct wlr_egl *egl = egl_create();
	if (egl == NULL) {
		return NULL;
	}

	if (!egl_init_display(egl, display)) {
		free(egl);
		return NULL;
	}

	egl->context = context;

	return egl;
}

void wlr_egl_destroy(struct wlr_egl *egl) {
	if (egl == NULL) {
		return;
	}

	wlr_drm_format_set_finish(&egl->dmabuf_render_formats);
	wlr_drm_format_set_finish(&egl->dmabuf_texture_formats);

	eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(egl->display, egl->context);

	if (egl->exts.KHR_display_reference) {
		eglTerminate(egl->display);
	}

	eglReleaseThread();

	if (egl->gbm_device) {
		int gbm_fd = gbm_device_get_fd(egl->gbm_device);
		gbm_device_destroy(egl->gbm_device);
		close(gbm_fd);
	}

	free(egl);
}

EGLDisplay wlr_egl_get_display(struct wlr_egl *egl) {
	return egl->display;
}

EGLContext wlr_egl_get_context(struct wlr_egl *egl) {
	return egl->context;
}

bool wlr_egl_destroy_image(struct wlr_egl *egl, EGLImage image) {
	if (!egl->exts.KHR_image_base) {
		return false;
	}
	if (!image) {
		return true;
	}
	return egl->procs.eglDestroyImageKHR(egl->display, image);
}

bool wlr_egl_make_current(struct wlr_egl *egl,
		struct wlr_egl_context *save_context) {
	if (save_context != NULL) {
		save_context->display = eglGetCurrentDisplay();
		save_context->context = eglGetCurrentContext();
		save_context->draw_surface = eglGetCurrentSurface(EGL_DRAW);
		save_context->read_surface = eglGetCurrentSurface(EGL_READ);
	}
	if (!eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
			egl->context)) {
		wlr_log(WLR_ERROR, "eglMakeCurrent failed");
		return false;
	}
	return true;
}

bool wlr_egl_unset_current(struct wlr_egl *egl) {
	if (!eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
			EGL_NO_CONTEXT)) {
		wlr_log(WLR_ERROR, "eglMakeCurrent failed");
		return false;
	}
	return true;
}

bool wlr_egl_restore_context(struct wlr_egl_context *context) {
	// If the saved context is a null-context, we must use the current
	// display instead of the saved display because eglMakeCurrent() can't
	// handle EGL_NO_DISPLAY.
	EGLDisplay display = context->display == EGL_NO_DISPLAY ?
		eglGetCurrentDisplay() : context->display;

	// If the current display is also EGL_NO_DISPLAY, we assume that there
	// is currently no context set and no action needs to be taken to unset
	// the context.
	if (display == EGL_NO_DISPLAY) {
		return true;
	}

	return eglMakeCurrent(display, context->draw_surface,
			context->read_surface, context->context);
}

EGLImageKHR wlr_egl_create_image_from_dmabuf(struct wlr_egl *egl,
		struct wlr_dmabuf_attributes *attributes, bool *external_only) {
	if (!egl->exts.KHR_image_base || !egl->exts.EXT_image_dma_buf_import) {
		wlr_log(WLR_ERROR, "dmabuf import extension not present");
		return NULL;
	}

	if (attributes->modifier != DRM_FORMAT_MOD_INVALID &&
			attributes->modifier != DRM_FORMAT_MOD_LINEAR &&
			!egl->has_modifiers) {
		wlr_log(WLR_ERROR, "EGL implementation doesn't support modifiers");
		return NULL;
	}

	unsigned int atti = 0;
	EGLint attribs[50];
	attribs[atti++] = EGL_WIDTH;
	attribs[atti++] = attributes->width;
	attribs[atti++] = EGL_HEIGHT;
	attribs[atti++] = attributes->height;
	attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
	attribs[atti++] = attributes->format;

	struct {
		EGLint fd;
		EGLint offset;
		EGLint pitch;
		EGLint mod_lo;
		EGLint mod_hi;
	} attr_names[WLR_DMABUF_MAX_PLANES] = {
		{
			EGL_DMA_BUF_PLANE0_FD_EXT,
			EGL_DMA_BUF_PLANE0_OFFSET_EXT,
			EGL_DMA_BUF_PLANE0_PITCH_EXT,
			EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
			EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT
		}, {
			EGL_DMA_BUF_PLANE1_FD_EXT,
			EGL_DMA_BUF_PLANE1_OFFSET_EXT,
			EGL_DMA_BUF_PLANE1_PITCH_EXT,
			EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
			EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT
		}, {
			EGL_DMA_BUF_PLANE2_FD_EXT,
			EGL_DMA_BUF_PLANE2_OFFSET_EXT,
			EGL_DMA_BUF_PLANE2_PITCH_EXT,
			EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT,
			EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT
		}, {
			EGL_DMA_BUF_PLANE3_FD_EXT,
			EGL_DMA_BUF_PLANE3_OFFSET_EXT,
			EGL_DMA_BUF_PLANE3_PITCH_EXT,
			EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT,
			EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT
		}
	};

	for (int i = 0; i < attributes->n_planes; i++) {
		attribs[atti++] = attr_names[i].fd;
		attribs[atti++] = attributes->fd[i];
		attribs[atti++] = attr_names[i].offset;
		attribs[atti++] = attributes->offset[i];
		attribs[atti++] = attr_names[i].pitch;
		attribs[atti++] = attributes->stride[i];
		if (egl->has_modifiers &&
				attributes->modifier != DRM_FORMAT_MOD_INVALID) {
			attribs[atti++] = attr_names[i].mod_lo;
			attribs[atti++] = attributes->modifier & 0xFFFFFFFF;
			attribs[atti++] = attr_names[i].mod_hi;
			attribs[atti++] = attributes->modifier >> 32;
		}
	}

	// Our clients don't expect our usage to trash the buffer contents
	attribs[atti++] = EGL_IMAGE_PRESERVED_KHR;
	attribs[atti++] = EGL_TRUE;

	attribs[atti++] = EGL_NONE;
	assert(atti <= sizeof(attribs)/sizeof(attribs[0]));

	EGLImageKHR image = egl->procs.eglCreateImageKHR(egl->display, EGL_NO_CONTEXT,
		EGL_LINUX_DMA_BUF_EXT, NULL, attribs);
	if (image == EGL_NO_IMAGE_KHR) {
		wlr_log(WLR_ERROR, "eglCreateImageKHR failed");
		return EGL_NO_IMAGE_KHR;
	}

	*external_only = !wlr_drm_format_set_has(&egl->dmabuf_render_formats,
		attributes->format, attributes->modifier);
	return image;
}

static int get_egl_dmabuf_formats(struct wlr_egl *egl, EGLint **formats) {
	if (!egl->exts.EXT_image_dma_buf_import) {
		wlr_log(WLR_DEBUG, "DMA-BUF import extension not present");
		return -1;
	}

	// when we only have the image_dmabuf_import extension we can't query
	// which formats are supported. These two are on almost always
	// supported; it's the intended way to just try to create buffers.
	// Just a guess but better than not supporting dmabufs at all,
	// given that the modifiers extension isn't supported everywhere.
	if (!egl->exts.EXT_image_dma_buf_import_modifiers) {
		static const EGLint fallback_formats[] = {
			DRM_FORMAT_ARGB8888,
			DRM_FORMAT_XRGB8888,
		};
		int num = sizeof(fallback_formats) / sizeof(fallback_formats[0]);

		*formats = calloc(num, sizeof(**formats));
		if (!*formats) {
			wlr_log_errno(WLR_ERROR, "Allocation failed");
			return -1;
		}

		memcpy(*formats, fallback_formats, num * sizeof(**formats));
		return num;
	}

	EGLint num;
	if (!egl->procs.eglQueryDmaBufFormatsEXT(egl->display, 0, NULL, &num)) {
		wlr_log(WLR_ERROR, "Failed to query number of dmabuf formats");
		return -1;
	}

	*formats = calloc(num, sizeof(**formats));
	if (*formats == NULL) {
		wlr_log(WLR_ERROR, "Allocation failed: %s", strerror(errno));
		return -1;
	}

	if (!egl->procs.eglQueryDmaBufFormatsEXT(egl->display, num, *formats, &num)) {
		wlr_log(WLR_ERROR, "Failed to query dmabuf format");
		free(*formats);
		return -1;
	}
	return num;
}

static int get_egl_dmabuf_modifiers(struct wlr_egl *egl, EGLint format,
		uint64_t **modifiers, EGLBoolean **external_only) {
	*modifiers = NULL;
	*external_only = NULL;

	if (!egl->exts.EXT_image_dma_buf_import) {
		wlr_log(WLR_DEBUG, "DMA-BUF extension not present");
		return -1;
	}
	if (!egl->exts.EXT_image_dma_buf_import_modifiers) {
		return 0;
	}

	EGLint num;
	if (!egl->procs.eglQueryDmaBufModifiersEXT(egl->display, format, 0,
			NULL, NULL, &num)) {
		wlr_log(WLR_ERROR, "Failed to query dmabuf number of modifiers");
		return -1;
	}
	if (num == 0) {
		return 0;
	}

	*modifiers = calloc(num, sizeof(**modifiers));
	if (*modifiers == NULL) {
		wlr_log_errno(WLR_ERROR, "Allocation failed");
		return -1;
	}
	*external_only = calloc(num, sizeof(**external_only));
	if (*external_only == NULL) {
		wlr_log_errno(WLR_ERROR, "Allocation failed");
		free(*modifiers);
		*modifiers = NULL;
		return -1;
	}

	if (!egl->procs.eglQueryDmaBufModifiersEXT(egl->display, format, num,
			*modifiers, *external_only, &num)) {
		wlr_log(WLR_ERROR, "Failed to query dmabuf modifiers");
		free(*modifiers);
		free(*external_only);
		return -1;
	}
	return num;
}

const struct wlr_drm_format_set *wlr_egl_get_dmabuf_texture_formats(
		struct wlr_egl *egl) {
	return &egl->dmabuf_texture_formats;
}

const struct wlr_drm_format_set *wlr_egl_get_dmabuf_render_formats(
		struct wlr_egl *egl) {
	return &egl->dmabuf_render_formats;
}

static bool device_has_name(const drmDevice *device, const char *name) {
	for (size_t i = 0; i < DRM_NODE_MAX; i++) {
		if (!(device->available_nodes & (1 << i))) {
			continue;
		}
		if (strcmp(device->nodes[i], name) == 0) {
			return true;
		}
	}
	return false;
}

static char *get_render_name(const char *name) {
	uint32_t flags = 0;
	int devices_len = drmGetDevices2(flags, NULL, 0);
	if (devices_len < 0) {
		wlr_log(WLR_ERROR, "drmGetDevices2 failed: %s", strerror(-devices_len));
		return NULL;
	}
	drmDevice **devices = calloc(devices_len, sizeof(*devices));
	if (devices == NULL) {
		wlr_log_errno(WLR_ERROR, "Allocation failed");
		return NULL;
	}
	devices_len = drmGetDevices2(flags, devices, devices_len);
	if (devices_len < 0) {
		free(devices);
		wlr_log(WLR_ERROR, "drmGetDevices2 failed: %s", strerror(-devices_len));
		return NULL;
	}

	const drmDevice *match = NULL;
	for (int i = 0; i < devices_len; i++) {
		if (device_has_name(devices[i], name)) {
			match = devices[i];
			break;
		}
	}

	char *render_name = NULL;
	if (match == NULL) {
		wlr_log(WLR_ERROR, "Cannot find DRM device %s", name);
	} else if (!(match->available_nodes & (1 << DRM_NODE_RENDER))) {
		// Likely a split display/render setup. Pick the primary node and hope
		// Mesa will open the right render node under-the-hood.
		wlr_log(WLR_DEBUG, "DRM device %s has no render node, "
			"falling back to primary node", name);
		assert(match->available_nodes & (1 << DRM_NODE_PRIMARY));
		render_name = strdup(match->nodes[DRM_NODE_PRIMARY]);
	} else {
		render_name = strdup(match->nodes[DRM_NODE_RENDER]);
	}

	for (int i = 0; i < devices_len; i++) {
		drmFreeDevice(&devices[i]);
	}
	free(devices);

	return render_name;
}

static int dup_egl_device_drm_fd(struct wlr_egl *egl) {
	if (egl->device == EGL_NO_DEVICE_EXT || (!egl->exts.EXT_device_drm &&
			!egl->exts.EXT_device_drm_render_node)) {
		return -1;
	}

	char *render_name = NULL;
#ifdef EGL_EXT_device_drm_render_node
	if (egl->exts.EXT_device_drm_render_node) {
		const char *name = egl->procs.eglQueryDeviceStringEXT(egl->device,
			EGL_DRM_RENDER_NODE_FILE_EXT);
		if (name == NULL) {
			wlr_log(WLR_DEBUG, "EGL device has no render node");
			return -1;
		}
		render_name = strdup(name);
	}
#endif

	if (render_name == NULL) {
		const char *primary_name = egl->procs.eglQueryDeviceStringEXT(egl->device,
			EGL_DRM_DEVICE_FILE_EXT);
		if (primary_name == NULL) {
			wlr_log(WLR_ERROR,
				"eglQueryDeviceStringEXT(EGL_DRM_DEVICE_FILE_EXT) failed");
			return -1;
		}

		render_name = get_render_name(primary_name);
		if (render_name == NULL) {
			wlr_log(WLR_ERROR, "Can't find render node name for device %s",
				primary_name);
			return -1;
		}
	}

	int render_fd = open(render_name, O_RDWR | O_NONBLOCK | O_CLOEXEC);
	if (render_fd < 0) {
		wlr_log_errno(WLR_ERROR, "Failed to open DRM render node %s",
			render_name);
		free(render_name);
		return -1;
	}
	free(render_name);

	return render_fd;
}

int wlr_egl_dup_drm_fd(struct wlr_egl *egl) {
	int fd = dup_egl_device_drm_fd(egl);
	if (fd >= 0) {
		return fd;
	}

	// Fallback to GBM's FD if we can't use EGLDevice
	if (egl->gbm_device == NULL) {
		return -1;
	}

	fd = fcntl(gbm_device_get_fd(egl->gbm_device), F_DUPFD_CLOEXEC, 0);
	if (fd < 0) {
		wlr_log_errno(WLR_ERROR, "Failed to dup GBM FD");
	}
	return fd;
}

EGLSyncKHR wlr_egl_create_sync(struct wlr_egl *egl, int fence_fd) {
	if (!egl->procs.eglCreateSyncKHR) {
		return EGL_NO_SYNC_KHR;
	}

	EGLint attribs[3] = { EGL_NONE };
	int dup_fd = -1;
	if (fence_fd >= 0) {
		dup_fd = fcntl(fence_fd, F_DUPFD_CLOEXEC, 0);
		if (dup_fd < 0) {
			wlr_log_errno(WLR_ERROR, "dup failed");
			return EGL_NO_SYNC_KHR;
		}

		attribs[0] = EGL_SYNC_NATIVE_FENCE_FD_ANDROID;
		attribs[1] = dup_fd;
		attribs[2] = EGL_NONE;
	}

	EGLSyncKHR sync = egl->procs.eglCreateSyncKHR(egl->display,
		EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
	if (sync == EGL_NO_SYNC_KHR) {
		wlr_log(WLR_ERROR, "eglCreateSyncKHR failed");
		if (dup_fd >= 0) {
			close(dup_fd);
		}
	}
	return sync;
}

void wlr_egl_destroy_sync(struct wlr_egl *egl, EGLSyncKHR sync) {
	if (sync == EGL_NO_SYNC_KHR) {
		return;
	}
	assert(egl->procs.eglDestroySyncKHR);
	if (egl->procs.eglDestroySyncKHR(egl->display, sync) != EGL_TRUE) {
		wlr_log(WLR_ERROR, "eglDestroySyncKHR failed");
	}
}

int wlr_egl_dup_fence_fd(struct wlr_egl *egl, EGLSyncKHR sync) {
	if (!egl->procs.eglDupNativeFenceFDANDROID) {
		return -1;
	}

	int fd = egl->procs.eglDupNativeFenceFDANDROID(egl->display, sync);
	if (fd == EGL_NO_NATIVE_FENCE_FD_ANDROID) {
		wlr_log(WLR_ERROR, "eglDupNativeFenceFDANDROID failed");
		return -1;
	}

	return fd;
}

bool wlr_egl_wait_sync(struct wlr_egl *egl, EGLSyncKHR sync) {
	if (egl->procs.eglWaitSyncKHR(egl->display, sync, 0) != EGL_TRUE) {
		wlr_log(WLR_ERROR, "eglWaitSyncKHR failed");
		return false;
	}
	return true;
}
