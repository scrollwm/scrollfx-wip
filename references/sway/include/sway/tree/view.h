#ifndef _SWAY_VIEW_H
#define _SWAY_VIEW_H
#include <wayland-server-core.h>
#include <wlr/config.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_tearing_control_v1.h>
#include "sway/config.h"
#if WLR_HAS_XWAYLAND
#include <wlr/xwayland.h>
#endif
#include "sway/input/input-manager.h"
#include "sway/input/seat.h"

struct sway_container;
struct sway_xdg_decoration;

enum sway_view_type {
	SWAY_VIEW_XDG_SHELL,
#if WLR_HAS_XWAYLAND
	SWAY_VIEW_XWAYLAND,
#endif
};

enum sway_view_prop {
	VIEW_PROP_TITLE,
	VIEW_PROP_APP_ID,
	VIEW_PROP_TAG,
	VIEW_PROP_CLASS,
	VIEW_PROP_INSTANCE,
	VIEW_PROP_WINDOW_TYPE,
	VIEW_PROP_WINDOW_ROLE,
#if WLR_HAS_XWAYLAND
	VIEW_PROP_X11_WINDOW_ID,
	VIEW_PROP_X11_PARENT_ID,
#endif
};

enum sway_view_tearing_mode {
	TEARING_OVERRIDE_FALSE,
	TEARING_OVERRIDE_TRUE,
	TEARING_WINDOW_HINT,
};

struct sway_view_impl {
	void (*get_constraints)(struct sway_view *view, double *min_width,
			double *max_width, double *min_height, double *max_height);
	const char *(*get_string_prop)(struct sway_view *view,
			enum sway_view_prop prop);
	uint32_t (*get_int_prop)(struct sway_view *view, enum sway_view_prop prop);
	uint32_t (*configure)(struct sway_view *view, double lx, double ly,
			int width, int height);
	void (*set_activated)(struct sway_view *view, bool activated);
	void (*set_tiled)(struct sway_view *view, bool tiled);
	void (*set_fullscreen)(struct sway_view *view, bool fullscreen);
	void (*set_resizing)(struct sway_view *view, bool resizing);
	bool (*wants_floating)(struct sway_view *view);
	bool (*is_transient_for)(struct sway_view *child,
			struct sway_view *ancestor);
	void (*close)(struct sway_view *view);
	void (*close_popups)(struct sway_view *view);
	void (*destroy)(struct sway_view *view);
};

struct sway_view {
	enum sway_view_type type;
	const struct sway_view_impl *impl;

	struct wlr_scene_tree *scene_tree;
	struct wlr_scene_tree *content_tree;
	struct wlr_scene_tree *saved_surface_tree;

	struct wlr_scene *image_capture_scene;
	struct wlr_ext_image_capture_source_v1 *image_capture_source;

	struct sway_container *container; // NULL if unmapped and transactions finished
	struct wlr_surface *surface; // NULL for unmapped views
	struct sway_xdg_decoration *xdg_decoration;

	pid_t pid;
	struct launcher_ctx *ctx;

	// The size the view would want to be if it weren't tiled.
	// Used when changing a view from tiled to floating.
	int natural_width, natural_height;

	bool using_csd;

	struct timespec urgent;
	bool allow_request_urgent;
	struct wl_event_source *urgent_timer;

	// The geometry for whatever the client is committing, regardless of
	// transaction state. Updated on every commit.
	struct wlr_box geometry;

	struct wlr_ext_foreign_toplevel_handle_v1 *ext_foreign_toplevel;

	struct wlr_foreign_toplevel_handle_v1 *foreign_toplevel;
	struct wl_listener foreign_activate_request;
	struct wl_listener foreign_fullscreen_request;
	struct wl_listener foreign_close_request;
	struct wl_listener foreign_destroy;

	bool destroying;

	list_t *executed_criteria; // struct criteria *

	union {
		struct wlr_xdg_toplevel *wlr_xdg_toplevel;
#if WLR_HAS_XWAYLAND
		struct wlr_xwayland_surface *wlr_xwayland_surface;
#endif
	};

	struct {
		struct wl_signal unmap;
	} events;

	int max_render_time; // In milliseconds

	enum seat_config_shortcuts_inhibit shortcuts_inhibit;

	enum sway_view_tearing_mode tearing_mode;
	enum wp_tearing_control_v1_presentation_hint tearing_hint;
};

struct sway_xdg_shell_view {
	struct sway_view view;

	struct wlr_scene_tree *image_capture_tree;
	char *tag;

	struct wl_listener commit;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_maximize;
	struct wl_listener request_fullscreen;
	struct wl_listener set_title;
	struct wl_listener set_app_id;
	struct wl_listener new_popup;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
};
#if WLR_HAS_XWAYLAND
struct sway_xwayland_view {
	struct sway_view view;

	struct wlr_scene_tree *surface_tree;

	struct wlr_scene_surface *image_capture_scene_surface;

	struct wl_listener commit;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_maximize;
	struct wl_listener request_minimize;
	struct wl_listener request_configure;
	struct wl_listener request_fullscreen;
	struct wl_listener request_activate;
	struct wl_listener set_title;
	struct wl_listener set_class;
	struct wl_listener set_role;
	struct wl_listener set_startup_id;
	struct wl_listener set_window_type;
	struct wl_listener set_hints;
	struct wl_listener set_decorations;
	struct wl_listener associate;
	struct wl_listener dissociate;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener override_redirect;

	struct wl_listener surface_tree_destroy;
};

struct sway_xwayland_unmanaged {
	struct wlr_xwayland_surface *wlr_xwayland_surface;

	struct wlr_scene_surface *surface_scene;

	struct wl_listener request_activate;
	struct wl_listener request_configure;
	struct wl_listener request_fullscreen;
	struct wl_listener set_geometry;
	struct wl_listener associate;
	struct wl_listener dissociate;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener override_redirect;
};
#endif

struct sway_popup_desc {
	struct wlr_scene_node *relative;
	struct sway_view *view;
};

struct sway_xdg_popup {
	struct sway_view *view;
	struct wlr_xdg_popup *wlr_xdg_popup;

	struct wlr_scene_tree *scene_tree;
	struct wlr_scene_tree *xdg_surface_tree;

	struct wlr_scene_tree *image_capture_tree;

	struct sway_popup_desc desc;

	struct wl_listener surface_commit;
	struct wl_listener new_popup;
	struct wl_listener reposition;
	struct wl_listener destroy;
};

const char *view_get_title(struct sway_view *view);

const char *view_get_app_id(struct sway_view *view);

const char *view_get_class(struct sway_view *view);

const char *view_get_instance(struct sway_view *view);

uint32_t view_get_x11_window_id(struct sway_view *view);

uint32_t view_get_x11_parent_id(struct sway_view *view);

const char *view_get_window_role(struct sway_view *view);

uint32_t view_get_window_type(struct sway_view *view);

const char *view_get_sandbox_engine(struct sway_view *view);

const char *view_get_sandbox_app_id(struct sway_view *view);

const char *view_get_sandbox_instance_id(struct sway_view *view);

const char *view_get_tag(struct sway_view *view);

const char *view_get_shell(struct sway_view *view);

void view_get_constraints(struct sway_view *view, double *min_width,
		double *max_width, double *min_height, double *max_height);

uint32_t view_configure(struct sway_view *view, double lx, double ly, int width,
	int height);

bool view_inhibit_idle(struct sway_view *view);

/**
 * Whether or not this view's most distant ancestor (possibly itself) is the
 * only visible node in its tree. If the view is tiling, there may be floating
 * views. If the view is floating, there may be tiling views or views in a
 * different floating container.
 */
bool view_ancestor_is_only_visible(struct sway_view *view);

/**
 * Configure the view's position and size based on the container's position and
 * size, taking borders into consideration.
 */
void view_autoconfigure(struct sway_view *view);

void view_set_activated(struct sway_view *view, bool activated);

/**
 * Called when the view requests to be focused.
 */
void view_request_activate(struct sway_view *view, struct sway_seat *seat);

/*
 * Called when the view requests urgent state
 */
void view_request_urgent(struct sway_view *view);

/**
 * If possible, instructs the client to change their decoration mode.
 */
void view_set_csd_from_server(struct sway_view *view, bool enabled);

/**
 * Updates the view's border setting when the client unexpectedly changes their
 * decoration mode.
 */
void view_update_csd_from_client(struct sway_view *view, bool enabled);

void view_set_tiled(struct sway_view *view, bool tiled);

void view_close(struct sway_view *view);

void view_close_popups(struct sway_view *view);

// view implementation

bool view_init(struct sway_view *view, enum sway_view_type type,
	const struct sway_view_impl *impl);

void view_destroy(struct sway_view *view);

void view_begin_destroy(struct sway_view *view);

/**
 * Map a view, ie. make it visible in the tree.
 *
 * `fullscreen` should be set to true (and optionally `fullscreen_output`
 * should be populated) if the view should be made fullscreen immediately.
 *
 * `decoration` should be set to true if the client prefers CSD. The client's
 * preference may be ignored.
 */
void view_map(struct sway_view *view, struct wlr_surface *wlr_surface,
	bool fullscreen, struct wlr_output *fullscreen_output, bool decoration);

void view_unmap(struct sway_view *view);

void view_update_size(struct sway_view *view);
void view_center_and_clip_surface(struct sway_view *view);

struct sway_view *view_from_wlr_xdg_surface(
	struct wlr_xdg_surface *xdg_surface);
#if WLR_HAS_XWAYLAND
struct sway_view *view_from_wlr_xwayland_surface(
	struct wlr_xwayland_surface *xsurface);
#endif
struct sway_view *view_from_wlr_surface(struct wlr_surface *surface);

void view_update_app_id(struct sway_view *view);

/**
 * Re-read the view's title property and update any relevant title bars.
 * The force argument makes it recreate the title bars even if the title hasn't
 * changed.
 */
void view_update_title(struct sway_view *view, bool force);

/**
 * Run any criteria that match the view and haven't been run on this view
 * before.
 */
void view_execute_criteria(struct sway_view *view);

/**
 * Returns true if there's a possibility the view may be rendered on screen.
 * Intended for damage tracking.
 */
bool view_is_visible(struct sway_view *view);

void view_set_urgent(struct sway_view *view, bool enable);

bool view_is_urgent(struct sway_view *view);

void view_remove_saved_buffer(struct sway_view *view);

void view_save_buffer(struct sway_view *view);

bool view_is_transient_for(struct sway_view *child, struct sway_view *ancestor);

void view_assign_ctx(struct sway_view *view, struct launcher_ctx *ctx);

void view_send_frame_done(struct sway_view *view);

bool view_can_tear(struct sway_view *view);

void xdg_toplevel_tag_manager_v1_handle_set_tag(struct wl_listener *listener, void *data);

#endif
