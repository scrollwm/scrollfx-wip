# Scene Graph API Changes - Detailed Analysis
## Part 1: Core Structure Changes

**Report Date**: 2025-10-24
**Focus**: Scene node API changes from wlroots 0.19 → 0.20dev
**Commits Analyzed**: 22 scene-related commits

---

## Critical Breaking Changes

### Change #1: Scene Node Info Structure Consolidation
**Commit**: `28258e73` - Add sway_scene_node_info to store all information about scene_node
**Date**: 2025-07-19
**Severity**: ⚠️ **CRITICAL** - Breaks all direct field access

**What Broke**:
```c
// Before (wlroots 0.19 / Sway 1.11)
struct sway_scene_node {
    enum sway_scene_node_type type;
    struct sway_scene_tree *parent;
    ...
    float scale;                      // Direct field
    struct wlr_output *wlr_output;    // Direct field
};

// After (wlroots 0.20dev / Current Scroll)
struct sway_scene_node {
    enum sway_scene_node_type type;
    struct sway_scene_tree *parent;
    ...
    struct sway_scene_node_info info; // Consolidated structure
};

struct sway_scene_node_info {
    double scale;                     // Type changed: float → double
    struct wlr_output *wlr_output;
    struct sway_workspace *workspace; // NEW field
    bool background;                  // NEW field (for wallpaper detection)
};
```

**Impact on SceneFX**:
- **Files affected**: All files that access `node->scale` or `node->wlr_output`
- **Estimated breakage**: Every scene node manipulation in SceneFX
- **Type change**: `float scale` → `double scale` requires careful casting

**Why This Matters**:
SceneFX heavily manipulates scene nodes to apply effects. Any code that reads or writes scale/output information will break. The addition of `workspace` and `background` fields suggests Scroll needs more context about nodes, which SceneFX effects must respect.

**Migration Steps**:
1. Replace all `node->scale` with `node->info.scale`
2. Replace all `node->wlr_output` with `node->info.wlr_output`
3. Update float→double conversions
4. Consider setting `node->info.workspace` and `node->info.background` appropriately for SceneFX nodes

**Complexity**: **High** - Pervasive change across all scene node code
**Estimated Effort**: 4-6 hours

---

### Change #2: Node Position Type Change
**Commit**: Multiple (gradual change)
**Severity**: ⚠️ **CRITICAL** - Type mismatch

**What Broke**:
```c
// Before
struct sway_scene_node {
    ...
    int x, y; // Integer positions
};

// After
struct sway_scene_node {
    ...
    double x, y; // Double-precision positions
};
```

**Impact on SceneFX**:
- Shadow positioning calculations
- Effect region calculations
- Any code setting node positions

**Why This Matters**:
Scroll needs sub-pixel positioning for smooth animations. SceneFX shadow offsets and effect regions must handle floating-point positions.

**Migration Steps**:
1. Change all `int x, y` to `double x, y` in position calculations
2. Review rounding/truncation logic
3. Update shadow offset math

**Complexity**: **Medium**
**Estimated Effort**: 2-3 hours

---

### Change #3: Frame Done API Change
**Commit**: `eb8acfd7` - Stop using wlr_scene_buffer_send_frame_done()
**Author**: Simon Ser (upstream wlroots)
**Date**: 2025-06-05
**Severity**: ⚠️ **CRITICAL** - API signature changed

**What Broke**:
```c
// Before
void wlr_scene_buffer_send_frame_done(
    struct wlr_scene_buffer *scene_buffer,
    const struct timespec *now
);

// After - function now requires output parameter
// Sway switched to using wlr_scene_surface variant instead
```

**Impact on SceneFX**:
If SceneFX sends custom frame callbacks for effects, this API change affects it.

**Why This Matters**:
Frame callbacks are critical for smooth rendering. The change suggests wlroots now needs to know which output is rendering the buffer.

**Migration Steps**:
1. Review SceneFX frame callback logic
2. Use `wlr_scene_surface` functions instead of `wlr_scene_buffer` where possible
3. Ensure output is available when sending frame done

**Complexity**: **Medium**
**Estimated Effort**: 2 hours

---

### Change #4: Color Management Support
**Commit**: `d9e615c5` - sway/server: set color_manager for root scene
**Commit**: `6fed1f9d` - Add support for color-management-v1
**Severity**: 🟡 **MEDIUM** - New feature, may conflict

**What Broke**:
```c
// Added to sway_scene structure
struct sway_scene {
    ...
    struct wlr_color_manager_v1 *color_manager_v1; // NEW

    struct {
        ...
        struct wl_listener color_manager_v1_destroy; // NEW
    };
};
```

**Impact on SceneFX**:
- Blur/dimming effects may need color management awareness
- Effect shaders might need color space conversions
- Shadow colors may need proper color space handling

**Why This Matters**:
Color management affects how colors are rendered. SceneFX's hardcoded RGBA values for shadows, dim colors, etc. may need color space considerations.

**Migration Steps**:
1. Review if SceneFX shaders respect color management
2. Check if shadow/dim colors need color space conversions
3. Test effects with color-managed outputs

**Complexity**: **Low** (if SceneFX doesn't do custom rendering) / **High** (if custom shaders)
**Estimated Effort**: 1-2 hours (investigation) / 8+ hours (shader updates)

---

### Change #5: Frame Pacing Output
**Commit**: Multiple (tracking wlroots)
**Severity**: 🟡 **MEDIUM** - New field in structure

**What Broke**:
```c
// Added to sway_scene_surface
struct sway_scene_surface {
    ...
    struct {
        ...
        // Output used for frame pacing (surface frame callbacks, etc)
        struct wlr_output *frame_pacing_output; // NEW
    };
};
```

**Impact on SceneFX**:
If SceneFX creates custom scene surfaces for effects, it needs to set this field.

**Migration Steps**:
1. When creating scene surfaces, set `frame_pacing_output`
2. Use the same output that will render the surface

**Complexity**: **Low**
**Estimated Effort**: 1 hour

---

## Summary of Scene Commits

Total scene-related commits: **22**

### Categories:

1. **Tracking wlroots updates** (12 commits):
   - `10ab73a3`, `c8bafa57`, `2a9d4984`, `fce186ed`, `f599f105`
   - `b75a6e10`, `33bc98a9`, `71c41aa6`, `9e95a44e`, `6fec10b5`
   - `41439555`, `9911f290`
   - Pattern: "scene: update to latest wlroots changes [SHA]"
   - These commits port upstream wlroots scene changes to Scroll's fork

2. **Scroll-specific improvements** (5 commits):
   - `28258e73` - Add sway_scene_node_info
   - `87598cc8` - Remove duplicated function
   - `ebf6ce8c` - Fix visible region leak
   - `fe6254d6` - update_node_update_outputs() use node info
   - `e1894e5b` - Fix function name after merge

3. **Upstream API changes** (5 commits):
   - `eb8acfd7` - Stop using wlr_scene_buffer_send_frame_done()
   - `c2f08075` - send event unconditionally in view_send_frame_done()
   - `d9e615c5` - Set color_manager for root scene
   - `1c1627fa` - node_at_coords() cursor verification
   - Plus color management additions

---

## Files Modified by Scene Changes

Based on commit analysis:

### Core Scene Files (Heavy Changes):
- `include/sway/tree/scene.h` - Core API definitions
- `sway/tree/scene/scene.c` - Main scene implementation
- `sway/tree/scene/surface.c` - Surface handling
- `sway/tree/scene/output.c` - Output management (NEW file in some commits)

### Integration Points (SceneFX Impact):
- `sway/desktop/output.c` - Render pipeline
- `sway/tree/view.c` - View/container rendering
- `sway/tree/layout.c` - Layout and arrangement
- `sway/tree/output.c` - Output handling

### New Files Added:
- `sway/tree/scene/ext_image_capture_source_v1.c` (325 lines) - Screen capture
- `sway/tree/scene/output.c` - Output management functions
- `include/sway/ext_image_capture_source_v1.h` - Capture API

---

## Risk Assessment

### High Risk Areas:

1. **Scene Node Info Structure** - Affects every scene node access
2. **Position Type Change** - int→double affects all positioning
3. **Scale Type Change** - float→double affects all scaling
4. **Frame Done API** - Breaking change in callback system

### Medium Risk Areas:

1. **Color Management** - May affect effect rendering
2. **Frame Pacing** - New field to initialize
3. **Visibility Calculations** - Changed in multiple commits

### Low Risk Areas:

1. **Image Capture** - Additive feature, doesn't affect effects
2. **Function Renaming** - Simple refactors

---

## Next Steps for Analysis

1. ✅ Analyzed core scene structure changes
2. 🔄 **NEXT**: Examine render pipeline changes (buffer/texture handling)
3. ⏳ Map SceneFX integration points to changed APIs
4. ⏳ Create comprehensive API translation guide
5. ⏳ Assess complexity and create roadmap

---

**Report Status**: Scene graph analysis complete (22/22 commits)
**Next Report**: Renderer and buffer handling changes
