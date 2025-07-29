## include/sway/config.h

**Purpose**: Core configuration structure modifications to support SceneFX cosmetic features

**SceneFX Changes**:
* **Renderer Integration**: No direct renderer changes in config header
* **Blur Implementation**: Added blur configuration options and data structures
* **Shadow Implementation**: Added shadow configuration parameters
* **Corner Radius**: Added corner radius configuration
* **Dimming/Opacity**: Added dim_inactive configuration options
* **Configuration**: New config struct members for all cosmetic features

**Key Struct Modifications**:

```c
struct sway_config {
    // New SceneFX configuration members
    int corner_radius;
    bool smart_corner_radius;
    
    float default_dim_inactive;
    struct {
        float unfocused[4];
        float urgent[4];
    } dim_inactive_colors;
    
    bool blur_enabled;
    bool blur_xray;
    struct blur_data blur_data;
    
    bool shadow_enabled;
    bool shadows_on_csd_enabled;
    int shadow_blur_sigma;
    float shadow_color[4];
    float shadow_inactive_color[4];
    float shadow_offset_x, shadow_offset_y;
    
    bool titlebar_separator;
    bool scratchpad_minimize;
    
    list_t *layer_criteria;
    // ... existing sway config members
};
```

**New SceneFX API Calls**:
* `#include <scenefx/types/fx/blur_data.h>` - Blur data structures
* Uses `struct blur_data` from SceneFX

**Configuration Integration**:
* Added blur parameters: enabled, xray, blur_data struct
* Added shadow parameters: enabled, CSD support, blur sigma, colors, offsets  
* Added corner radius with smart detection
* Added dimming with separate unfocused/urgent colors
* Added layer_criteria list for layer shell effects

## include/sway/tree/container.h

**Purpose**: Container structure modifications to store per-window effect properties

**SceneFX Changes**:
* **Shadow Implementation**: Added shadow scene node to container
* **Corner Radius**: Added corner radius property
* **Blur Implementation**: Added blur enabled flag
* **Dimming/Opacity**: Added opacity and dim properties

**Key Struct Modifications**:

```c
struct sway_container {
    // ... existing container members
    
    struct wlr_scene_shadow *shadow;
    struct wlr_scene_tree *content_tree;
    struct wlr_scene_rect *dim_rect;
    
    float alpha;
    int corner_radius;
    bool blur_enabled;
    bool shadow_enabled;
    float dim;
    
    // ... rest of container
};
```

**New SceneFX API Calls**:
* `struct wlr_scene_shadow *shadow` - SceneFX shadow node
* Uses SceneFX scene types for effects

**Configuration Integration**:
* Per-container effect properties stored in container struct
* Shadow node integrated into scene graph
* Dim rect for unfocused window dimming

## include/sway/commands.h

**Purpose**: Command declarations for new SceneFX configuration commands

**SceneFX Changes**:
* **Blur Implementation**: Added blur-related command declarations
* **Shadow Implementation**: Added shadow command declarations  
* **Corner Radius**: Added corner radius commands
* **Dimming/Opacity**: Added dimming commands
* **Configuration**: Added layer effects commands

**New Command Declarations**:

```c
// Blur commands
sway_cmd cmd_blur;
sway_cmd cmd_blur_brightness;
sway_cmd cmd_blur_contrast;
sway_cmd cmd_blur_noise;
sway_cmd cmd_blur_passes;
sway_cmd cmd_blur_radius;
sway_cmd cmd_blur_saturation;
sway_cmd cmd_blur_xray;

// Corner radius commands
sway_cmd cmd_corner_radius;
sway_cmd cmd_smart_corner_radius;
bool cmd_corner_radius_parse_value(char *arg, int* result);

// Dimming commands
sway_cmd cmd_default_dim_inactive;
sway_cmd cmd_dim_inactive;
sway_cmd cmd_dim_inactive_colors_unfocused;
sway_cmd cmd_dim_inactive_colors_urgent;

// Shadow commands
sway_cmd cmd_shadow_blur_radius;
sway_cmd cmd_shadow_color;
sway_cmd cmd_shadow_offset;
sway_cmd cmd_shadow_inactive_color;
sway_cmd cmd_shadows;
sway_cmd cmd_shadows_on_csd;

// Layer effects
sway_cmd cmd_layer_effects;

// Titlebar
sway_cmd cmd_titlebar_separator;

// Scratchpad
sway_cmd cmd_scratchpad_minimize;
```

**Configuration Integration**:
* Complete command interface for all SceneFX features
* Granular control over blur parameters
* Shadow configuration commands
* Layer shell effect commands

## include/sway/layer_criteria.h

**Purpose**: New header for layer shell effect configuration

**SceneFX Changes**:
* **Blur Implementation**: Layer shell blur configuration
* **Shadow Implementation**: Layer shell shadow configuration
* **Corner Radius**: Layer shell corner radius

**New Structures**:

```c
struct layer_criteria {
    char *namespace;
    char *cmdlist;
    
    bool shadow_enabled;
    bool blur_enabled;
    bool blur_xray;
    bool blur_ignore_transparent;
    int corner_radius;
};

void layer_criteria_destroy(struct layer_criteria *criteria);
struct layer_criteria *layer_criteria_add(char *namespace, char *cmdlist);
struct layer_criteria *layer_criteria_for_namespace(char *namespace);
```

**New SceneFX API Calls**:
* Layer-specific effect configuration
* Namespace-based effect application

**Configuration Integration**:
* Per-layer shell namespace effect configuration
* Supports blur, shadow, corner radius for panels/notifications

## include/sway/layers.h

**Purpose**: Layer shell surface modifications for SceneFX effects

**SceneFX Changes**:
* **Shadow Implementation**: Added shadow node to layer surface
* **Blur Implementation**: Added blur configuration flags
* **Corner Radius**: Added corner radius property

**Key Struct Modifications**:

```c
struct sway_layer_surface {
    // ... existing layer surface members
    
    struct wlr_scene_shadow *shadow_node;
    
    bool shadow_enabled;
    bool blur_enabled;
    bool blur_xray;
    bool blur_ignore_transparent;
    int corner_radius;
};

void layer_apply_criteria(struct sway_layer_surface *surface, struct layer_criteria *criteria);
```

**New SceneFX API Calls**:
* `struct wlr_scene_shadow *shadow_node` - SceneFX shadow for layer shells
* `layer_apply_criteria()` - Apply effects to layer surfaces

**Configuration Integration**:
* Layer shell surfaces can have individual effect settings
* Integration with layer_criteria system

## include/sway/output.h

**Purpose**: Output structure modifications for SceneFX blur optimization

**SceneFX Changes**:
* **Blur Implementation**: Added optimized blur layer to output structure

**Key Struct Modifications**:

```c
struct sway_output {
    struct {
        struct wlr_scene_tree *shell_background;
        struct wlr_scene_tree *shell_bottom;
        // Used for optimized blur. Everything exclusively below gets blurred
        struct wlr_scene_optimized_blur *blur_layer;
        struct wlr_scene_tree *tiling;
        struct wlr_scene_tree *fullscreen;
        struct wlr_scene_tree *shell_top;
        struct wlr_scene_tree *shell_overlay;
        struct wlr_scene_tree *session_lock;
    } layers;
    
    // ... rest of output structure
};
```

**New SceneFX API Calls**:
* `struct wlr_scene_optimized_blur *blur_layer` - SceneFX optimized blur node

**Configuration Integration**:
* Optimized blur integrated into scene graph layering
* Blur layer positioned between background and tiling layers

## include/sway/tree/container.h (Additional Analysis)

**Purpose**: Container helper functions for SceneFX effects

**SceneFX Changes**:

**New Helper Functions**:

```c
bool container_has_shadow(struct sway_container *con);
bool container_has_corner_radius(struct sway_container *con);
```

**New SceneFX API Calls**:
* Effect detection helper functions

## include/sway/tree/node.h

**Purpose**: Scene node allocation helpers for SceneFX

**SceneFX Changes**:
* **Shadow Implementation**: Added shadow allocation helper

**New Helper Functions**:

```c
struct wlr_scene_shadow *alloc_scene_shadow(struct wlr_scene_tree *parent,
        int width, int height, int corner_radius, float blur_sigma,
        const float color [static 4], bool *failed);
```

**New SceneFX API Calls**:
* `alloc_scene_shadow()` - SceneFX shadow node allocation with error handling

**Configuration Integration**:
* Safe shadow node allocation with failure detection

## include/scenefx/ (SceneFX API Headers)

**Purpose**: Complete SceneFX API definitions for effects and rendering

**SceneFX Changes**:
* **Renderer Integration**: FX renderer API definitions
* **Blur Implementation**: Blur data structures and render passes
* **Shadow Implementation**: Shadow scene nodes and rendering
* **Corner Radius**: Corner location enums and clipping
* **Dimming/Opacity**: Render pass options for effects

**Key API Structures**:

```c
// Blur data configuration
struct blur_data {
    int num_passes;
    int radius;
    float noise;
    float brightness;
    float contrast;
    float saturation;
};

// Corner location flags
enum corner_location {
    CORNER_LOCATION_NONE = 0,
    CORNER_LOCATION_TOP_LEFT = 1 << 0,
    CORNER_LOCATION_TOP_RIGHT = 1 << 1,
    CORNER_LOCATION_BOTTOM_RIGHT = 1 << 2,
    CORNER_LOCATION_BOTTOM_LEFT = 1 << 3,
    CORNER_LOCATION_ALL = /* all corners */,
};

// Scene node types
enum wlr_scene_node_type {
    WLR_SCENE_NODE_TREE,
    WLR_SCENE_NODE_RECT,
    WLR_SCENE_NODE_SHADOW,        // New
    WLR_SCENE_NODE_BUFFER,
    WLR_SCENE_NODE_OPTIMIZED_BLUR, // New
};
```

**New SceneFX API Calls**:
* `wlr_scene_shadow_create()` - Create shadow nodes
* `wlr_scene_optimized_blur_create()` - Create optimized blur
* `wlr_scene_rect_set_backdrop_blur()` - Enable backdrop blur
* `wlr_scene_buffer_set_corner_radius()` - Set corner radius
* `fx_render_pass_add_*()` - Custom render passes for effects

**Configuration Integration**:
* Complete API for all cosmetic effects
* Extensible blur and shadow configuration
* Scene graph integration for all effects

## include/render/fx_renderer/ (FX Renderer Headers)

**Purpose**: Custom renderer implementation for SceneFX effects

**SceneFX Changes**:
* **Renderer Integration**: Complete FX renderer replacing wlroots renderer
* **Blur Implementation**: Blur shaders and framebuffers
* **Shadow Implementation**: Box shadow shaders
* **Corner Radius**: Rounded rectangle shaders

**Key Renderer Structures**:

```c
struct fx_renderer {
    struct wlr_renderer wlr_renderer;
    // ... renderer internals
    
    struct {
        struct quad_shader quad;
        struct quad_round_shader quad_round;
        struct tex_shader tex_rgba;
        struct box_shadow_shader box_shadow;
        struct blur_shader blur1;
        struct blur_shader blur2;
        struct blur_effects_shader blur_effects;
    } shaders;
};
```

**New SceneFX API Calls**:
* `fx_renderer_create()` - Create FX renderer
* `fx_render_pass_add_blur()` - Render blur effects
* `fx_render_pass_add_box_shadow()` - Render shadows
* `fx_render_pass_add_rounded_rect()` - Render rounded rectangles

**Configuration Integration**:
* Shader-based implementation of all effects
* Optimized rendering pipeline for performance
* Framebuffer management for complex effects
