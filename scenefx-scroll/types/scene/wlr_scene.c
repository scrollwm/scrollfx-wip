#include <assert.h>
#include <pixman.h>
#include <stdio.h>
#include <stdlib.h>
#include <scenefx/types/sway_scene.h>
#include <string.h>
#include <wlr/backend.h>
#include <wlr/render/gles2.h>
#include <wlr/render/drm_syncobj.h>
#include <wlr/render/swapchain.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_damage_ring.h>
#include <wlr/types/wlr_gamma_control_v1.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wlr/util/region.h>
#include <wlr/util/transform.h>

#include "scenefx/render/fx_renderer/fx_effect_framebuffers.h"
#include "scenefx/render/fx_renderer/fx_renderer.h"
#include "scenefx/render/pass.h"
#include "scenefx/types/fx/blur_data.h"
#include "scenefx/types/fx/clipped_region.h"
#include "scenefx/types/fx/corner_location.h"
#include "types/wlr_output.h"
#include "scenefx/types/sway_scene.h"
#include "util/array.h"
#include "util/env.h"
#include "util/time.h"
#include "wlr/util/box.h"

#include <wlr/config.h>

#if WLR_HAS_XWAYLAND
#include <wlr/xwayland/xwayland.h>
#endif

#define DMABUF_FEEDBACK_DEBOUNCE_FRAMES  30
#define HIGHLIGHT_DAMAGE_FADEOUT_TIME   250

#define SCENE_BUFFER_SHOULD_BLUR(scene_buffer, blur_data) \
	scene_buffer->backdrop_blur && is_scene_blur_enabled(blur_data) && \
		(!scene_buffer->buffer_is_opaque || scene_buffer->opacity < 1.0f)
#define SCENE_RECT_SHOULD_BLUR(scene_rect, blur_data) \
	scene_rect->backdrop_blur && is_scene_blur_enabled(blur_data) && \
		scene_rect->color[3] < 1.0f

struct sway_scene_tree *sway_scene_tree_from_node(struct sway_scene_node *node) {
	assert(node->type == SWAY_SCENE_NODE_TREE);
	struct sway_scene_tree *tree = wl_container_of(node, tree, node);
	return tree;
}

struct sway_scene_rect *sway_scene_rect_from_node(struct sway_scene_node *node) {
	assert(node->type == SWAY_SCENE_NODE_RECT);
	struct sway_scene_rect *rect = wl_container_of(node, rect, node);
	return rect;
}

struct sway_scene_optimized_blur *sway_scene_optimized_blur_from_node(
		struct sway_scene_node *node) {
	assert(node->type == SWAY_SCENE_NODE_OPTIMIZED_BLUR);
	struct sway_scene_optimized_blur *blur_node =
		wl_container_of(node, blur_node, node);
	return blur_node;
}

struct sway_scene_buffer *sway_scene_buffer_from_node(
		struct sway_scene_node *node) {
	assert(node->type == SWAY_SCENE_NODE_BUFFER);
	struct sway_scene_buffer *buffer = wl_container_of(node, buffer, node);
	return buffer;
}

struct sway_scene_shadow *sway_scene_shadow_from_node(struct sway_scene_node *node) {
	assert(node->type == SWAY_SCENE_NODE_SHADOW);
	struct sway_scene_shadow *shadow = wl_container_of(node, shadow, node);
	return shadow;
}

struct sway_scene *scene_node_get_root(struct sway_scene_node *node) {
	struct sway_scene_tree *tree;
	if (node->type == SWAY_SCENE_NODE_TREE) {
		tree = sway_scene_tree_from_node(node);
	} else {
		tree = node->parent;
	}

	while (tree->node.parent != NULL) {
		tree = tree->node.parent;
	}
	struct sway_scene *scene = wl_container_of(tree, scene, tree);
	return scene;
}

static void scene_node_init(struct sway_scene_node *node,
		enum sway_scene_node_type type, struct sway_scene_tree *parent) {
	*node = (struct sway_scene_node){
		.type = type,
		.parent = parent,
		.enabled = true,
	};

	wl_list_init(&node->link);

	wl_signal_init(&node->events.destroy);
	pixman_region32_init(&node->visible);

	if (parent != NULL) {
		wl_list_insert(parent->children.prev, &node->link);
	}

	wlr_addon_set_init(&node->addons);
}

struct highlight_region {
	pixman_region32_t region;
	struct timespec when;
	struct wl_list link;
};

static void scene_buffer_set_buffer(struct sway_scene_buffer *scene_buffer,
	struct wlr_buffer *buffer);
static void scene_buffer_set_texture(struct sway_scene_buffer *scene_buffer,
	struct wlr_texture *texture);

void sway_scene_node_destroy(struct sway_scene_node *node) {
	if (node == NULL) {
		return;
	}

	// We want to call the destroy listeners before we do anything else
	// in case the destroy signal would like to remove children before they
	// are recursively destroyed.
	wl_signal_emit_mutable(&node->events.destroy, NULL);
	wlr_addon_set_finish(&node->addons);

	sway_scene_node_set_enabled(node, false);

	struct sway_scene *scene = scene_node_get_root(node);
	if (node->type == SWAY_SCENE_NODE_BUFFER) {
		struct sway_scene_buffer *scene_buffer = sway_scene_buffer_from_node(node);

		uint64_t active = scene_buffer->active_outputs;
		if (active) {
			struct sway_scene_output *scene_output;
			wl_list_for_each(scene_output, &scene->outputs, link) {
				if (active & (1ull << scene_output->index)) {
					wl_signal_emit_mutable(&scene_buffer->events.output_leave,
						scene_output);
				}
			}
		}

		scene_buffer_set_buffer(scene_buffer, NULL);
		scene_buffer_set_texture(scene_buffer, NULL);
		pixman_region32_fini(&scene_buffer->opaque_region);
		wlr_drm_syncobj_timeline_unref(scene_buffer->wait_timeline);

		assert(wl_list_empty(&scene_buffer->events.output_leave.listener_list));
		assert(wl_list_empty(&scene_buffer->events.output_enter.listener_list));
		assert(wl_list_empty(&scene_buffer->events.outputs_update.listener_list));
		assert(wl_list_empty(&scene_buffer->events.output_sample.listener_list));
		assert(wl_list_empty(&scene_buffer->events.frame_done.listener_list));
	} else if (node->type == SWAY_SCENE_NODE_TREE) {
		struct sway_scene_tree *scene_tree = sway_scene_tree_from_node(node);

		if (scene_tree == &scene->tree) {
			assert(!node->parent);
			struct sway_scene_output *scene_output, *scene_output_tmp;
			wl_list_for_each_safe(scene_output, scene_output_tmp, &scene->outputs, link) {
				sway_scene_output_destroy(scene_output);
			}

			wl_list_remove(&scene->linux_dmabuf_v1_destroy.link);
			wl_list_remove(&scene->gamma_control_manager_v1_destroy.link);
			wl_list_remove(&scene->gamma_control_manager_v1_set_gamma.link);
		} else {
			assert(node->parent);
		}

		struct sway_scene_node *child, *child_tmp;
		wl_list_for_each_safe(child, child_tmp,
				&scene_tree->children, link) {
			sway_scene_node_destroy(child);
		}
	}

	assert(wl_list_empty(&node->events.destroy.listener_list));

	wl_list_remove(&node->link);
	pixman_region32_fini(&node->visible);
	free(node);
}

static void scene_tree_init(struct sway_scene_tree *tree,
		struct sway_scene_tree *parent) {
	*tree = (struct sway_scene_tree){0};
	scene_node_init(&tree->node, SWAY_SCENE_NODE_TREE, parent);
	wl_list_init(&tree->children);
}

struct sway_scene *sway_scene_create(void) {
	struct sway_scene *scene = calloc(1, sizeof(*scene));
	if (scene == NULL) {
		return NULL;
	}

	scene_tree_init(&scene->tree, NULL);

	wl_list_init(&scene->outputs);
	wl_list_init(&scene->linux_dmabuf_v1_destroy.link);
	wl_list_init(&scene->gamma_control_manager_v1_destroy.link);
	wl_list_init(&scene->gamma_control_manager_v1_set_gamma.link);

	const char *debug_damage_options[] = {
		"none",
		"rerender",
		"highlight",
		NULL
	};

	scene->debug_damage_option = env_parse_switch("WLR_SCENE_DEBUG_DAMAGE", debug_damage_options);
	scene->direct_scanout = !env_parse_bool("WLR_SCENE_DISABLE_DIRECT_SCANOUT");
	scene->calculate_visibility = !env_parse_bool("WLR_SCENE_DISABLE_VISIBILITY");
	scene->highlight_transparent_region = env_parse_bool("WLR_SCENE_HIGHLIGHT_TRANSPARENT_REGION");

	scene->blur_data = blur_data_get_default();

	return scene;
}

struct sway_scene_tree *sway_scene_tree_create(struct sway_scene_tree *parent) {
	assert(parent);

	struct sway_scene_tree *tree = calloc(1, sizeof(*tree));
	if (tree == NULL) {
		return NULL;
	}

	scene_tree_init(tree, parent);
	return tree;
}

static void scene_node_get_size(struct sway_scene_node *node, int *lx, int *ly);

typedef bool (*scene_node_box_iterator_func_t)(struct sway_scene_node *node,
	int sx, int sy, void *data);

static bool _scene_nodes_in_box(struct sway_scene_node *node, struct wlr_box *box,
		scene_node_box_iterator_func_t iterator, void *user_data, int lx, int ly) {
	if (!node->enabled) {
		return false;
	}

	switch (node->type) {
	case SWAY_SCENE_NODE_TREE:;
		struct sway_scene_tree *scene_tree = sway_scene_tree_from_node(node);
		struct sway_scene_node *child;
		wl_list_for_each_reverse(child, &scene_tree->children, link) {
			if (_scene_nodes_in_box(child, box, iterator, user_data, lx + child->x, ly + child->y)) {
				return true;
			}
		}
		break;
	case SWAY_SCENE_NODE_OPTIMIZED_BLUR:;
	case SWAY_SCENE_NODE_RECT:
	case SWAY_SCENE_NODE_SHADOW:
	case SWAY_SCENE_NODE_BUFFER:;
		struct wlr_box node_box = { .x = lx, .y = ly };
		scene_node_get_size(node, &node_box.width, &node_box.height);

		if (wlr_box_intersection(&node_box, &node_box, box) &&
				iterator(node, lx, ly, user_data)) {
			return true;
		}
		break;
	}

	return false;
}

static bool scene_nodes_in_box(struct sway_scene_node *node, struct wlr_box *box,
		scene_node_box_iterator_func_t iterator, void *user_data) {
	int x, y;
	sway_scene_node_coords(node, &x, &y);

	return _scene_nodes_in_box(node, box, iterator, user_data, x, y);
}

static void scene_node_opaque_region(struct sway_scene_node *node, int x, int y,
		pixman_region32_t *opaque) {
	int width, height;
	scene_node_get_size(node, &width, &height);

	if (node->type == SWAY_SCENE_NODE_RECT) {
		struct sway_scene_rect *scene_rect = sway_scene_rect_from_node(node);
		if (scene_rect->corner_radius > 0) {
			// TODO: this is incorrect
			return;
		}
		if (scene_rect->color[3] != 1) {
			return;
		}
		if (!wlr_box_empty(&scene_rect->clipped_region.area)) {
			pixman_region32_fini(opaque);
			pixman_region32_init_rect(opaque, x, y, width, height);

			// Subtract the clipped region from a otherwise fully opaque rect
			struct wlr_box *clipped = &scene_rect->clipped_region.area;
			pixman_region32_t clipped_region;
			pixman_region32_init_rect(&clipped_region, clipped->x + x, clipped->y + y,
					clipped->width, clipped->height);
			pixman_region32_subtract(opaque, opaque, &clipped_region);
			pixman_region32_fini(&clipped_region);
			return;
		}
	} else if (node->type == SWAY_SCENE_NODE_SHADOW) {
		// TODO: test & handle case of blur sigma = 0 and color[3] = 1?
		return;
	} else if (node->type == SWAY_SCENE_NODE_BUFFER) {
		struct sway_scene_buffer *scene_buffer = sway_scene_buffer_from_node(node);

		if (!scene_buffer->buffer) {
			return;
		}

		if (scene_buffer->opacity != 1) {
			return;
		}

		if (scene_buffer->corner_radius > 0) {
			// TODO: this is incorrect
			return;
		}

		if (!scene_buffer->buffer_is_opaque) {
			pixman_region32_copy(opaque, &scene_buffer->opaque_region);
			pixman_region32_intersect_rect(opaque, opaque, 0, 0, width, height);
			pixman_region32_translate(opaque, x, y);
			return;
		}
	} else if (node->type == SWAY_SCENE_NODE_OPTIMIZED_BLUR) {
		// Always transparent
		return;
	}

	pixman_region32_fini(opaque);
	pixman_region32_init_rect(opaque, x, y, width, height);
}

struct scene_update_data {
	pixman_region32_t *visible;
	pixman_region32_t *update_region;
	struct wlr_box update_box;
	struct wl_list *outputs;
	bool calculate_visibility;

#if WLR_HAS_XWAYLAND
	struct wlr_xwayland_surface *restack_above;
#endif

	bool optimized_blur_dirty;
	struct blur_data *blur_data;
};

static uint32_t region_area(pixman_region32_t *region) {
	uint32_t area = 0;

	int nrects;
	pixman_box32_t *rects = pixman_region32_rectangles(region, &nrects);
	for (int i = 0; i < nrects; ++i) {
		area += (rects[i].x2 - rects[i].x1) * (rects[i].y2 - rects[i].y1);
	}

	return area;
}

static void scale_region(pixman_region32_t *region, float scale, bool round_up) {
	wlr_region_scale(region, region, scale);

	if (round_up && floor(scale) != scale) {
		wlr_region_expand(region, region, 1);
	}
}

struct render_data {
	enum wl_output_transform transform;
	float scale;
	struct wlr_box logical;
	int trans_width, trans_height;

	struct sway_scene_output *output;

	struct fx_gles_render_pass *render_pass;
	pixman_region32_t damage;

	bool has_blur;
};

// PLACEHOLDER - rest of file needs to be converted