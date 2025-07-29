# Part 1/2

**`sway/tree/arrange.c`**

**Purpose**: Layout arrangement with SceneFX blur layer size management

**SceneFX Changes**:

**Blur Implementation**: 
- Optimized blur layer size updates during output arrangement
```c
void arrange_output(struct sway_output *output) {
    // ... existing arrangement code ...
    
    int output_width, output_height;
    wlr_output_effective_resolution(output->wlr_output, &output_width, &output_height);
    wlr_scene_optimized_blur_set_size(output->layers.blur_layer, output_width, output_height);
}
```

**New SceneFX API Calls**:
- `wlr_scene_optimized_blur_set_size()` - updates blur layer dimensions to match output resolution

---

**`sway/tree/container.c`**

**Purpose**: Container lifecycle management with SceneFX effects integration

**SceneFX Changes**:

**Shadow Implementation**:
```c
struct sway_container *container_create(struct sway_view *view) {
    // ... existing container setup ...
    
    c->shadow = alloc_scene_shadow(c->scene_tree, 0, 0,
        0, config->shadow_blur_sigma, config->shadow_color, &failed);
    
    // Shadow positioning and configuration
    c->corner_radius = config->corner_radius;
    c->blur_enabled = config->blur_enabled;
    c->shadow_enabled = config->shadow_enabled;
    c->dim = config->default_dim_inactive;
}
```

**Corner Radius**: 
- Titlebar corner radius with proper corner location handling
```c
void container_arrange_title_bar(struct sway_container *con) {
    int background_corner_radius = container_has_corner_radius(con) ?
        con->corner_radius + con->current.border_thickness - thickness : 0;
    enum corner_location corners = CORNER_LOCATION_TOP;
    
    // Handle tabbed/stacked layout corner variations
    if (layout == L_TABBED && siblings->length > 1) {
        if (siblings->items[0] == con) {
            corners = CORNER_LOCATION_TOP_LEFT;
        } else if (siblings->items[siblings->length - 1] == con) {
            corners = CORNER_LOCATION_TOP_RIGHT;
        } else {
            background_corner_radius = 0;
            corners = CORNER_LOCATION_NONE;
        }
    }
    
    wlr_scene_rect_set_corner_radius(con->title_bar.background, background_corner_radius, corners);
    wlr_scene_rect_set_corner_radius(con->title_bar.border, 
        background_corner_radius ? background_corner_radius + thickness : 0, corners);
}
```

**Dimming/Opacity**:
```c
void container_update(struct sway_container *con) {
    if (con->dim_rect) {
        float *color = config->dim_inactive_colors.unfocused;
        
        if (con->view && view_is_urgent(con->view)) {
            color = config->dim_inactive_colors.urgent;
        }
        
        bool focused = con->current.focused || container_is_current_parent_focused(con);
        scene_rect_set_color(con->dim_rect, color, focused ? 0.0 : con->dim);
    }
}
```

**Configuration**: 
- New container properties for SceneFX features
- Shadow, blur, corner radius, and dimming state management

**Key Function Modifications**:
```c
// Shadow and corner radius helper functions
bool container_has_shadow(struct sway_container *con) {
    return con->shadow_enabled
        && (con->current.border != B_CSD || config->shadows_on_csd_enabled);
}

bool container_has_corner_radius(struct sway_container *con) {
    return (container_is_floating_or_child(con) ||
        !(config->smart_corner_radius && con->current.workspace->current_gaps.top == 0)) &&
        con->corner_radius;
}
```

**New SceneFX API Calls**:
- `wlr_scene_rect_set_corner_radius()` - applies rounded corners to rectangles
- `wlr_scene_rect_set_clipped_region()` - creates clipping regions for effects
- `alloc_scene_shadow()` - creates shadow scene nodes

**Configuration Integration**:
- `config->corner_radius`, `config->blur_enabled`, `config->shadow_enabled`
- `config->shadow_blur_sigma`, `config->shadow_color`
- `config->default_dim_inactive`, `config->dim_inactive_colors`
- `config->shadows_on_csd_enabled`, `config->smart_corner_radius`

---

**`sway/tree/node.c`**

**Purpose**: Scene node allocation helpers for SceneFX effects

**SceneFX Changes**:

**Shadow Implementation**:
```c
struct wlr_scene_shadow *alloc_scene_shadow(struct wlr_scene_tree *parent,
        int width, int height, int corner_radius, float blur_sigma,
        const float color [static 4], bool *failed) {
    if (*failed) {
        return NULL;
    }

    struct wlr_scene_shadow *shadow = wlr_scene_shadow_create(
            parent, width, height, corner_radius, blur_sigma, color);
    if (!shadow) {
        sway_log(SWAY_ERROR, "Failed to allocate a scene node");
        *failed = true;
    }

    return shadow;
}
```

**New SceneFX API Calls**:
- `wlr_scene_shadow_create()` - creates shadow scene nodes with blur and color parameters

---

# Part 2/2

**`sway/tree/output.c`**

**Purpose**: Output management with SceneFX blur layer integration

**SceneFX Changes**:

**Renderer Integration**: Replace wlroots scene includes with SceneFX
```c
// Original Sway
#include <wlr/types/wlr_scene.h>

// SwayFX 
#include <scenefx/types/wlr_scene.h>
```

**Blur Implementation**: Add blur layer to output layer stack
```c
// output_create() - Original Sway
output->layers.tiling = alloc_scene_tree(root->staging, &failed);
output->layers.fullscreen = alloc_scene_tree(root->staging, &failed);

// SwayFX addition
output->layers.blur_layer = wlr_scene_optimized_blur_create(root->staging, 0, 0);
```

**Key Function Modifications**:
```c
destroy_scene_layers() {
    // Original cleanup
    wlr_scene_node_destroy(&output->layers.tiling->node);
    wlr_scene_node_destroy(&output->layers.fullscreen->node);
    
    // SwayFX addition
    wlr_scene_node_destroy(&output->layers.blur_layer->node);
}
```

**New SceneFX API Calls**:
* `wlr_scene_optimized_blur_create()` - Creates optimized blur layer for output

---

**`sway/tree/root.c`**

**Purpose**: Root scene graph setup with blur tree and minimize functionality

**SceneFX Changes**:

**Renderer Integration**: SceneFX scene header replacement
```c
// Original Sway  
#include <wlr/types/wlr_scene.h>

// SwayFX
#include <scenefx/types/wlr_scene.h>
```

**Blur Implementation**: Add blur tree to root layer stack
```c
// root_create() - SwayFX addition
root->layers.blur_tree = alloc_scene_tree(root->layer_tree, &failed);
```

**Key Function Modifications**:
```c
// New function in SwayFX
root_scratchpad_set_minimize(struct sway_container *con, bool minimize) {
    if (con->view) {
#if WLR_HAS_XWAYLAND
        struct wlr_xwayland_surface *xsurface;
        if ((xsurface = wlr_xwayland_surface_try_from_wlr_surface(con->view->surface))) {
            wlr_xwayland_surface_set_minimized(xsurface, minimize);
            return;
        }
#endif
        struct wlr_foreign_toplevel_handle_v1 *foreign_toplevel = con->view->foreign_toplevel;
        if (foreign_toplevel) {
            wlr_foreign_toplevel_handle_v1_set_minimized(foreign_toplevel, minimize);
        }
    }
}
```

**Configuration Integration**:
```c
// Scratchpad minimize integration
if (config->scratchpad_minimize) {
    root_scratchpad_set_minimize(con, true);  // when hiding
    root_scratchpad_set_minimize(con, false); // when showing
}
```

---

**`sway/tree/view.c`**

**Purpose**: View rendering with blur, corner radius, and effect clipping

**SceneFX Changes**:

**Blur Implementation**: Modify clipping logic for blur compatibility
```c
// view_center_and_clip_surface() - Original Sway
clip_to_geometry = !view->using_csd;

// SwayFX  
clip_to_geometry = !view->using_csd
    || con->blur_enabled
    || con->shadow_enabled  
    || con->corner_radius > 0;
```

**Corner Radius**: Apply corner radius to saved buffers
```c
// view_save_buffer_iterator() - SwayFX additions
wlr_scene_buffer_set_corner_radius(sbuf, buffer->corner_radius, buffer->corners);
wlr_scene_buffer_set_backdrop_blur(sbuf, buffer->backdrop_blur);
wlr_scene_buffer_set_backdrop_blur_optimized(sbuf, buffer->backdrop_blur_optimized);
wlr_scene_buffer_set_backdrop_blur_ignore_transparent(sbuf, buffer->backdrop_blur_ignore_transparent);
```

**Key Function Modifications**:
```c
// New minimize handling
handle_foreign_minimize(struct wl_listener *listener, void *data) {
    struct sway_view *view = wl_container_of(listener, view, foreign_minimize);
    struct wlr_foreign_toplevel_handle_v1_minimized_event *event = data;
    struct sway_container *container = view->container;
    
    if (event->minimized) {
        if (!container->scratchpad) {
            root_scratchpad_add_container(container, NULL);
        } else if (container->pending.workspace) {
            root_scratchpad_hide(container);
        }
    } else {
        if (container->scratchpad)
            root_scratchpad_show(container);
    }
}
```

**Configuration Integration**:
```c
// Foreign toplevel minimize listener setup
if (config->scratchpad_minimize) {
    view->foreign_minimize.notify = handle_foreign_minimize;
    wl_signal_add(&view->foreign_toplevel->events.request_minimize,
            &view->foreign_minimize);
}

// Cleanup
if (config->scratchpad_minimize) {
    wl_list_remove(&view->foreign_minimize.link);
}
```

**New SceneFX API Calls**:
* `wlr_scene_buffer_set_corner_radius()` - Apply corner radius to buffers
* `wlr_scene_buffer_set_backdrop_blur()` - Enable backdrop blur
* `wlr_scene_buffer_set_backdrop_blur_optimized()` - Use optimized blur
* `wlr_scene_buffer_set_backdrop_blur_ignore_transparent()` - Skip transparent areas

---

**`sway/tree/workspace.c`**

**Purpose**: No SceneFX-specific changes detected - workspace functionality remains unchanged between Sway and SwayFX versions.
