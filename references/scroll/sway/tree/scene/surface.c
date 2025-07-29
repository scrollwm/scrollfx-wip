#include <assert.h>
#include <stdlib.h>
#include <wlr/types/wlr_alpha_modifier_v1.h>
#include <wlr/types/wlr_color_management_v1.h>
#include "sway/tree/scene.h"
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_fractional_scale_v1.h>
#include <wlr/types/wlr_linux_drm_syncobj_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/util/transform.h>

static double get_surface_preferred_buffer_scale(struct wlr_surface *surface) {
	double scale = 1;
	struct wlr_surface_output *surface_output;
	wl_list_for_each(surface_output, &surface->current_outputs, link) {
		if (surface_output->output->scale > scale) {
			scale = surface_output->output->scale;
		}
	}
	return scale;
}

static struct wlr_output *get_surface_frame_pacing_output(struct wlr_surface *surface) {
	struct wlr_output *frame_pacing_output = NULL;
	struct wlr_surface_output *surface_output;
	wl_list_for_each(surface_output, &surface->current_outputs, link) {
		if (frame_pacing_output == NULL ||
				surface_output->output->refresh > frame_pacing_output->refresh) {
			frame_pacing_output = surface_output->output;
		}
	}
	return frame_pacing_output;
}

static void handle_scene_buffer_outputs_update(
		struct wl_listener *listener, void *data) {
	struct sway_scene_surface *surface =
		wl_container_of(listener, surface, outputs_update);

	surface->frame_pacing_output = get_surface_frame_pacing_output(surface->surface);

	double scale = get_surface_preferred_buffer_scale(surface->surface);
	wlr_fractional_scale_v1_notify_scale(surface->surface, scale);
	wlr_surface_set_preferred_buffer_scale(surface->surface, ceil(scale));
}

static void handle_scene_buffer_output_enter(
		struct wl_listener *listener, void *data) {
	struct sway_scene_surface *surface =
		wl_container_of(listener, surface, output_enter);
	struct sway_scene_output *output = data;

	wlr_surface_send_enter(surface->surface, output->output);
}

static void handle_scene_buffer_output_leave(
		struct wl_listener *listener, void *data) {
	struct sway_scene_surface *surface =
		wl_container_of(listener, surface, output_leave);
	struct sway_scene_output *output = data;

	wlr_surface_send_leave(surface->surface, output->output);
}

static void handle_scene_buffer_output_sample(
		struct wl_listener *listener, void *data) {
	struct sway_scene_surface *surface =
		wl_container_of(listener, surface, output_sample);
	const struct sway_scene_output_sample_event *event = data;
	struct wlr_output *output = event->output->output;
	if (surface->frame_pacing_output != output) {
		return;
	}

	if (event->direct_scanout) {
		wlr_presentation_surface_scanned_out_on_output(surface->surface, output);
	} else {
		wlr_presentation_surface_textured_on_output(surface->surface, output);
	}
}

static void handle_scene_buffer_frame_done(
		struct wl_listener *listener, void *data) {
	struct sway_scene_surface *surface =
		wl_container_of(listener, surface, frame_done);
	struct sway_scene_frame_done_event *event = data;
	if (surface->frame_pacing_output != event->output->output) {
		return;
	}

	wlr_surface_send_frame_done(surface->surface, &event->when);
}

void sway_scene_surface_send_frame_done(struct sway_scene_surface *scene_surface,
		const struct timespec *when) {
	if (!pixman_region32_empty(&scene_surface->buffer->node.visible)) {
		wlr_surface_send_frame_done(scene_surface->surface, when);
	}
}

static void scene_surface_handle_surface_destroy(
		struct wl_listener *listener, void *data) {
	struct sway_scene_surface *surface =
		wl_container_of(listener, surface, surface_destroy);

	sway_scene_node_destroy(&surface->buffer->node);
}

// This is used for sway_scene where it unconditionally locks buffers preventing
// reuse of the existing texture for shm clients. With the usage pattern of
// sway_scene surface handling, we can mark its locked buffer as safe
// for mutation.
static void client_buffer_mark_next_can_damage(struct wlr_client_buffer *buffer) {
	buffer->WLR_PRIVATE.n_ignore_locks++;
}

static void scene_buffer_unmark_client_buffer(struct sway_scene_buffer *scene_buffer) {
	if (!scene_buffer->buffer) {
		return;
	}

	struct wlr_client_buffer *buffer = wlr_client_buffer_get(scene_buffer->buffer);
	if (!buffer) {
		return;
	}

	// If the buffer was a single-pixel buffer where we cached its color
	// then it won't have been marked as damage-allowed.
	if (buffer->WLR_PRIVATE.n_ignore_locks > 0) {
		buffer->WLR_PRIVATE.n_ignore_locks--;
	}
}

static int min(int a, int b) {
	return a < b ? a : b;
}

static void surface_reconfigure(struct sway_scene_surface *scene_surface) {
	struct sway_scene_buffer *scene_buffer = scene_surface->buffer;
	struct wlr_surface *surface = scene_surface->surface;
	struct wlr_surface_state *state = &surface->current;

	struct wlr_fbox src_box;
	wlr_surface_get_buffer_source_box(surface, &src_box);

	pixman_region32_t opaque;
	pixman_region32_init(&opaque);
	pixman_region32_copy(&opaque, &surface->opaque_region);

	int width = state->width;
	int height = state->height;

	if (!wlr_box_empty(&scene_surface->clip)) {
		struct wlr_box *clip = &scene_surface->clip;

		int buffer_width = state->buffer_width;
		int buffer_height = state->buffer_height;
		width = min(clip->width, width - clip->x);
		height = min(clip->height, height - clip->y);

		wlr_fbox_transform(&src_box, &src_box, state->transform,
			buffer_width, buffer_height);
		wlr_output_transform_coords(state->transform, &buffer_width, &buffer_height);

		src_box.x += (double)(clip->x * src_box.width) / state->width;
		src_box.y += (double)(clip->y * src_box.height) / state->height;
		src_box.width *= (double)width / state->width;
		src_box.height *= (double)height / state->height;

		wlr_fbox_transform(&src_box, &src_box, wlr_output_transform_invert(state->transform),
			buffer_width, buffer_height);

		pixman_region32_translate(&opaque, -clip->x, -clip->y);
		pixman_region32_intersect_rect(&opaque, &opaque, 0, 0, width, height);
	}

	if (width <= 0 || height <= 0 || (src_box.width <= 1.0 && src_box.height <= 1.0)) {
		sway_scene_buffer_set_buffer(scene_buffer, NULL);
		pixman_region32_fini(&opaque);
		return;
	}

	float opacity = 1.0;
	const struct wlr_alpha_modifier_surface_v1_state *alpha_modifier_state =
		wlr_alpha_modifier_v1_get_surface_state(surface);
	if (alpha_modifier_state != NULL) {
		opacity = (float)alpha_modifier_state->multiplier;
	}

	enum wlr_color_transfer_function tf = WLR_COLOR_TRANSFER_FUNCTION_SRGB;
	enum wlr_color_named_primaries primaries = WLR_COLOR_NAMED_PRIMARIES_SRGB;
	const struct wlr_image_description_v1_data *img_desc =
		wlr_surface_get_image_description_v1_data(surface);
	if (img_desc != NULL) {
		tf = wlr_color_manager_v1_transfer_function_to_wlr(img_desc->tf_named);
		primaries = wlr_color_manager_v1_primaries_to_wlr(img_desc->primaries_named); 
	}

	sway_scene_buffer_set_opaque_region(scene_buffer, &opaque);
	sway_scene_buffer_set_source_box(scene_buffer, &src_box);
	double total_scale = 1.0f;
	double content_scale = scene_node_get_parent_content_scale(&scene_buffer->node);
	double scale = scene_node_get_parent_scale(&scene_buffer->node);
	if (content_scale > 0.0f) {
		total_scale *= content_scale;
	}
	if (scale > 0.0f) {
		total_scale *= scale;
	}
	sway_scene_buffer_set_dest_size(scene_buffer, round(width * total_scale), round(height * total_scale));
	sway_scene_buffer_set_transform(scene_buffer, state->transform);
	sway_scene_buffer_set_opacity(scene_buffer, opacity);
	sway_scene_buffer_set_transfer_function(scene_buffer, tf);
	sway_scene_buffer_set_primaries(scene_buffer, primaries);

	scene_buffer_unmark_client_buffer(scene_buffer);

	if (surface->buffer) {
		// If we've cached the buffer's single-pixel buffer color
		// then any in-place updates to the texture wouldn't be
		// reflected in rendering. So only allow in-place texture
		// updates if it's not a single pixel buffer.  Note that we
		// can't use the cached scene_buffer->is_single_pixel_buffer
		// because that's only set later on.
		bool is_single_pixel_buffer = false;
		struct wlr_client_buffer *client_buffer = wlr_client_buffer_get(&surface->buffer->base);
		if (client_buffer != NULL && client_buffer->source != NULL) {
			struct wlr_single_pixel_buffer_v1 *spb =
				wlr_single_pixel_buffer_v1_try_from_buffer(client_buffer->source);
			is_single_pixel_buffer = spb != NULL;
		}
		if (!is_single_pixel_buffer) {
			client_buffer_mark_next_can_damage(surface->buffer);
		}

		struct wlr_linux_drm_syncobj_surface_v1_state *syncobj_surface_state =
			wlr_linux_drm_syncobj_v1_get_surface_state(surface);

		struct wlr_drm_syncobj_timeline *wait_timeline = NULL;
		uint64_t wait_point = 0;
		if (syncobj_surface_state != NULL) {
			wait_timeline = syncobj_surface_state->acquire_timeline;
			wait_point = syncobj_surface_state->acquire_point;
		}

		struct sway_scene_buffer_set_buffer_options options = {
			.damage = &surface->buffer_damage,
			.wait_timeline = wait_timeline,
			.wait_point = wait_point,
		};
		sway_scene_buffer_set_buffer_with_options(scene_buffer,
			&surface->buffer->base, &options);

		if (syncobj_surface_state != NULL &&
				(surface->current.committed & WLR_SURFACE_STATE_BUFFER)) {
			wlr_linux_drm_syncobj_v1_state_signal_release_with_buffer(syncobj_surface_state,
				surface->buffer->source);
		}
	} else {
		sway_scene_buffer_set_buffer(scene_buffer, NULL);
	}

	pixman_region32_fini(&opaque);
}

static void handle_scene_surface_surface_commit(
		struct wl_listener *listener, void *data) {
	struct sway_scene_surface *surface =
		wl_container_of(listener, surface, surface_commit);
	struct sway_scene_buffer *scene_buffer = surface->buffer;

	surface_reconfigure(surface);

	// If the surface has requested a frame done event, honour that. The
	// frame_callback_list will be populated in this case. We should only
	// schedule the frame however if the node is enabled and there is an
	// output intersecting, otherwise the frame done events would never reach
	// the surface anyway.
	int lx, ly;
	bool enabled = sway_scene_node_coords(&scene_buffer->node, &lx, &ly);

	if (!wl_list_empty(&surface->surface->current.frame_callback_list) &&
			surface->buffer->primary_output != NULL && enabled) {
		wlr_output_schedule_frame(surface->buffer->primary_output->output);
	}
}

static bool scene_buffer_point_accepts_input(struct sway_scene_buffer *scene_buffer,
		double *sx, double *sy) {
	struct sway_scene_surface *scene_surface =
		sway_scene_surface_try_from_buffer(scene_buffer);

	*sx += scene_surface->clip.x;
	*sy += scene_surface->clip.y;

	return wlr_surface_point_accepts_input(scene_surface->surface, *sx, *sy);
}

static void surface_addon_destroy(struct wlr_addon *addon) {
	struct sway_scene_surface *surface = wl_container_of(addon, surface, addon);

	scene_buffer_unmark_client_buffer(surface->buffer);

	wlr_addon_finish(&surface->addon);

	wl_list_remove(&surface->outputs_update.link);
	wl_list_remove(&surface->output_enter.link);
	wl_list_remove(&surface->output_leave.link);
	wl_list_remove(&surface->output_sample.link);
	wl_list_remove(&surface->frame_done.link);
	wl_list_remove(&surface->surface_destroy.link);
	wl_list_remove(&surface->surface_commit.link);

	free(surface);
}

static const struct wlr_addon_interface surface_addon_impl = {
	.name = "sway_scene_surface",
	.destroy = surface_addon_destroy,
};

struct sway_scene_surface *sway_scene_surface_try_from_buffer(
		struct sway_scene_buffer *scene_buffer) {
	struct wlr_addon *addon = wlr_addon_find(&scene_buffer->node.addons,
		scene_buffer, &surface_addon_impl);
	if (!addon) {
		return NULL;
	}

	struct sway_scene_surface *surface = wl_container_of(addon, surface, addon);
	return surface;
}

struct sway_scene_surface *sway_scene_surface_create(struct sway_scene_tree *parent,
		struct wlr_surface *wlr_surface) {
	struct sway_scene_surface *surface = calloc(1, sizeof(*surface));
	if (surface == NULL) {
		return NULL;
	}

	struct sway_scene_buffer *scene_buffer = sway_scene_buffer_create(parent, NULL);
	if (!scene_buffer) {
		free(surface);
		return NULL;
	}

	surface->buffer = scene_buffer;
	surface->surface = wlr_surface;
	scene_buffer->point_accepts_input = scene_buffer_point_accepts_input;

	surface->outputs_update.notify = handle_scene_buffer_outputs_update;
	wl_signal_add(&scene_buffer->events.outputs_update, &surface->outputs_update);

	surface->output_enter.notify = handle_scene_buffer_output_enter;
	wl_signal_add(&scene_buffer->events.output_enter, &surface->output_enter);

	surface->output_leave.notify = handle_scene_buffer_output_leave;
	wl_signal_add(&scene_buffer->events.output_leave, &surface->output_leave);

	surface->output_sample.notify = handle_scene_buffer_output_sample;
	wl_signal_add(&scene_buffer->events.output_sample, &surface->output_sample);

	surface->frame_done.notify = handle_scene_buffer_frame_done;
	wl_signal_add(&scene_buffer->events.frame_done, &surface->frame_done);

	surface->surface_destroy.notify = scene_surface_handle_surface_destroy;
	wl_signal_add(&wlr_surface->events.destroy, &surface->surface_destroy);

	surface->surface_commit.notify = handle_scene_surface_surface_commit;
	wl_signal_add(&wlr_surface->events.commit, &surface->surface_commit);

	wlr_addon_init(&surface->addon, &scene_buffer->node.addons,
		scene_buffer, &surface_addon_impl);

	surface_reconfigure(surface);

	return surface;
}

void scene_surface_set_clip(struct sway_scene_surface *surface, struct wlr_box *clip) {
	if (wlr_box_equal(clip, &surface->clip)) {
		return;
	}

	if (clip) {
		surface->clip = *clip;
	} else {
		surface->clip = (struct wlr_box){0};
	}

	surface_reconfigure(surface);
}
