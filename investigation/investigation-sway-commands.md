SwayFX adds entirely new command files that don't exist in Sway. Key SceneFX-related new commands include: blur.c, blur_brightness.c, blur_contrast.c, blur_noise.c, blur_passes.c, blur_radius.c, blur_saturation.c, blur_xray.c, corner_radius.c, default_dim_inactive.c, dim_inactive.c, dim_inactive_colors.c, layer_effects.c, shadow_blur_radius.c, shadow_color.c, shadow_inactive_color.c, shadow_offset.c, shadows.c, shadows_on_csd.c, smart_corner_radius.c, scratchpad_minimize.c, and titlebar_separator.c

---

# Part 1

Looking at the files provided, I can see:

/sway/commands/allow_tearing.c - This appears to be identical in both provided versions (documents 28 and 120), so no SceneFX changes here.
/sway/commands/assign.c - Also appears identical in both versions (documents 29 and 121), so no SceneFX changes.
/sway/commands/bar.c and all /sway/commands/bar/* files - These appear to be identical between the two versions, suggesting no SceneFX-specific changes to bar commands.
/sway/commands/input/* files - These also appear identical, so no SceneFX changes to input commands.
/sway/commands/output/* files - These appear identical as well, so no SceneFX changes to output commands.
/sway/commands/seat/* files - These also appear identical.

---

# Part 2

**`blur.c`**
**Purpose**: Container-level blur control command
**SceneFX Changes**:
***Blur Implementation**: 
- Sets `con->blur_enabled` flag on containers
- Marks output blur layers dirty via `wlr_scene_optimized_blur_mark_dirty()`
- Global blur config affects all containers via `arrange_blur_iter()`

**Key Function Modifications**:
```c
struct cmd_results *cmd_blur(int argc, char **argv) {
    // Sets container or global blur state
    if (con == NULL) {
        config->blur_enabled = result;
        root_for_each_container(arrange_blur_iter, NULL);
    } else {
        con->blur_enabled = result;
    }
    
    // Mark all output blur layers dirty
    struct sway_output *output;
    wl_list_for_each(output, &root->all_outputs, link) {
        wlr_scene_optimized_blur_mark_dirty(output->layers.blur_layer);
    }
}

static void arrange_blur_iter(struct sway_container *con, void *data) {
    con->blur_enabled = config->blur_enabled;
}
```

**New SceneFX API Calls**:
- `wlr_scene_optimized_blur_mark_dirty()` - Forces blur re-render

---

**`blur_brightness.c`**
**Purpose**: Global blur brightness parameter control
**SceneFX Changes**:
***Blur Implementation**:
- Modifies scene-level blur data structure
- Uses SceneFX scene blur data API

**Key Function Modifications**:
```c
struct cmd_results *cmd_blur_brightness(int argc, char **argv) {
    struct wlr_scene *root_scene = root->root_scene;
    struct blur_data blur_data = root_scene->blur_data;
    blur_data.brightness = value;
    wlr_scene_set_blur_data(root_scene, blur_data);
}
```

**New SceneFX API Calls**:
- `wlr_scene_set_blur_data()` - Sets global blur parameters

---

**`blur_contrast.c`** 
**Purpose**: Global blur contrast parameter control
**SceneFX Changes**:
***Blur Implementation**:
- Sets contrast value in scene blur data

**New SceneFX API Calls**:
- `wlr_scene_set_blur_data()` - Updates scene blur contrast

---

**`blur_noise.c`**
**Purpose**: Global blur noise parameter control  
**SceneFX Changes**:
***Blur Implementation**:
- Sets noise value in scene blur data

**New SceneFX API Calls**:
- `wlr_scene_set_blur_data()` - Updates scene blur noise

---

**`blur_passes.c`**
**Purpose**: Global blur pass count control
**SceneFX Changes**:
***Blur Implementation**:
- Sets number of blur passes in scene data

**New SceneFX API Calls**:
- `wlr_scene_set_blur_data()` - Updates blur pass count

---

**`blur_radius.c`**
**Purpose**: Global blur radius parameter control
**SceneFX Changes**:
***Blur Implementation**:
- Sets blur radius in scene blur data

**New SceneFX API Calls**:
- `wlr_scene_set_blur_data()` - Updates scene blur radius

---

**`blur_saturation.c`**
**Purpose**: Global blur saturation parameter control
**SceneFX Changes**:
***Blur Implementation**:
- Sets saturation value in scene blur data

**New SceneFX API Calls**:
- `wlr_scene_set_blur_data()` - Updates scene blur saturation

---

**`blur_xray.c`**
**Purpose**: Global blur xray mode control
**SceneFX Changes**:
***Blur Implementation**:
- Sets `config->blur_xray` flag
- Marks all output blur layers dirty

**Key Function Modifications**:
```c
struct cmd_results *cmd_blur_xray(int argc, char **argv) {
    config->blur_xray = result;
    
    struct sway_output *output;
    wl_list_for_each(output, &root->all_outputs, link) {
        wlr_scene_optimized_blur_mark_dirty(output->layers.blur_layer);
    }
}
```

**New SceneFX API Calls**:
- `wlr_scene_optimized_blur_mark_dirty()` - Forces blur re-render

---

**`corner_radius.c`**
**Purpose**: Global corner radius configuration
**SceneFX Changes**:
***Corner Radius**:
- Sets global `config->corner_radius` 
- Applies to all containers via `arrange_corner_radius_iter()`
- Updates titlebar padding based on corner radius

**Key Function Modifications**:
```c
struct cmd_results *cmd_corner_radius(int argc, char **argv) {
    config->corner_radius = value;
    
    if (!config->handler_context.container) {
        root_for_each_container(arrange_corner_radius_iter, NULL);
        arrange_root();
    }
    
    // Adjust titlebar padding for corners
    if (value > config->titlebar_h_padding) {
        config->titlebar_h_padding = value;
    }
    if (value > (int)container_titlebar_height()) {
        config->titlebar_v_padding = (value - config->font_height) / 2;
    }
}

static void arrange_corner_radius_iter(struct sway_container *con, void *data) {
    con->corner_radius = config->corner_radius;
}
```

**Configuration Integration**:
- Sets `config->corner_radius` global value
- Adjusts titlebar padding calculations
- Applies to container corner radius properties

---

**`default_dim_inactive.c`**
**Purpose**: Default window dimming configuration
**SceneFX Changes**:
***Dimming/Opacity**:
- Sets global default dimming value
- Triggers layout rearrangement

**Key Function Modifications**:
```c
struct cmd_results *cmd_default_dim_inactive(int argc, char **argv) {
    config->default_dim_inactive = val;
    
    if (config->active) {
        arrange_root();
    }
}
```

**Configuration Integration**:
- Sets `config->default_dim_inactive` float value

---

**`dim_inactive.c`**
**Purpose**: Per-container dimming control  
**SceneFX Changes**:
***Dimming/Opacity**:
- Sets container-specific dim value
- Only usable with for_window rules

**Key Function Modifications**:
```c
struct cmd_results *cmd_dim_inactive(int argc, char **argv) {
    struct sway_container *container = config->handler_context.container;
    container->dim = val;
}
```

**Configuration Integration**:
- Sets `container->dim` float value per container

---

**`dim_inactive_colors.c`**
**Purpose**: Dimming color configuration
**SceneFX Changes**:
***Dimming/Opacity**:
- Sets dim colors for unfocused/urgent states
- Triggers root rearrangement

**Key Function Modifications**:
```c
static struct cmd_results *handle_command(int argc, char **argv, char *cmd_name,
        float config_option[4]) {
    uint32_t color;
    parse_color(argv[0], &color);
    color_to_rgba(config_option, color);
    
    if (config->active) {
        arrange_root();
    }
}

struct cmd_results *cmd_dim_inactive_colors_unfocused(int argc, char **argv) {
    return handle_command(argc, argv, "dim_inactive_colors.unfocused",
            config->dim_inactive_colors.unfocused);
}

struct cmd_results *cmd_dim_inactive_colors_urgent(int argc, char **argv) {
    return handle_command(argc, argv, "dim_inactive_colors.urgent",
            config->dim_inactive_colors.urgent);
}
```

**Configuration Integration**:
- Sets `config->dim_inactive_colors.unfocused[4]` RGBA array
- Sets `config->dim_inactive_colors.urgent[4]` RGBA array

---

**`layer_effects.c`**
**Purpose**: Layer surface effects application
**SceneFX Changes**:
***Configuration**:
- Applies effects to layer surfaces via criteria system
- Iterates through shell layer trees to find matching surfaces

**Key Function Modifications**:
```c
struct cmd_results *cmd_layer_effects(int argc, char **argv) {
    struct layer_criteria *criteria = layer_criteria_add(argv[0], join_args(argv + 1, argc - 1));
    
    // Apply to all layer surfaces across outputs
    for (int i = 0; i < root->outputs->length; ++i) {
        struct sway_output *output = root->outputs->items[i];
        struct wlr_scene_tree *layers[] = {
            output->layers.shell_background,
            output->layers.shell_bottom,
            output->layers.shell_overlay,
            output->layers.shell_top,
        };
        
        // Check each layer surface against criteria
        wl_list_for_each_reverse(node, &layers[i]->children, link) {
            struct sway_layer_surface *surface = scene_descriptor_try_get(node,
                SWAY_SCENE_DESC_LAYER_SHELL);
            if (strcmp(surface->layer_surface->namespace, criteria->namespace) == 0) {
                layer_apply_criteria(surface, criteria);
            }
        }
        arrange_layers(output);
    }
}
```

**Configuration Integration**:
- Uses layer criteria system for effect application
- Calls `layer_apply_criteria()` and `arrange_layers()`

---

# Part 3

Looking at the SwayFX command implementations, I can identify several commands that are exclusively related to SceneFX cosmetic features:

**`sway/commands/shadow_blur_radius.c`**
**Purpose**: Configure shadow blur sigma/radius for box shadows
**SceneFX Changes**:
***Configuration**: Adds shadow blur radius control
```c
struct cmd_results *cmd_shadow_blur_radius(int argc, char **argv) {
    char *inv;
    int value = strtol(argv[0], &inv, 10);
    if (*inv != '\0' || value < 0 || value > 99) {
        return cmd_results_new(CMD_FAILURE, "Invalid size specified");
    }

    config->shadow_blur_sigma = value;
    arrange_root();
    
    return cmd_results_new(CMD_SUCCESS, NULL);
}
```
**Configuration Integration**: 
* New config member: `config->shadow_blur_sigma`
* Range validation: 0-99 pixels
* Triggers full re-arrangement for immediate effect

**`sway/commands/shadow_color.c`**
**Purpose**: Configure active window shadow color
**SceneFX Changes**:
***Shadow Implementation**: Shadow color management with inactive sync
```c
struct cmd_results *cmd_shadow_color(int argc, char **argv) {
    uint32_t color;
    if (!parse_color(argv[0], &color)) {
        return cmd_results_new(CMD_INVALID, "Invalid %s color %s",
                "shadow_color", argv[0]);
    }
    // Sync inactive color if they were previously the same
    if (memcmp(config->shadow_color, config->shadow_inactive_color, 
               sizeof(config->shadow_color)) == 0) {
        color_to_rgba(config->shadow_inactive_color, color);
    }
    color_to_rgba(config->shadow_color, color);

    arrange_root();
    return cmd_results_new(CMD_SUCCESS, NULL);
}
```
**Configuration Integration**:
* New config member: `config->shadow_color` (RGBA array)
* Automatic inactive color synchronization
* Uses standard color parsing utilities

**`sway/commands/shadow_inactive_color.c`**
**Purpose**: Configure inactive window shadow color
**SceneFX Changes**:
***Shadow Implementation**: Separate inactive window shadow coloring
```c
struct cmd_results *cmd_shadow_inactive_color(int argc, char **argv) {
    uint32_t color;
    if (!parse_color(argv[0], &color)) {
        return cmd_results_new(CMD_INVALID, "Invalid %s color %s",
                "shadow_inactive_color", argv[0]);
    }
    color_to_rgba(config->shadow_inactive_color, color);

    arrange_root();
    return cmd_results_new(CMD_SUCCESS, NULL);
}
```
**Configuration Integration**:
* New config member: `config->shadow_inactive_color`
* Independent from active shadow color
* Enables window focus indication through shadow color

**`sway/commands/shadow_offset.c`**
**Purpose**: Configure shadow positioning offset
**SceneFX Changes**:
***Shadow Implementation**: Shadow position control
```c
struct cmd_results *cmd_shadow_offset(int argc, char **argv) {
    char *err;
    float offset_x = strtof(argv[0], &err);
    float offset_y = strtof(argv[1], &err);
    if (*err || offset_x < -99.9f || offset_x > 99.9f) {
        return cmd_results_new(CMD_INVALID, "x offset float invalid");
    }
    if (*err || offset_y < -99.9f || offset_y > 99.9f) {
        return cmd_results_new(CMD_INVALID, "y offset float invalid");
    }

    config->shadow_offset_x = offset_x;
    config->shadow_offset_y = offset_y;

    return cmd_results_new(CMD_SUCCESS, NULL);
}
```
**Configuration Integration**:
* New config members: `config->shadow_offset_x`, `config->shadow_offset_y`
* Range validation: -99.9 to 99.9 pixels
* No immediate re-arrangement (applied during rendering)

**`sway/commands/shadows.c`**
**Purpose**: Enable/disable shadows globally or per-container
**SceneFX Changes**:
***Shadow Implementation**: Shadow toggle with container iteration
```c
static void arrange_shadow_iter(struct sway_container *con, void *data) {
    con->shadow_enabled = config->shadow_enabled;
}

struct cmd_results *cmd_shadows(int argc, char **argv) {
    struct sway_container *con = config->handler_context.container;

    bool result = parse_boolean(argv[0], true);
    if (con == NULL) {
        config->shadow_enabled = result;
        // Config reload: reset all containers to config value
        root_for_each_container(arrange_shadow_iter, NULL);
    } else {
        con->shadow_enabled = result;
    }

    arrange_root();
    return cmd_results_new(CMD_SUCCESS, NULL);
}
```
**Configuration Integration**:
* New config member: `config->shadow_enabled`
* New container member: `con->shadow_enabled`
* Global vs per-container shadow control
* Iterator function for bulk container updates

**`sway/commands/shadows_on_csd.c`**
**Purpose**: Control shadows on client-side decorated windows
**SceneFX Changes**:
***Shadow Implementation**: CSD-specific shadow control
```c
struct cmd_results *cmd_shadows_on_csd(int argc, char **argv) {
    config->shadows_on_csd_enabled = parse_boolean(argv[0], config->shadows_on_csd_enabled);

    arrange_root();
    return cmd_results_new(CMD_SUCCESS, NULL);
}
```
**Configuration Integration**:
* New config member: `config->shadows_on_csd_enabled`
* Separates CSD shadow behavior from SSD shadows
* Enables different shadow policies for decoration types

**`sway/commands/smart_corner_radius.c`**
**Purpose**: Enable smart corner radius behavior
**SceneFX Changes**:
***Corner Radius**: Intelligent corner radius application
```c
struct cmd_results *cmd_smart_corner_radius(int argc, char **argv) {
    config->smart_corner_radius = parse_boolean(argv[0], true);

    arrange_root();
    return cmd_results_new(CMD_SUCCESS, NULL);
}
```
**Configuration Integration**:
* New config member: `config->smart_corner_radius`
* Controls when corner radius is automatically applied
* Likely disables corners in fullscreen/maximized states

**`sway/commands/opacity.c`**
**Purpose**: Control window opacity/transparency
**SceneFX Changes**:
***Dimming/Opacity**: Dynamic opacity control with arithmetic operations
```c
struct cmd_results *cmd_opacity(int argc, char **argv) {
    struct sway_container *con = config->handler_context.container;

    char *err;
    float val = strtof(argc == 1 ? argv[0] : argv[1], &err);
    
    if (!strcasecmp(argv[0], "plus")) {
        val = con->alpha + val;
    } else if (!strcasecmp(argv[0], "minus")) {
        val = con->alpha - val;
    } else if (argc > 1 && strcasecmp(argv[0], "set")) {
        return cmd_results_new(CMD_INVALID,
                "Expected: set|plus|minus <0..1>: %s", argv[0]);
    }

    if (val < 0 || val > 1) {
        return cmd_results_new(CMD_FAILURE, "opacity value out of bounds");
    }

    con->alpha = val;
    container_update(con);

    return cmd_results_new(CMD_SUCCESS, NULL);
}
```
**Configuration Integration**:
* New container member: `con->alpha`
* Arithmetic operations: set/plus/minus
* Range validation: 0.0-1.0
* Immediate container update rather than full re-arrangement

**Key Function Modifications**:
```c
// Original Sway: No shadow/opacity commands exist
// vs
// SwayFX: Dedicated commands for each cosmetic feature

// Shadow commands use arrange_root() for immediate effect
arrange_root();

// Opacity uses container_update() for efficient updates  
container_update(con);

// Container iteration for global changes
root_for_each_container(arrange_shadow_iter, NULL);
```

**New SceneFX API Calls**:
* No direct SceneFX API calls in commands - they modify config/container state
* Effects applied during rendering via config values
* `arrange_root()` triggers re-rendering with new effect parameters
* `container_update()` efficiently updates single container effects

**Configuration Integration**:
* **Shadow config members**: `shadow_blur_sigma`, `shadow_color[]`, `shadow_inactive_color[]`, `shadow_offset_x/y`, `shadow_enabled`, `shadows_on_csd_enabled`
* **Corner config members**: `smart_corner_radius`  
* **Opacity config members**: Container-level `alpha` values
* **Boolean parsing**: Standard `parse_boolean()` for enable/disable
* **Color parsing**: Standard `parse_color()` and `color_to_rgba()` utilities
* **Range validation**: Specific bounds checking for each effect parameter

