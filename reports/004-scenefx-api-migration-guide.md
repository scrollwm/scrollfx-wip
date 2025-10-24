# SceneFX to Scroll API Migration Guide
## Comprehensive Mapping of Integration Points to API Changes

**Report Date**: 2025-10-24
**Purpose**: Detailed guide for migrating each SceneFX API call to Scroll's current API
**Based On**: SceneFX Integration Surface Area + Scroll API analysis

---

## Executive Summary

This guide maps every SceneFX integration point (documented in the quick reference) to the corresponding API changes in Scroll. Each section addresses a specific SceneFX feature and lists the exact changes needed.

**Key Change Categories**:
1. **Type conversions**: int/float → double (affects 100% of APIs)
2. **Structure changes**: Direct fields → info struct
3. **New fields**: Color management, frame pacing
4. **Function signatures**: All position/size APIs changed

---

## Part 1: Scene Node Creation APIs

### 1.1 Shadow Node Creation (SceneFX Custom)

**SceneFX API (from swayfx)**:
```c
// SwayFX creates custom shadow scene nodes
struct wlr_scene_shadow *wlr_scene_shadow_create(
    struct wlr_scene_tree *parent
);

// Sway wrapper (documented in quick reference)
struct wlr_scene_shadow *alloc_scene_shadow(
    struct wlr_scene_tree *parent,
    int width, int height,              // OLD: int
    int corner_radius,                   // OLD: int
    float blur_sigma,                    // OLD: float
    const float color[static 4],
    bool *failed
);
```

**Required Changes for Scroll**:
```c
// Scroll uses sway_scene_* not wlr_scene_*
struct sway_scene_shadow *sway_scene_shadow_create(
    struct sway_scene_tree *parent
);

// Updated signature with double precision
struct sway_scene_shadow *alloc_scene_shadow(
    struct sway_scene_tree *parent,
    double width, double height,         // NEW: double
    double corner_radius,                // NEW: double (or int if pixel-based)
    double blur_sigma,                   // NEW: double
    const float color[static 4],         // Unchanged
    bool *failed
);
```

**Migration Steps**:
1. Replace `wlr_scene_shadow` → `sway_scene_shadow`
2. Update all width/height/radius parameters to double
3. Update blur_sigma to double if needed
4. Review shadow positioning math for double precision

**Files Affected**: `include/sway/tree/node.h`, `sway/tree/node.c`

---

### 1.2 Optimized Blur Layer Creation (SceneFX Custom)

**SceneFX API**:
```c
struct wlr_scene_optimized_blur *wlr_scene_optimized_blur_create(
    struct wlr_scene_tree *parent
);
```

**Required Changes for Scroll**:
```c
struct sway_scene_optimized_blur *sway_scene_optimized_blur_create(
    struct sway_scene_tree *parent
);
```

**Migration Steps**:
1. Replace `wlr_scene_*` → `sway_scene_*`
2. Note: This is a custom SceneFX node type, needs to be added to Scroll's scene types
3. Must add to `enum sway_scene_node_type` if not present

**Files Affected**: `include/sway/tree/scene.h`, scene creation files

---

## Part 2: Buffer Effect APIs

### 2.1 Corner Radius

**SceneFX API**:
```c
void wlr_scene_buffer_set_corner_radius(
    struct wlr_scene_buffer *buffer,
    int radius,                          // OLD: int
    enum corner_location corners
);
```

**Required Changes for Scroll**:
```c
void sway_scene_buffer_set_corner_radius(
    struct sway_scene_buffer *buffer,
    double radius,                       // NEW: double (or keep int?)
    enum corner_location corners
);
```

**Migration Considerations**:
- Corner radius is typically in pixels, may stay as int
- BUT: With double precision positions, fractional radius might make sense
- Scroll's animations could benefit from smooth radius transitions
- **Recommendation**: Use double for consistency, round when rendering

**Files Affected**: SceneFX buffer effect implementation

---

### 2.2 Backdrop Blur

**SceneFX APIs**:
```c
void wlr_scene_buffer_set_backdrop_blur(
    struct wlr_scene_buffer *buffer,
    bool enabled
);

void wlr_scene_buffer_set_backdrop_blur_optimized(
    struct wlr_scene_buffer *buffer,
    bool optimized
);

void wlr_scene_buffer_set_backdrop_blur_ignore_transparent(
    struct wlr_scene_buffer *buffer,
    bool ignore
);
```

**Required Changes for Scroll**:
```c
void sway_scene_buffer_set_backdrop_blur(
    struct sway_scene_buffer *buffer,   // sway_scene_buffer
    bool enabled
);

void sway_scene_buffer_set_backdrop_blur_optimized(
    struct sway_scene_buffer *buffer,   // sway_scene_buffer
    bool optimized
);

void sway_scene_buffer_set_backdrop_blur_ignore_transparent(
    struct sway_scene_buffer *buffer,   // sway_scene_buffer
    bool ignore
);
```

**Migration Steps**:
1. Replace `wlr_scene_buffer` → `sway_scene_buffer`
2. No type changes needed (bool parameters)
3. Ensure blur state is preserved during Scroll's transactions
4. Test with Scroll's workspace scaling

**Critical Note**: These properties MUST be saved/restored in `view_save_buffer_iterator()` to survive Scroll's animations! See commit e50b16a6 for pattern.

**Files Affected**: Buffer creation, transaction save/restore

---

### 2.3 Opacity

**SceneFX API**:
```c
void wlr_scene_buffer_set_opacity(
    struct wlr_scene_buffer *buffer,
    float opacity                       // OLD: float
);
```

**Required Changes for Scroll**:
```c
void sway_scene_buffer_set_opacity(
    struct sway_scene_buffer *buffer,   // sway_scene_buffer
    float opacity                       // float is OK for 0.0-1.0
);
```

**Migration Considerations**:
- Scroll already uses opacity (commit e50b16a6 saves it)
- SceneFX uses opacity for dim_inactive
- **CONFLICT RISK**: Must ensure SceneFX dim doesn't override Scroll's opacity
- **Solution**: SceneFX should read current opacity and multiply rather than replace

**Critical**:
```c
// WRONG: Overwrites Scroll's opacity
wlr_scene_buffer_set_opacity(buffer, 0.7);

// RIGHT: Preserves Scroll's opacity
float current = buffer->opacity;
wlr_scene_buffer_set_opacity(buffer, current * 0.7);
```

**Files Affected**: Dim implementation, transaction code

---

## Part 3: Rectangle Effect APIs

### 3.1 Rect Corner Radius

**SceneFX API**:
```c
void wlr_scene_rect_set_corner_radius(
    struct wlr_scene_rect *rect,
    int radius,                          // OLD: int
    enum corner_location corners
);
```

**Required Changes for Scroll**:
```c
void sway_scene_rect_set_corner_radius(
    struct sway_scene_rect *rect,       // sway_scene_rect
    double radius,                       // NEW: double
    enum corner_location corners
);
```

**Structural Note**: Rect itself changed:
```c
// OLD
struct sway_scene_rect {
    struct sway_scene_node node;
    int width, height;                   // OLD: int
    float color[4];
};

// NEW
struct sway_scene_rect {
    struct sway_scene_node node;
    double width, double height;         // NEW: double
    float color[4];
};
```

**Migration Steps**:
1. Replace `wlr_scene_rect` → `sway_scene_rect`
2. Update radius to double
3. When creating rects, use double for width/height

**Files Affected**: Border rendering, title bar rendering, dim rects

---

### 3.2 Rect Backdrop Blur

**SceneFX API**:
```c
void wlr_scene_rect_set_backdrop_blur(
    struct wlr_scene_rect *rect,
    bool enabled
);
```

**Required Changes for Scroll**:
```c
void sway_scene_rect_set_backdrop_blur(
    struct sway_scene_rect *rect,       // sway_scene_rect
    bool enabled
);
```

**Migration Steps**:
1. Replace `wlr_scene_rect` → `sway_scene_rect`
2. No other changes (bool parameter)

**Files Affected**: Rect effect implementation

---

### 3.3 Clipped Region

**SceneFX API**:
```c
void wlr_scene_rect_set_clipped_region(
    struct wlr_scene_rect *rect,
    struct clipped_region *region        // Contains int values
);

struct clipped_region {
    int corner_radius;                   // OLD: int
    enum corner_location corners;
    struct wlr_box area;                 // wlr_box uses int x,y,width,height
};
```

**Required Changes for Scroll**:
```c
void sway_scene_rect_set_clipped_region(
    struct sway_scene_rect *rect,       // sway_scene_rect
    struct clipped_region *region
);

struct clipped_region {
    double corner_radius;                // NEW: double
    enum corner_location corners;
    struct wlr_box area;                 // Still uses int!
};
```

**Critical Issue**: `wlr_box` still uses int:
```c
struct wlr_box {
    int x, y, width, height;
};
```

**Migration Problem**: Scroll uses double positions internally, but wlr_box uses int. Clipping must convert:
```c
// When setting clipped region with double positions:
struct wlr_box clip_box = {
    .x = (int)round(double_x),          // Round to nearest pixel
    .y = (int)round(double_y),
    .width = (int)round(double_width),
    .height = (int)round(double_height),
};
```

**Files Affected**: Shadow clipping, border clipping, effect regions

---

## Part 4: Shadow Management APIs

### 4.1 Shadow Size

**SceneFX API**:
```c
void wlr_scene_shadow_set_size(
    struct wlr_scene_shadow *shadow,
    int width, int height                // OLD: int
);
```

**Required Changes for Scroll**:
```c
void sway_scene_shadow_set_size(
    struct sway_scene_shadow *shadow,   // sway_scene_shadow
    double width, double height          // NEW: double
);
```

**Migration Steps**:
1. Replace `wlr_scene_shadow` → `sway_scene_shadow`
2. Update width/height to double
3. Shadow must follow parent container's double precision

**Files Affected**: `sway/desktop/transaction.c` (arrange_container)

---

### 4.2 Shadow Color

**SceneFX API**:
```c
void wlr_scene_shadow_set_color(
    struct wlr_scene_shadow *shadow,
    const float color[4]                 // RGBA, no change
);
```

**Required Changes for Scroll**:
```c
void sway_scene_shadow_set_color(
    struct sway_scene_shadow *shadow,   // sway_scene_shadow
    const float color[4]
);
```

**Color Management Consideration**:
- Scroll added color management (color_manager_v1)
- Shadow colors are hardcoded RGBA values
- **Question**: Do shadow colors need color space conversion?
- **Likely**: No, shadows are compositor-rendered decorations, not client content

**Migration Steps**:
1. Replace `wlr_scene_shadow` → `sway_scene_shadow`
2. No type changes needed
3. Consider if HDR outputs need special handling

**Files Affected**: Shadow color switching (focused/unfocused/urgent)

---

### 4.3 Shadow Blur and Corner Radius

**SceneFX APIs**:
```c
void wlr_scene_shadow_set_blur_sigma(
    struct wlr_scene_shadow *shadow,
    float sigma                          // OLD: float
);

void wlr_scene_shadow_set_corner_radius(
    struct wlr_scene_shadow *shadow,
    int radius                           // OLD: int
);
```

**Required Changes for Scroll**:
```c
void sway_scene_shadow_set_blur_sigma(
    struct sway_scene_shadow *shadow,   // sway_scene_shadow
    double sigma                         // NEW: double for consistency
);

void sway_scene_shadow_set_corner_radius(
    struct sway_scene_shadow *shadow,   // sway_scene_shadow
    double radius                        // NEW: double
);
```

**Migration Steps**:
1. Replace `wlr_scene_shadow` → `sway_scene_shadow`
2. Update numeric types to double
3. Blur sigma calculations should already use floating point

**Files Affected**: Shadow configuration, transaction code

---

### 4.4 Shadow Clipped Region

**SceneFX API**:
```c
void wlr_scene_shadow_set_clipped_region(
    struct wlr_scene_shadow *shadow,
    struct clipped_region *region        // See clipped_region above
);
```

**Required Changes for Scroll**:
```c
void sway_scene_shadow_set_clipped_region(
    struct sway_scene_shadow *shadow,   // sway_scene_shadow
    struct clipped_region *region
);
```

**Same considerations as rect clipped region** (see 3.3 above)

**Files Affected**: `sway/desktop/transaction.c` (arrange_container shadow clipping)

---

## Part 5: Blur Management APIs

### 5.1 Scene Blur Data

**SceneFX API**:
```c
void wlr_scene_set_blur_data(
    struct wlr_scene *scene,
    struct blur_data *blur_data
);

struct blur_data {
    int num_passes;
    int radius;
    float noise;
    float brightness;
    float contrast;
    float saturation;
};
```

**Required Changes for Scroll**:
```c
void sway_scene_set_blur_data(
    struct sway_scene *scene,           // sway_scene
    struct blur_data *blur_data
);

struct blur_data {
    int num_passes;                     // int OK (discrete value)
    int radius;                          // int OK (pixels)
    float noise;                         // float OK (0.0-1.0 range)
    float brightness;                    // float OK
    float contrast;                      // float OK
    float saturation;                    // float OK
};
```

**Migration Steps**:
1. Replace `wlr_scene` → `sway_scene`
2. No type changes needed in blur_data (all reasonable as-is)
3. Ensure blur shader respects these parameters

**Files Affected**: Blur configuration, root scene initialization

---

### 5.2 Optimized Blur Management

**SceneFX APIs**:
```c
void wlr_scene_optimized_blur_mark_dirty(
    struct wlr_scene_optimized_blur *blur_layer
);

void wlr_scene_optimized_blur_set_size(
    struct wlr_scene_optimized_blur *blur_layer,
    int width, int height                // OLD: int
);
```

**Required Changes for Scroll**:
```c
void sway_scene_optimized_blur_mark_dirty(
    struct sway_scene_optimized_blur *blur_layer  // sway_scene_optimized_blur
);

void sway_scene_optimized_blur_set_size(
    struct sway_scene_optimized_blur *blur_layer,
    double width, double height          // NEW: double
);
```

**Migration Steps**:
1. Replace `wlr_scene_optimized_blur` → `sway_scene_optimized_blur`
2. Update size parameters to double
3. Blur layer sizing must match Scroll's double-precision output dimensions

**Files Affected**: `sway/tree/arrange.c`, `sway/tree/output.c` (blur layer size updates)

---

## Part 6: Scene Node Manipulation (Core APIs)

### 6.1 Node Position

**SceneFX Usage**:
```c
// Setting shadow position relative to container
wlr_scene_node_set_position(&shadow->node, x_offset, y_offset);
```

**API Change**:
```c
// OLD
void sway_scene_node_set_position(
    struct sway_scene_node *node,
    int x, int y                         // OLD: int
);

// NEW
void sway_scene_node_set_position(
    struct sway_scene_node *node,
    double x, double y                   // NEW: double
);
```

**Migration Steps**:
1. All position variables must be double
2. Shadow offsets (config->shadow_offset_x/y) should be double
3. Dim rect positioning must use double
4. Review all arithmetic for proper types

**Example Migration**:
```c
// OLD CODE
int shadow_x = container->x + config->shadow_offset_x;
int shadow_y = container->y + config->shadow_offset_y;
sway_scene_node_set_position(&shadow->node, shadow_x, shadow_y);

// NEW CODE
double shadow_x = container->content_x + config->shadow_offset_x;
double shadow_y = container->content_y + config->shadow_offset_y;
sway_scene_node_set_position(&shadow->node, shadow_x, shadow_y);
```

**Files Affected**: Every file that positions effect nodes

---

### 6.2 Node Coordinates

**SceneFX Usage**:
```c
// Getting absolute position of a node
int abs_x, abs_y;
sway_scene_node_coords(&node, &abs_x, &abs_y);
```

**API Change**:
```c
// OLD
bool sway_scene_node_coords(
    struct sway_scene_node *node,
    int *lx, int *ly                     // OLD: int*
);

// NEW
bool sway_scene_node_coords(
    struct sway_scene_node *node,
    double *lx, double *ly               // NEW: double*
);
```

**Migration Steps**:
1. All coordinate variables must be double pointers
2. Update printf format strings

**Example Migration**:
```c
// OLD CODE
int abs_x, abs_y;
sway_scene_node_coords(&effect_node->node, &abs_x, &abs_y);
printf("Effect at %d, %d\n", abs_x, abs_y);

// NEW CODE
double abs_x, abs_y;
sway_scene_node_coords(&effect_node->node, &abs_x, &abs_y);
printf("Effect at %.2f, %.2f\n", abs_x, abs_y);
```

**Files Affected**: Effect positioning calculations

---

### 6.3 Node Scale Access

**SceneFX Usage**:
```c
// Reading node scale for effect calculations
float scale = node->scale;
node->scale = new_scale;
```

**Structure Change**:
```c
// OLD
struct sway_scene_node {
    ...
    float scale;                         // Direct field
    struct wlr_output *wlr_output;       // Direct field
};

// NEW
struct sway_scene_node {
    ...
    struct sway_scene_node_info info;    // Consolidated
};

struct sway_scene_node_info {
    double scale;                        // Type changed too
    struct wlr_output *wlr_output;
    struct sway_workspace *workspace;    // New
    bool background;                     // New
};
```

**Migration Steps**:
1. Replace `node->scale` → `node->info.scale`
2. Replace `node->wlr_output` → `node->info.wlr_output`
3. Update type from float to double
4. Consider setting `node->info.workspace` and `node->info.background` for SceneFX nodes

**Example Migration**:
```c
// OLD CODE
float parent_scale = parent_node->scale;
float effect_scale = view_scale * parent_scale;
effect_node->scale = effect_scale;

// NEW CODE
double parent_scale = parent_node->info.scale;
double effect_scale = view_scale * parent_scale;
effect_node->info.scale = effect_scale;
effect_node->info.workspace = parent_workspace;  // Set if known
effect_node->info.background = false;            // Effects aren't background
```

**Files Affected**: EVERY file that accesses node scale or output

---

## Part 7: Scene Node Creation

### 7.1 Rect Creation

**SceneFX Usage**:
```c
// Creating dim rect
struct wlr_scene_rect *dim_rect = wlr_scene_rect_create(
    parent, width, height, dim_color
);
```

**API Change**:
```c
// OLD
struct sway_scene_rect *sway_scene_rect_create(
    struct sway_scene_tree *parent,
    int width, int height,               // OLD: int
    const float color[static 4]
);

// NEW
struct sway_scene_rect *sway_scene_rect_create(
    struct sway_scene_tree *parent,
    double width, double height,         // NEW: double
    const float color[static 4]
);
```

**Migration Steps**:
1. Replace `wlr_scene_rect` → `sway_scene_rect`
2. Width/height must be double
3. Ensure dimensions match container's double precision

**Example Migration**:
```c
// OLD CODE
int con_width = container->width;
int con_height = container->height;
struct wlr_scene_rect *dim_rect = wlr_scene_rect_create(
    container->content_tree, con_width, con_height, dim_color
);

// NEW CODE
double con_width = container->width;   // Assuming container struct uses double
double con_height = container->height;
struct sway_scene_rect *dim_rect = sway_scene_rect_create(
    container->content_tree, con_width, con_height, dim_color
);
```

**Files Affected**: `sway/tree/container.c` (dim rect creation)

---

### 7.2 Rect Size Change

**SceneFX Usage**:
```c
// Resizing dim rect to match container
wlr_scene_rect_set_size(container->dim_rect, new_width, new_height);
```

**API Change**:
```c
// OLD
void sway_scene_rect_set_size(
    struct sway_scene_rect *rect,
    int width, int height                // OLD: int
);

// NEW
void sway_scene_rect_set_size(
    struct sway_scene_rect *rect,
    double width, double height          // NEW: double
);
```

**Migration Steps**:
1. Width/height parameters must be double
2. Usually called during transactions/arrangements

**Example Migration**:
```c
// OLD CODE in transaction
int new_w = container->pending.width;
int new_h = container->pending.height;
sway_scene_rect_set_size(container->dim_rect, new_w, new_h);

// NEW CODE
double new_w = container->pending.width;
double new_h = container->pending.height;
sway_scene_rect_set_size(container->dim_rect, new_w, new_h);
```

**Files Affected**: `sway/desktop/transaction.c` (arrange_container, dim rect sizing)

---

## Part 8: Scene Query APIs

### 8.1 Get Node Size

**SceneFX Usage**:
```c
// Getting size of effect node
int width, height;
scene_node_get_size(effect_node, &width, &height);
```

**API Change**:
```c
// OLD
void scene_node_get_size(
    struct sway_scene_node *node,
    int *width, int *height              // OLD: int*
);

// NEW
void scene_node_get_size(
    struct sway_scene_node *node,
    double *width, double *height        // NEW: double*
);
```

**Migration Steps**:
1. Size variables must be double
2. Update all call sites

**Example Migration**:
```c
// OLD CODE
int shadow_w, shadow_h;
scene_node_get_size(&shadow->node, &shadow_w, &shadow_h);

// NEW CODE
double shadow_w, shadow_h;
scene_node_get_size(&shadow->node, &shadow_w, &shadow_h);
```

**Files Affected**: Effect sizing logic

---

### 8.2 Get Parent Total Scale

**SceneFX Usage**:
```c
// Getting cumulative scale from parent hierarchy
float total_scale;
scene_node_get_parent_total_scale(node, &total_scale);
```

**API Change**:
```c
// OLD
bool scene_node_get_parent_total_scale(
    struct sway_scene_node *node,
    float *scale                         // OLD: float*
);

// NEW
bool scene_node_get_parent_total_scale(
    struct sway_scene_node *node,
    double *scale                        // NEW: double*
);
```

**Migration Steps**:
1. Scale variable must be double
2. Used for calculating effective scale of effects

**Example Migration**:
```c
// OLD CODE
float parent_scale;
if (scene_node_get_parent_total_scale(&effect_node->node, &parent_scale)) {
    float effective_radius = base_radius * parent_scale;
}

// NEW CODE
double parent_scale;
if (scene_node_get_parent_total_scale(&effect_node->node, &parent_scale)) {
    double effective_radius = base_radius * parent_scale;
}
```

**Files Affected**: Effect scaling calculations

---

## Part 9: Buffer Creation and Configuration

### 9.1 Buffer Destination Size

**SceneFX Usage**:
```c
// Setting render size for effect buffer
sway_scene_buffer_set_dest_size(effect_buffer, target_width, target_height);
```

**API Change**:
```c
// OLD
void sway_scene_buffer_set_dest_size(
    struct sway_scene_buffer *scene_buffer,
    int width, int height                // OLD: int
);

// NEW
void sway_scene_buffer_set_dest_size(
    struct sway_scene_buffer *scene_buffer,
    double width, double height          // NEW: double
);
```

**Migration Steps**:
1. Width/height must be double
2. Critical for pixel-perfect rendering at various scales

**Example Migration**:
```c
// OLD CODE
int buf_width = (int)(container_width * scale);
int buf_height = (int)(container_height * scale);
sway_scene_buffer_set_dest_size(buffer, buf_width, buf_height);

// NEW CODE
double buf_width = container_width * scale;    // Keep precision!
double buf_height = container_height * scale;
sway_scene_buffer_set_dest_size(buffer, buf_width, buf_height);
```

**Files Affected**: Effect buffer creation

---

## Part 10: Transaction Save/Restore (Critical!)

### 10.1 Saving Buffer State

**SceneFX Must Do**:
All effect properties set on buffers MUST be saved during Scroll's transactions to survive animations.

**Pattern from commit e50b16a6**:
```c
// In view_save_buffer_iterator() or equivalent
static void save_buffer_with_effects(
    struct wlr_scene_buffer *buffer,
    struct wlr_scene_buffer *saved_buffer,
    int sx, int sy, void *data
) {
    // Save standard properties
    sway_scene_buffer_set_dest_size(saved_buffer,
        buffer->dst_width, buffer->dst_height);
    sway_scene_buffer_set_opaque_region(saved_buffer, &buffer->opaque_region);

    // MUST SAVE THESE (added in e50b16a6):
    sway_scene_buffer_set_opacity(saved_buffer, buffer->opacity);
    sway_scene_buffer_set_filter_mode(saved_buffer, buffer->filter_mode);
    sway_scene_buffer_set_transfer_function(saved_buffer, buffer->transfer_function);
    sway_scene_buffer_set_primaries(saved_buffer, buffer->primaries);

    // SceneFX MUST ADD:
    sway_scene_buffer_set_corner_radius(saved_buffer,
        buffer->corner_radius, buffer->corner_location);
    sway_scene_buffer_set_backdrop_blur(saved_buffer, buffer->blur_enabled);
    sway_scene_buffer_set_backdrop_blur_optimized(saved_buffer, buffer->blur_optimized);
    sway_scene_buffer_set_backdrop_blur_ignore_transparent(saved_buffer,
        buffer->blur_ignore_transparent);

    // Continue with other properties...
    sway_scene_buffer_set_source_box(saved_buffer, &buffer->src_box);
    sway_scene_node_set_position(&saved_buffer->node, sx, sy);
    sway_scene_buffer_set_transform(saved_buffer, buffer->transform);
}
```

**Critical**: Without proper save/restore, effects will flicker or reset during:
- Window movement animations
- Resizing animations
- Workspace animations
- Focus changes with animations

**Files Affected**: `sway/tree/view.c` (view_save_buffer_iterator)

---

## Part 11: Output Configure Scene (Core Integration)

### 11.1 The output_configure_scene Function

This is THE most critical SceneFX integration point. Documented in quick reference with these parameters:

**SwayFX Signature**:
```c
void output_configure_scene(
    struct sway_output *output,
    struct sway_scene_node *node,
    float opacity,                       // OLD: float
    int corner_radius,                   // OLD: int
    bool blur_enabled,
    bool has_titlebar,
    struct sway_container *closest_con
);
```

**Scroll Signature (Updated)**:
```c
void output_configure_scene(
    struct sway_output *output,
    struct sway_scene_node *node,
    double opacity,                      // NEW: double (maybe, or stay float for 0-1)
    double corner_radius,                // NEW: double
    bool blur_enabled,
    bool has_titlebar,
    struct sway_container *closest_con
);
```

**This Function**:
1. Recursively walks scene tree
2. Inherits effect properties from containers
3. Applies effects to buffers based on node type
4. Configures layer surfaces
5. Makes XRay blur optimization decisions

**Migration Checklist for output_configure_scene**:
- [ ] Update signature to double precision
- [ ] When calling `sway_scene_buffer_set_corner_radius()`, use double
- [ ] When calling `sway_scene_rect_set_corner_radius()`, use double
- [ ] Access `node->info.scale` instead of `node->scale`
- [ ] Access `node->info.wlr_output` instead of `node->wlr_output`
- [ ] Consider checking `node->info.workspace` for additional context
- [ ] Consider checking `node->info.background` for wallpaper handling
- [ ] Ensure effect property inheritance uses double precision math
- [ ] Test with workspace scaling (view scale * workspace scale)

**Files Affected**: `sway/desktop/output.c`

---

## Part 12: Scroll-Specific Considerations

### 12.1 Workspace Scaling

**Scroll Feature**: Can scale entire workspace (overview mode, manual scaling)

**SceneFX Implications**:
- Effects must scale correctly with workspace
- Blur radius might need adjustment at different scales
- Shadow offsets might need scaling
- Corner radius should probably scale

**Required Handling**:
```c
// When applying effects, consider workspace scale
double workspace_scale = layout_scale_get(workspace);
double effective_corner_radius = config->corner_radius * workspace_scale;
double effective_shadow_offset_x = config->shadow_offset_x * workspace_scale;
// Apply scaled values...
```

**Files Affected**: Effect application logic

---

### 12.2 Content Scaling

**Scroll Feature**: Individual window content can be scaled independently

**SceneFX Implications**:
- Effects applied to buffers must respect content scale
- Shadow size should match scaled content
- Blur regions must account for content scale
- Corner radius scaling decision needed

**Consider**:
```c
// Is content scaling handled by view scale or buffer dst_size?
// SceneFX effects might need to read the effective content scale
// to size shadows/blur regions correctly
```

---

### 12.3 Animations

**Scroll Feature**: Customizable Bezier curve animations for movement/resize/etc

**SceneFX Implications**:
- Effects MUST persist through animations (transaction save/restore critical!)
- Shadow positions must update during movement animations
- Blur layers must resize during window animations
- No flicker or property resets

**Critical Requirements**:
1. All effect properties saved in transaction
2. Effect nodes properly linked to containers
3. Effect updates in arrange functions
4. Test ALL animation types

---

## Part 13: New Scroll Features to Integrate

### 13.1 Color Management

**Added**: `struct wlr_color_manager_v1 *color_manager_v1` to scene

**Questions for SceneFX**:
1. Do effect shaders need color space awareness?
2. Should shadow colors be in a specific color space?
3. Do blur shaders need color space conversions?
4. Is HDR support needed for effects?

**Recommendation**:
- Start with assumption effects are in sRGB
- Test on HDR outputs if available
- May need shader updates for proper HDR

---

### 13.2 Frame Pacing

**Added**: `struct wlr_output *frame_pacing_output` to scene surfaces

**SceneFX Action**:
When creating scene surfaces for effects, set frame_pacing_output to the appropriate output.

---

## Part 14: Testing Requirements

### After Migration, Test:

1. **All Scales**:
   - Output scale: 1.0, 1.5, 2.0
   - Workspace scale: 0.5, 1.0, 1.5
   - Content scale: 0.5, 1.0, 1.5, 2.0
   - Combined: Output 2.0 + Workspace 0.5 + Content 1.5

2. **All Animations**:
   - Window movement (effects persist)
   - Window resize (effects resize)
   - Workspace scale (overview mode)
   - Focus changes
   - Workspace switches

3. **Effect Combinations**:
   - Blur + corner radius + shadow
   - Blur + dim + shadow
   - Opacity + corner radius
   - Layer shell with effects

4. **Edge Cases**:
   - Multiple outputs
   - Mixed fractional scales
   - Fullscreen windows
   - Tabbed/stacked layouts
   - Floating windows

---

## Part 15: Migration Effort Estimate

### By Integration Point:

| Integration Point | Complexity | Estimated Hours |
|------------------|-----------|----------------|
| Scene node info structure | High | 4-6 |
| Position APIs (int→double) | High | 6-8 |
| Size APIs (int→double) | High | 4-6 |
| Scale access (field→info) | High | 3-4 |
| Buffer effect APIs | Medium | 3-4 |
| Rect effect APIs | Medium | 2-3 |
| Shadow APIs | Medium | 3-4 |
| Blur APIs | Medium | 2-3 |
| Transaction save/restore | High | 6-8 |
| output_configure_scene | Critical | 8-12 |
| Testing & debugging | High | 16-24 |
| **TOTAL** | | **57-82 hours** |

**Range**: 57-82 hours = **7-10 working days**

---

## Conclusion

**GO Recommendation**: Migration is feasible but requires careful attention to:
1. Universal type migration (int/float → double)
2. Structure access changes (direct → info struct)
3. Transaction integrity (save/restore all effects)
4. Comprehensive testing at all scale combinations

**Success depends on**:
- Methodical, file-by-file migration
- Not missing any API call sites
- Thorough transaction handling
- Extensive testing with Scroll's unique features

---

**Report Status**: API migration guide complete
**Next Report**: Implementation roadmap with phased approach
