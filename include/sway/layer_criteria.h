#ifndef _SWAY_LAYER_CRITERIA_H
#define _SWAY_LAYER_CRITERIA_H
#include <stdbool.h>

/**
 * Layer criteria structure for applying effects to layer shell surfaces
 * based on their namespace (e.g., waybar, mako, etc.)
 */
struct layer_criteria {
	char *namespace;
	char *cmdlist;

	// Effect properties
	bool shadow_enabled;
	bool blur_enabled;
	bool blur_xray;
	bool blur_ignore_transparent;
	int corner_radius;
};

/**
 * Destroy a layer criteria and free its memory
 */
void layer_criteria_destroy(struct layer_criteria *criteria);

/**
 * Add a new layer criteria for the given namespace
 * Returns the created criteria or NULL on failure
 */
struct layer_criteria *layer_criteria_add(char *namespace, char *cmdlist);

/**
 * Find layer criteria for a given namespace
 * Returns the matching criteria or NULL if not found
 */
struct layer_criteria *layer_criteria_for_namespace(char *namespace);

#endif
