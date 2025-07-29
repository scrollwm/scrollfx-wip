#include <stdlib.h>
#include "sway/tree/scene.h"
#include <wlr/types/wlr_xdg_shell.h>

struct sway_scene_xdg_surface {
	struct sway_scene_tree *tree;
	struct wlr_xdg_surface *xdg_surface;
	struct sway_scene_tree *surface_tree;

	struct wl_listener tree_destroy;
	struct wl_listener xdg_surface_destroy;
	struct wl_listener xdg_surface_commit;
};

static void scene_xdg_surface_handle_tree_destroy(struct wl_listener *listener,
		void *data) {
	struct sway_scene_xdg_surface *scene_xdg_surface =
		wl_container_of(listener, scene_xdg_surface, tree_destroy);
	// tree and surface_node will be cleaned up by scene_node_finish
	wl_list_remove(&scene_xdg_surface->tree_destroy.link);
	wl_list_remove(&scene_xdg_surface->xdg_surface_destroy.link);
	wl_list_remove(&scene_xdg_surface->xdg_surface_commit.link);
	free(scene_xdg_surface);
}

static void scene_xdg_surface_handle_xdg_surface_destroy(struct wl_listener *listener,
		void *data) {
	struct sway_scene_xdg_surface *scene_xdg_surface =
		wl_container_of(listener, scene_xdg_surface, xdg_surface_destroy);
	sway_scene_node_destroy(&scene_xdg_surface->tree->node);
}

static void scene_xdg_surface_update_position(
		struct sway_scene_xdg_surface *scene_xdg_surface) {
	struct wlr_xdg_surface *xdg_surface = scene_xdg_surface->xdg_surface;

	sway_scene_node_set_position(&scene_xdg_surface->surface_tree->node,
		-xdg_surface->geometry.x, -xdg_surface->geometry.y);

	if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
		struct wlr_xdg_popup *popup = xdg_surface->popup;
		if (popup != NULL) {
			sway_scene_node_set_position(&scene_xdg_surface->tree->node,
				popup->current.geometry.x, popup->current.geometry.y);
		}
	}
}

static void scene_xdg_surface_handle_xdg_surface_commit(struct wl_listener *listener,
		void *data) {
	struct sway_scene_xdg_surface *scene_xdg_surface =
		wl_container_of(listener, scene_xdg_surface, xdg_surface_commit);
	scene_xdg_surface_update_position(scene_xdg_surface);
}

struct sway_scene_tree *sway_scene_xdg_surface_create(
		struct sway_scene_tree *parent, struct wlr_xdg_surface *xdg_surface) {
	struct sway_scene_xdg_surface *scene_xdg_surface =
		calloc(1, sizeof(*scene_xdg_surface));
	if (scene_xdg_surface == NULL) {
		return NULL;
	}

	scene_xdg_surface->xdg_surface = xdg_surface;

	scene_xdg_surface->tree = sway_scene_tree_create(parent);
	if (scene_xdg_surface->tree == NULL) {
		free(scene_xdg_surface);
		return NULL;
	}

	scene_xdg_surface->surface_tree = sway_scene_subsurface_tree_create(
		scene_xdg_surface->tree, xdg_surface->surface);
	if (scene_xdg_surface->surface_tree == NULL) {
		sway_scene_node_destroy(&scene_xdg_surface->tree->node);
		free(scene_xdg_surface);
		return NULL;
	}

	scene_xdg_surface->tree_destroy.notify =
		scene_xdg_surface_handle_tree_destroy;
	wl_signal_add(&scene_xdg_surface->tree->node.events.destroy,
		&scene_xdg_surface->tree_destroy);

	scene_xdg_surface->xdg_surface_destroy.notify =
		scene_xdg_surface_handle_xdg_surface_destroy;
	wl_signal_add(&xdg_surface->events.destroy, &scene_xdg_surface->xdg_surface_destroy);

	scene_xdg_surface->xdg_surface_commit.notify =
		scene_xdg_surface_handle_xdg_surface_commit;
	wl_signal_add(&xdg_surface->surface->events.commit,
		&scene_xdg_surface->xdg_surface_commit);

	scene_xdg_surface_update_position(scene_xdg_surface);

	return scene_xdg_surface->tree;
}
