**`sway/desktop/layer_shell.c`**

**Purpose**: Manages Wayland layer surfaces with SceneFX visual effects integration

**SceneFX Changes**:

**Renderer Integration**: 
- Includes SceneFX scene types: `#include <scenefx/types/wlr_scene.h>`

**Blur Implementation**:
```c
struct sway_layer_surface {
    // ... existing fields ...
    bool blur_enabled;
    bool blur_xray;
    bool blur_ignore_transparent;
};

void layer_apply_criteria(struct sway_layer_surface *surface, struct layer_criteria *criteria) {
    if (criteria) {
        surface->blur_enabled = criteria->blur_enabled;
        surface->blur_xray = criteria->blur_xray;
        surface->blur_ignore_transparent = criteria->blur_ignore_transparent;
    } else {
        surface->blur_enabled = false;
        surface->blur_xray = false;
        surface->blur_ignore_transparent = false;
    }
}
```

**Shadow Implementation**:
```c
struct sway_layer_surface {
    // ... existing fields ...
    bool shadow_enabled;
    struct wlr_scene_shadow *shadow_node;
};

// Shadow node creation in sway_layer_surface_create()
surface->shadow_node = alloc_scene_shadow(surface->tree, 0, 0,
    0, config->shadow_blur_sigma, config->shadow_color, &failed);

// Shadow configuration in arrange_surface()
wlr_scene_node_set_enabled(&surface->shadow_node->node, surface->shadow_enabled);
if (surface->shadow_enabled) {
    wlr_scene_shadow_set_size(surface->shadow_node,
        surface->layer_surface->surface->current.width + config->shadow_blur_sigma * 2,
        surface->layer_surface->surface->current.height + config->shadow_blur_sigma * 2);
    
    wlr_scene_shadow_set_clipped_region(surface->shadow_node, (struct clipped_region) {
        .corner_radius = surface->corner_radius,
        .corners = CORNER_LOCATION_ALL,
        .area = {
            .x = -x, .y = -y,
            .width = surface->layer_surface->surface->current.width,
            .height = surface->layer_surface->surface->current.height,
        },
    });
}
```

**Corner Radius**:
```c
struct sway_layer_surface {
    int corner_radius;
};

void layer_apply_criteria(struct sway_layer_surface *surface, struct layer_criteria *criteria) {
    if (criteria) {
        surface->corner_radius = criteria->corner_radius;
    } else {
        surface->corner_radius = 0;
    }
}
```

**Configuration Integration**:
- New criteria parsing: `layer_parse_criteria()` function calls `layer_criteria_for_namespace()`
- Applied on map: `layer_parse_criteria(surface)` in `handle_map()`
- Reset on surface creation with default values

**New SceneFX API Calls**:
- `alloc_scene_shadow()` - creates shadow nodes
- `wlr_scene_shadow_set_size()`
- `wlr_scene_shadow_set_clipped_region()`
- `wlr_scene_shadow_set_blur_sigma()`

**Key Function Modifications**:
```c
// Enhanced surface creation with effects
static struct sway_layer_surface *sway_layer_surface_create() {
    // Original: basic surface allocation
    // SwayFX: adds shadow node creation and effects initialization
    surface->shadow_node = alloc_scene_shadow(surface->tree, 0, 0,
        0, config->shadow_blur_sigma, config->shadow_color, &failed);
}

// New effects application in arrange_surface()
static void arrange_surface() {
    // Original: basic layout arrangement
    // SwayFX: adds shadow positioning and clipping configuration
    wlr_scene_node_set_enabled(&surface->shadow_node->node, surface->shadow_enabled);
    // ... shadow sizing and clipping logic
}
```

---

**`sway/desktop/output.c`**

**Purpose**: Output rendering with comprehensive SceneFX effects integration

**SceneFX Changes**:

**Renderer Integration**:
- Added SceneFX corner location import: `#include "scenefx/types/fx/corner_location.h"`

**Blur Implementation**:
```c
static void output_configure_scene(struct sway_output *output, struct wlr_scene_node *node, 
        float opacity, int corner_radius, bool blur_enabled, bool has_titlebar, 
        struct sway_container *closest_con) {
    
    // Container blur state tracking
    if (con) {
        blur_enabled = con->blur_enabled;
    }
    
    // Buffer-level blur application
    if (surface && layer_surface && layer_surface->data) {
        struct sway_layer_surface *surface = layer_surface->data;
        wlr_scene_buffer_set_backdrop_blur(buffer, surface->blur_enabled);
        wlr_scene_buffer_set_backdrop_blur_ignore_transparent(buffer, surface->blur_ignore_transparent);
        wlr_scene_buffer_set_backdrop_blur_optimized(buffer, surface->blur_xray);
    }
    
    // XRay blur optimization
    bool should_optimize_blur = (closest_con && !container_is_floating_or_child(closest_con)) || config->blur_xray;
    wlr_scene_buffer_set_backdrop_blur_optimized(buffer, should_optimize_blur);
}
```

**Shadow Implementation**:
```c
// Layer surface shadow configuration in output_configure_scene()
if (layer_surface && layer_surface->data) {
    struct sway_layer_surface *surface = layer_surface->data;
    wlr_scene_shadow_set_blur_sigma(surface->shadow_node, config->shadow_blur_sigma);
    wlr_scene_shadow_set_corner_radius(surface->shadow_node, surface->corner_radius);
}
```

**Corner Radius**:
```c
static void output_configure_scene() {
    // Container corner radius tracking
    if (con) {
        corner_radius = con->corner_radius;
    }
    
    // Buffer corner radius application
    wlr_scene_buffer_set_corner_radius(buffer,
        container_has_corner_radius(closest_con) ? corner_radius : 0,
        has_titlebar ? CORNER_LOCATION_BOTTOM : CORNER_LOCATION_ALL);
        
    // Layer surface corners
    wlr_scene_buffer_set_corner_radius(buffer, surface->corner_radius, CORNER_LOCATION_ALL);
}
```

**Dimming/Opacity**:
```c
static void output_configure_scene() {
    // Container opacity inheritance
    if (con) {
        opacity = con->alpha;
    }
    
    // Alpha modifier surface integration
    if (surface) {
        const struct wlr_alpha_modifier_surface_v1_state *alpha_modifier_state =
            wlr_alpha_modifier_v1_get_surface_state(surface->surface);
        if (alpha_modifier_state != NULL) {
            opacity *= (float)alpha_modifier_state->multiplier;
        }
    }
    
    wlr_scene_buffer_set_opacity(buffer, opacity);
}
```

**New SceneFX API Calls**:
- `wlr_scene_buffer_set_corner_radius()`
- `wlr_scene_buffer_set_backdrop_blur()`
- `wlr_scene_buffer_set_backdrop_blur_optimized()`
- `wlr_scene_buffer_set_backdrop_blur_ignore_transparent()`
- `wlr_scene_shadow_set_blur_sigma()`
- `wlr_scene_shadow_set_corner_radius()`

**Key Function Modifications**:
```c
// Enhanced scene configuration with effects
static void output_configure_scene(struct sway_output *output, struct wlr_scene_node *node, 
        float opacity, int corner_radius, bool blur_enabled, bool has_titlebar, 
        struct sway_container *closest_con) {
    // Original: basic opacity handling
    // SwayFX: comprehensive effects application with container state tracking
    
    // Recursive effects inheritance through scene tree
    struct sway_container *con = scene_descriptor_try_get(node, SWAY_SCENE_DESC_CONTAINER);
    if (con) {
        closest_con = con;
        opacity = con->alpha;
        corner_radius = con->corner_radius;
        blur_enabled = con->blur_enabled;
    }
}

// Modified repaint handler
static int output_repaint_timer_handler(void *data) {
    // Original: output_configure_scene(output, &root->root_scene->tree.node, 1.0f);
    // SwayFX: Enhanced with effect parameters
    output_configure_scene(output, &root->root_scene->tree.node, 1.0f,
        0, false, false, NULL);
}
```

**Configuration Integration**:
- Effects applied per-frame during scene configuration
- Container-based effect inheritance through scene tree traversal
- Layer surface effects integrated with global config values (`config->shadow_blur_sigma`, `config->blur_xray`)

---

**sway/desktop/transaction.c**

**Purpose**: Core window arrangement and decoration management with SceneFX effects integration

**SceneFX Changes**:

**Renderer Integration**: 
- Added SceneFX header includes for effects management
- Integrated shadow nodes into transaction system

**Blur Implementation**:
- Added blur layer management in arrange functions
- Blur layer enable/disable logic in `arrange_output()` and `arrange_root()`

**Shadow Implementation**:
- Complete shadow management system integrated into container arrangement
- Shadow sizing, positioning, and clipping in `arrange_container()`
- Shadow enable/disable based on container properties
- Urgent state color switching for shadows

**Corner Radius**: 
- Border corner radius calculations and applications
- Clipping region setup for rounded borders
- Title bar corner radius integration

**Dimming/Opacity**:
- Dim rectangle positioning and sizing
- Corner radius application to dim overlays

**Key Function Modifications**:
```c
// arrange_children() - Shadow management added
wlr_scene_node_set_enabled(&child->shadow->node, false); // Tabbed/stacked
wlr_scene_node_set_enabled(&child->shadow->node,
    container_has_shadow(child) && child->view); // Tiled layouts

// arrange_container() - Complete shadow system
if (container_has_shadow(con)) {
    int corner_radius = has_corner_radius ? con->corner_radius + con->current.border_thickness : 0;
    wlr_scene_shadow_set_size(con->shadow,
        width + config->shadow_blur_sigma * 2,
        height + config->shadow_blur_sigma * 2);
    // Shadow positioning, clipping, coloring
}

// arrange_output() - Blur layer management  
wlr_scene_node_set_enabled(&output->layers.blur_layer->node, !fs);

// arrange_root() - Global blur management
wlr_scene_node_set_enabled(&root->layers.blur_tree->node, !fs);
wlr_scene_node_reparent(&output->layers.blur_layer->node, root->layers.blur_tree);
```

**New SceneFX API Calls**:
- `wlr_scene_shadow_set_size()`
- `wlr_scene_shadow_set_clipped_region()`
- `wlr_scene_shadow_set_color()`
- `wlr_scene_shadow_set_blur_sigma()`
- `wlr_scene_shadow_set_corner_radius()`
- `wlr_scene_rect_set_corner_radius()`
- `wlr_scene_rect_set_clipped_region()`

**Configuration Integration**:
- Shadow configuration: `config->shadow_blur_sigma`, `config->shadow_offset_x/y`
- Shadow colors: `config->shadow_color`, `config->shadow_inactive_color`
- Container property checks: `container_has_shadow()`, `container_has_corner_radius()`

---

**sway/desktop/xdg_shell.c**

**Purpose**: XDG shell window management with minimal SceneFX integration

**SceneFX Changes**:

**Configuration**: 
- Added scratchpad minimize support through `config->scratchpad_minimize`

**Key Function Modifications**:
```c
// handle_request_minimize() - New function
static void handle_request_minimize() {
    if (!container->scratchpad) {
        root_scratchpad_add_container(container, NULL);
    } else if (container->pending.workspace) {
        root_scratchpad_hide(container);
    }
}

// handle_map() - Conditional minimize listener
if (config->scratchpad_minimize) {
    xdg_shell_view->request_minimize.notify = handle_request_minimize;
    wl_signal_add(&toplevel->events.request_minimize,
        &xdg_shell_view->request_minimize);
}
```

---

**sway/desktop/xwayland.c**

**Purpose**: Xwayland window management with SceneFX scene graph integration

**SceneFX Changes**:

**Renderer Integration**:
- Header change: `#include <scenefx/types/wlr_scene.h>`

**Configuration**:
- Enhanced minimize handling with scratchpad integration

**Key Function Modifications**:
```c
// handle_request_minimize() - Enhanced scratchpad support
if (config->scratchpad_minimize) {
    if(e->minimize) {
        if (!container->scratchpad) {
            root_scratchpad_add_container(container, NULL);
        } else if (container->pending.workspace) {
            root_scratchpad_hide(container);
        }
    } else {
        if(container->scratchpad) {
            root_scratchpad_show(container);
        }
    }
    transaction_commit_dirty();
    return;
}
```

---

**sway/desktop/tearing.c**

**Purpose**: Tearing control management

**SceneFX Changes**: None - file unchanged between Sway and SwayFX versions
