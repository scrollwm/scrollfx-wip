#ifndef _SWAY_NODE_H
#define _SWAY_NODE_H
#include <wayland-server-core.h>
#include <stdbool.h>
#include <scene-scroll/scene.h>
#include "list.h"

#define MIN_SANE_W 100
#define MIN_SANE_H 60

struct sway_root;
struct sway_output;
struct sway_workspace;
struct sway_container;
struct sway_transaction_instruction;
struct wlr_box;

enum sway_node_type {
	N_ROOT,
	N_OUTPUT,
	N_WORKSPACE,
	N_CONTAINER,
};

enum sway_node_focus_warp {
	FOCUS_WARP_NONE,
	FOCUS_WARP_DEFAULT,
	FOCUS_WARP_FORCE
};

struct sway_node {
	enum sway_node_type type;
	union {
		struct sway_root *sway_root;
		struct sway_output *sway_output;
		struct sway_workspace *sway_workspace;
		struct sway_container *sway_container;
	};

	/**
	 * A unique ID to identify this node.
	 * Primarily used in the get_tree JSON output.
	 */
	size_t id;

	struct sway_transaction_instruction *instruction;
	size_t ntxnrefs;
	bool destroying;

	// If true, indicates that the container has pending state that differs from
	// the current.
	bool dirty;

	struct {
		struct wl_signal destroy;
	} events;

	enum sway_node_focus_warp focus_warp;
};

void node_init(struct sway_node *node, enum sway_node_type type, void *thing);

const char *node_type_to_str(enum sway_node_type type);

/**
 * Mark a node as dirty if it isn't already. Dirty nodes will be included in the
 * next transaction then unmarked as dirty.
 */
void node_set_dirty(struct sway_node *node);

bool node_is_view(struct sway_node *node);

char *node_get_name(struct sway_node *node);

void node_get_box(struct sway_node *node, struct wlr_box *box);

struct sway_output *node_get_output(struct sway_node *node);

enum sway_container_layout node_get_layout(struct sway_node *node);

struct sway_node *node_get_parent(struct sway_node *node);

list_t *node_get_children(struct sway_node *node);

bool node_has_ancestor(struct sway_node *node, struct sway_node *ancestor);

// when destroying a sway tree, it's not known which order the tree will be
// destroyed. To prevent freeing of scene_nodes recursing up the tree,
// let's use this helper function to disown them to the staging node.
void scene_node_disown_children(struct sway_scene_tree *tree);

// a helper function used to allocate tree nodes. If an allocation failure
// occurs a flag is flipped that can be checked later to destroy a parent
// of this scene node preventing memory leaks.
struct sway_scene_tree *alloc_scene_tree(struct sway_scene_tree *parent,
		bool *failed);

// Set node's focus warp mode that will be applied when the transaction ends.
// If it is a container, its view will already be mapped and the container's
// position correct
void node_set_focus_warp(struct sway_node *node, enum sway_node_focus_warp warp);

enum sway_node_focus_warp node_get_focus_warp(struct sway_node *node);

#endif
