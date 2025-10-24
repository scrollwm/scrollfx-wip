# Buffer and Renderer Changes - Detailed Analysis
## Part 2: Rendering Pipeline and Buffer Handling

**Report Date**: 2025-10-24
**Focus**: Buffer rendering and pixel-perfect scaling changes
**Commits Analyzed**: 8+ buffer/renderer commits

---

## Critical Breaking Changes

### Change #1: Comprehensive Type Migration (int/float → double)
**Commit**: `872d9a45` - Improve mapping buffer sizes to on screen rendering
**Date**: 2025-09-13
**Severity**: ⚠️ **CRITICAL** - Massive API breakage
**Files Changed**: 21 files, +270/-223 lines

**What Broke**:
```c
// EVERYTHING changed from int/float to double precision

// Node positions
struct sway_scene_node {
    int x, y;              // BEFORE
    double x, y;           // AFTER
};

// Rectangle dimensions
struct sway_scene_rect {
    int width, height;      // BEFORE
    double width, height;   // AFTER
};

// Buffer destination size
struct sway_scene_buffer {
    int dst_width, dst_height;      // BEFORE
    double dst_width, dst_height;   // AFTER
};

// Scale in node info
struct sway_scene_node_info {
    float scale;           // BEFORE
    double scale;          // AFTER
};

// ALL function signatures changed:
void sway_scene_node_set_position(struct sway_scene_node *node,
    int x, int y);         // BEFORE
void sway_scene_node_set_position(struct sway_scene_node *node,
    double x, double y);   // AFTER

bool sway_scene_node_coords(struct sway_scene_node *node,
    int *lx, int *ly);     // BEFORE
bool sway_scene_node_coords(struct sway_scene_node *node,
    double *lx, double *ly); // AFTER

struct sway_scene_rect *sway_scene_rect_create(struct sway_scene_tree *parent,
    int width, int height, const float color[static 4]);  // BEFORE
struct sway_scene_rect *sway_scene_rect_create(struct sway_scene_tree *parent,
    double width, double height, const float color[static 4]); // AFTER

void sway_scene_rect_set_size(struct sway_scene_rect *rect,
    int width, int height);   // BEFORE
void sway_scene_rect_set_size(struct sway_scene_rect *rect,
    double width, double height); // AFTER

void sway_scene_buffer_set_dest_size(struct sway_scene_buffer *scene_buffer,
    int width, int height);   // BEFORE
void sway_scene_buffer_set_dest_size(struct sway_scene_buffer *scene_buffer,
    double width, double height); // AFTER

void scene_node_get_size(struct sway_scene_node *node,
    int *width, int *height);  // BEFORE
void scene_node_get_size(struct sway_scene_node *node,
    double *width, double *height); // AFTER

bool scene_node_get_parent_total_scale(struct sway_scene_node *node,
    float *scale);            // BEFORE
bool scene_node_get_parent_total_scale(struct sway_scene_node *node,
    double *scale);           // AFTER
```

**Impact on SceneFX**:
- **EVERY** SceneFX integration point that:
  - Creates scene nodes (rects, buffers)
  - Sets node positions
  - Gets node positions
  - Sets sizes/dimensions
  - Reads sizes/dimensions
  - Handles scale values
  - Passes position/size parameters

**Why This Matters**:
Scroll needs sub-pixel precision for smooth animations and exact workspace scaling. Integer positions cause jitter during animated movements. SceneFX's shadow offsets, blur regions, corner radius calculations - all must handle double precision.

**Migration Steps**:
1. **Global search-replace** in SceneFX code:
   - All `int x, y, width, height` → `double`
   - All `float scale` → `double scale`
   - Review all casting operations
   - Update printf format strings (%d → %f)

2. **Effect calculations**:
   - Shadow offset math: Already use float, should be compatible
   - Corner radius: Update any integer-based pixel calculations
   - Blur region: Ensure proper rounding when needed
   - Clipped regions: Update wlr_box usage (still uses int internally!)

3. **Watch for precision issues**:
   - When converting to `wlr_box` (which uses int), proper rounding needed
   - Shadow pixel perfect alignment may need floor/ceil operations
   - Blur sampling coordinates need careful handling

**Complexity**: **CRITICAL** - Affects 100% of SceneFX scene manipulation
**Estimated Effort**: 8-12 hours (careful review of every function)

---

### Change #2: New Buffer Properties (Color Management)
**Commit**: `e50b16a6` - tree/view: save new wlr_scene_buffer fields
**Related**: Color management commits
**Severity**: ⚠️ **HIGH** - New fields must be handled

**What Changed**:
```c
struct sway_scene_buffer {
    ...
    float opacity;                                    // Existed, now saved
    enum wlr_scale_filter_mode filter_mode;          // Existed, now saved
    struct wlr_fbox src_box;                         // Existed
    double dst_width, dst_height;                    // Type changed
    enum wl_output_transform transform;              // Existed
    pixman_region32_t opaque_region;                 // Existed
    enum wlr_color_transfer_function transfer_function; // NEW in current
    enum wlr_color_primaries primaries;              // NEW (inferred from save code)
};
```

**New fields that must be saved during transactions**:
- `opacity` - Alpha transparency (0.0-1.0)
- `filter_mode` - Texture filtering (nearest/linear)
- `transfer_function` - HDR/SDR color transfer (NEW field)
- `primaries` - Color gamut definition (NEW field)

**Impact on SceneFX**:
- SceneFX already uses opacity for dimming - **must preserve this**
- Blur effects may need to respect transfer_function for HDR
- Shadow colors may need color space awareness
- Corner radius rendering might interact with filter_mode

**Why This Matters**:
Scroll saves view state during transactions (animations). If SceneFX modifies these properties, they must be preserved across transactions or effects will flicker/reset during animations.

**Migration Steps**:
1. **In view save/restore code**: Ensure all SceneFX-modified buffer properties are saved
2. **Opacity conflicts**: SceneFX uses opacity for dim_inactive. Must not conflict with Scroll's usage
3. **Color management**: If SceneFX does custom rendering, respect color spaces
4. **Transaction safety**: Test that effects persist through Scroll's animations

**Complexity**: **High**
**Estimated Effort**: 4-6 hours + thorough testing

---

### Change #3: Buffer Rendering Improvements
**Commit**: `5e6501cd` - Improve buffer rendering by matching dst_size and src_box
**Commit**: `872d9a45` - Improve mapping buffer sizes... exact when workspace scale is 1
**Severity**: 🟡 **MEDIUM** - Logic changes in render path

**What Changed**:
Scroll now precisely calculates buffer source boxes and destination sizes to avoid blurry rendering when workspace scale = 1. The math for mapping texture coordinates to screen coordinates changed.

**Impact on SceneFX**:
- If SceneFX manipulates `src_box` or `dst_width/dst_height`, the new precision requirements must be met
- Blur effects that sample buffers need correct source box calculations
- Shadow rendering from buffers must use exact dimensions

**Migration Steps**:
1. Review any SceneFX code that sets buffer source/dest parameters
2. Ensure blur sampling uses exact buffer dimensions
3. Test at various scales (0.5, 1.0, 1.5, 2.0)

**Complexity**: **Medium**
**Estimated Effort**: 3-4 hours

---

### Change #4: Frame Done Event Structure
**Commit**: `eb8acfd7` - Stop using wlr_scene_buffer_send_frame_done()
**Severity**: 🟡 **MEDIUM** - API signature change

**What Changed**:
```c
// Buffer events changed
struct sway_scene_buffer {
    struct {
        ...
        struct wl_signal frame_done; // struct timespec (BEFORE)
        struct wl_signal frame_done; // struct sway_scene_frame_done_event (AFTER)
    } events;
};
```

The frame_done signal now passes a structured event instead of just a timespec.

**Impact on SceneFX**:
If SceneFX listens to frame_done events or sends custom frame callbacks, the event handling code must be updated.

**Migration Steps**:
1. Find any `frame_done` listeners in SceneFX
2. Update listener functions to expect new event structure
3. Use `wlr_scene_surface` functions instead of `wlr_scene_buffer` where possible

**Complexity**: **Low** (if no custom frame callbacks) / **Medium** (if custom handling)
**Estimated Effort**: 1-2 hours

---

## Related Rendering Changes

### Buffer Rendering Context Improvements

Multiple commits improved how buffers are rendered:

1. **32cb7264** - Simplify by calling surface_reconfigure to recreate buffers
   - Better animation step handling
   - Workspace scaling buffer recreation

2. **fd4a7e52** - Background render region reset for jump workspaces
   - Virtual workspace rendering fix
   - May affect layer shell backgrounds with effects

3. **7a7f0d2b** - Following wlroots surface_reconfigure() changes
   - Upstream API tracking

### Scale Handling

1. **de172012** - output: add 'exact' parameter to scale
   - Better content rendering
   - May affect how effects scale

2. **ded31ac3** - output: remove _exact_ option (later removed)
   - API churn

3. **fdab5e07** - output: support _exact_ parameter in subsurfaces
   - Subsurface scaling precision

4. **e1894e5b** - scene: fix function name from automatic merge
   - Scene API cleanup

---

## Files Most Affected by Buffer/Render Changes

### High-Impact Files (SceneFX Integration Points):

1. **sway/tree/scene/scene.c** - Core rendering logic
   - Type changes throughout
   - Buffer size calculations
   - Scale handling

2. **sway/tree/scene/surface.c** - Surface rendering
   - Buffer property handling
   - Frame callbacks

3. **sway/tree/view.c** - View management
   - Buffer property save/restore
   - Transaction handling

4. **sway/desktop/transaction.c** - Transaction system
   - 47 lines changed
   - Buffer state management

5. **sway/tree/container.c** - Container management
   - 60 lines changed
   - Position/size handling

6. **sway/tree/layout.c** - Layout calculations
   - 42 lines changed
   - Scale and positioning

---

## API Migration Checklist for SceneFX

### Buffer Node APIs:

| Old Signature | New Signature | Change |
|--------------|---------------|--------|
| `sway_scene_buffer_set_dest_size(buf, int w, int h)` | `sway_scene_buffer_set_dest_size(buf, double w, double h)` | int→double |

### Rect Node APIs:

| Old Signature | New Signature | Change |
|--------------|---------------|--------|
| `sway_scene_rect_create(parent, int w, int h, color)` | `sway_scene_rect_create(parent, double w, double h, color)` | int→double |
| `sway_scene_rect_set_size(rect, int w, int h)` | `sway_scene_rect_set_size(rect, double w, double h)` | int→double |

### Node Position APIs:

| Old Signature | New Signature | Change |
|--------------|---------------|--------|
| `sway_scene_node_set_position(node, int x, int y)` | `sway_scene_node_set_position(node, double x, double y)` | int→double |
| `sway_scene_node_coords(node, int *x, int *y)` | `sway_scene_node_coords(node, double *x, double *y)` | int*→double* |

### Size Query APIs:

| Old Signature | New Signature | Change |
|--------------|---------------|--------|
| `scene_node_get_size(node, int *w, int *h)` | `scene_node_get_size(node, double *w, double *h)` | int*→double* |
| `scene_node_get_parent_total_scale(node, float *s)` | `scene_node_get_parent_total_scale(node, double *s)` | float*→double* |

---

## Risk Assessment

### Critical Risks:

1. **Type conversion errors**: Missing int↔double conversions causing truncation
2. **Precision loss**: Effects using integer math where doubles needed
3. **Rounding errors**: Accumulating precision issues in effect calculations
4. **Transaction flickering**: Effects not preserved during Scroll animations

### High Risks:

1. **Color management conflicts**: HDR vs SDR rendering of effects
2. **Opacity conflicts**: SceneFX dim vs Scroll's opacity usage
3. **Scale precision**: Sub-pixel effects at fractional scales

### Medium Risks:

1. **Filter mode interaction**: Effects rendering with wrong filter
2. **Source box math**: Blur sampling with incorrect coordinates

---

## Testing Requirements

### Must Test:

1. **All scales**: 0.5, 0.75, 1.0, 1.25, 1.5, 2.0
2. **During animations**: Effects persist through movement/resize
3. **Workspace scaling**: Effects at various workspace scales
4. **HDR outputs**: If available, test color management
5. **Fractional positioning**: Sub-pixel effect alignment
6. **Transaction integrity**: No flickering during commits

---

## Recommendations

### High Priority:

1. ✅ **Accept the double precision migration** - Necessary for Scroll's smooth animations
2. ✅ **Update all SceneFX APIs** - Comprehensive type migration needed
3. ✅ **Preserve transaction state** - Save/restore all effect properties
4. ⚠️ **Test extensively at all scales** - Critical for quality

### Medium Priority:

1. 📋 **Review color management** - May need shader updates for HDR
2. 📋 **Audit opacity usage** - Ensure no conflicts with Scroll
3. 📋 **Precision testing** - Find any accumulated rounding errors

### Nice to Have:

1. 💡 **Leverage double precision** - Smoother effect animations possible
2. 💡 **HDR-aware effects** - Future-proof for wide color gamut

---

**Report Status**: Buffer/renderer analysis complete (8+ commits)
**Next Report**: Comprehensive API migration guide
