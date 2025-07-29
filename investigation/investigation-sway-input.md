# Part 1/2:

Looking at the `/sway/input` directory files provided, there are **no SceneFX-specific changes** in the input subsystem. The files contain standard Sway input handling code without any cosmetic effect integration.

**Analysis Summary**:

The input files (`cursor.c`, `input-manager.c`, `keyboard.c`, `libinput.c`, `seat.c`) contain:
- Standard wlroots input event handling
- Traditional cursor/pointer management  
- Keyboard event processing and bindings
- Libinput device configuration
- Seat focus and capability management

**No SceneFX Integration Found**:
- No `fx_*` function calls
- No blur/shadow/corner configuration options
- No SceneFX renderer integration
- No cosmetic effect-related code additions
- No new config parsing for visual effects

**Rationale**: 
Input handling operates at a lower level than the visual rendering pipeline where SceneFX effects are applied. Cosmetic enhancements like blur, shadows, and rounded corners are implemented in the scene graph and renderer layers, not in input event processing.

The input subsystem remains unchanged between Sway and SwayFX for SceneFX integration purposes.

---

# Part 2/2:

Looking at the Sway input directory files, the changes for SceneFX integration are indeed minimal since input handling is separate from rendering. However, there are a few specific areas where SceneFX integration occurs:

## *seat.c*

**Purpose**: Core seat management and scene graph coordination for input focus

**SceneFX Changes**:

**Scene Graph Integration**: 
- Scene tree allocation for seat-specific effects
- Drag icon scene management with potential effect support

**Key Function Modifications**:
```c
seat_create() {
    // Original: Basic scene tree allocation
    seat->scene_tree = alloc_scene_tree(root->layers.seat, &failed);
    seat->drag_icons = alloc_scene_tree(seat->scene_tree, &failed);
    
    // SwayFX: Same structure but with SceneFX-aware scene trees
    // for potential effect layering on seat elements
}
```

**New SceneFX API Calls**:
- Scene trees created may support effect nodes
- Drag icon positioning retains compatibility with effect-enabled scenes

**Configuration Integration**:
- No direct config changes in input handling
- Scene structure prepared for effect inheritance

## *seatop_move_tiling.c*

**Purpose**: Handle tiling window movement with visual feedback

**SceneFX Changes**:

**Corner Radius Integration**:
```c
static void update_indicator(struct seatop_move_tiling_event *e, struct wlr_box *box) {
    wlr_scene_node_set_position(&e->indicator_rect->node, box->x, box->y);
    wlr_scene_rect_set_size(e->indicator_rect, box->width, box->height);

    // SwayFX addition: Corner radius for move indicators
    int corner_radius = config->corner_radius;
    if (e->con) {
        corner_radius = e->con->corner_radius;
        if (e->target_node && e->target_node->type != N_CONTAINER) {
            corner_radius += e->con->current.border_thickness;
        }
    }
    wlr_scene_rect_set_corner_radius(e->indicator_rect,
        MIN(corner_radius, MIN(box->width / 2, box->height / 2)),
        CORNER_LOCATION_ALL);
}
```

**New SceneFX API Calls**:
- `wlr_scene_rect_set_corner_radius()` - Apply rounded corners to move indicators
- `wlr_scene_rect_set_backdrop_blur()` - Enable blur on move indicators

**Configuration Integration**:
- Access to `config->corner_radius` and per-container corner radius
- Integration with container border thickness calculations

## *seatop_move_tiling.c* (Additional)

**Purpose**: Backdrop blur for move indicators

**SceneFX Changes**:

**Blur Implementation**:
```c
seatop_begin_move_tiling_threshold() {
    // SwayFX: Enable backdrop blur on move indicator
    e->indicator_rect = wlr_scene_rect_create(seat->scene_tree, 0, 0, color);
    if (!e->indicator_rect) {
        free(e);
        return;
    }
    wlr_scene_rect_set_backdrop_blur(e->indicator_rect, true);
}
```

**New SceneFX API Calls**:
- `wlr_scene_rect_set_backdrop_blur()` - Enable blur on drag indicators

## *Other Input Files*

**seatop_default.c, seatop_down.c, seatop_move_floating.c, seatop_resize_floating.c, seatop_resize_tiling.c, switch.c, tablet.c, text_input.c**:

**Purpose**: Standard input handling operations

**SceneFX Changes**: **None** - These files remain functionally identical to Sway as they handle pure input logic without visual effect requirements.

## Summary

The input directory has minimal SceneFX integration, limited to:

1. **Move indicator effects**: Rounded corners and backdrop blur on tiling move indicators
2. **Scene graph compatibility**: Ensuring scene trees support effect inheritance  
3. **Container property access**: Reading corner radius values during operations

The changes are constrained to visual feedback elements during window operations, with core input handling logic remaining unchanged.
