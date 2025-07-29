#ifndef TYPES_SWAY_SCENE_H
#define TYPES_SWAY_SCENE_H

#include <sway/tree/scene.h>

struct sway_scene *scene_node_get_root(struct sway_scene_node *node);

void scene_surface_set_clip(struct sway_scene_surface *surface, struct wlr_box *clip);

#endif