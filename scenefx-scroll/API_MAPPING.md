# SceneFX to Scroll Scene API Mapping

This document maps SceneFX's wlr_scene API calls to Scroll's sway_scene equivalents.

## Core Structure Mappings

### Basic Scene Structures
| SceneFX (wlr_scene) | Scroll (sway_scene) | Notes |
|---------------------|---------------------|-------|
| `struct wlr_scene` | `struct sway_scene` | Root scene structure |
| `struct wlr_scene_tree` | `struct sway_scene_tree` | Scene tree node |
| `struct wlr_scene_node` | `struct sway_scene_node` | Base scene node |
| `struct wlr_scene_buffer` | `struct sway_scene_buffer` | Buffer display node |
| `struct wlr_scene_rect` | `struct sway_scene_rect` | Rectangle display node |
| `struct wlr_scene_surface` | `struct sway_scene_surface` | Surface display node |
| `struct wlr_scene_output` | `struct sway_scene_output` | Output viewport |

### SceneFX Extension Structures (No Direct Scroll Equivalent)
| SceneFX Structure | Scroll Adaptation | Notes |
|-------------------|-------------------|-------|
| `struct wlr_scene_shadow` | `struct sway_scene_shadow` | Shadow node - needs new implementation |
| `struct wlr_scene_optimized_blur` | `struct sway_scene_optimized_blur` | Blur node - needs new implementation |

### Enum Mappings
| SceneFX Enum | Scroll Enum | Notes |
|--------------|-------------|-------|
| `enum wlr_scene_node_type` | `enum sway_scene_node_type` | Node type enumeration |
| `WLR_SCENE_NODE_TREE` | `SWAY_SCENE_NODE_TREE` | Tree node type |
| `WLR_SCENE_NODE_RECT` | `SWAY_SCENE_NODE_RECT` | Rectangle node type |
| `WLR_SCENE_NODE_BUFFER` | `SWAY_SCENE_NODE_BUFFER` | Buffer node type |
| `WLR_SCENE_NODE_SHADOW` | `SWAY_SCENE_NODE_SHADOW` | Shadow node type - NEW |
| `WLR_SCENE_NODE_OPTIMIZED_BLUR` | `SWAY_SCENE_NODE_OPTIMIZED_BLUR` | Blur node type - NEW |

## Function Mappings

### Core Scene Functions
| SceneFX Function | Scroll Function | Notes |
|------------------|-----------------|-------|
| `wlr_scene_create()` | `sway_scene_create()` | Create scene |
| `wlr_scene_node_destroy()` | `sway_scene_node_destroy()` | Destroy node |
| `wlr_scene_node_set_enabled()` | `sway_scene_node_set_enabled()` | Enable/disable node |
| `wlr_scene_node_set_position()` | `sway_scene_node_set_position()` | Set node position |
| `wlr_scene_node_place_above()` | `sway_scene_node_place_above()` | Reorder nodes |
| `wlr_scene_node_place_below()` | `sway_scene_node_place_below()` | Reorder nodes |
| `wlr_scene_node_raise_to_top()` | `sway_scene_node_raise_to_top()` | Reorder nodes |
| `wlr_scene_node_lower_to_bottom()` | `sway_scene_node_lower_to_bottom()` | Reorder nodes |
| `wlr_scene_node_reparent()` | `sway_scene_node_reparent()` | Change parent |
| `wlr_scene_node_coords()` | `sway_scene_node_coords()` | Get coordinates |
| `wlr_scene_node_for_each_buffer()` | `sway_scene_node_for_each_buffer()` | Iterate buffers |
| `wlr_scene_node_at()` | `sway_scene_node_at()` | Find node at position |

### Tree Functions
| SceneFX Function | Scroll Function | Notes |
|------------------|-----------------|-------|
| `wlr_scene_tree_create()` | `sway_scene_tree_create()` | Create tree node |
| `wlr_scene_tree_from_node()` | `sway_scene_tree_from_node()` | Cast to tree |

### Buffer Functions
| SceneFX Function | Scroll Function | Notes |
|------------------|-----------------|-------|
| `wlr_scene_buffer_create()` | `sway_scene_buffer_create()` | Create buffer node |
| `wlr_scene_buffer_from_node()` | `sway_scene_buffer_from_node()` | Cast to buffer |
| `wlr_scene_buffer_set_buffer()` | `sway_scene_buffer_set_buffer()` | Set buffer |
| `wlr_scene_buffer_set_buffer_with_damage()` | `sway_scene_buffer_set_buffer_with_damage()` | Set buffer with damage |
| `wlr_scene_buffer_set_opaque_region()` | `sway_scene_buffer_set_opaque_region()` | Set opaque region |
| `wlr_scene_buffer_set_source_box()` | `sway_scene_buffer_set_source_box()` | Set source box |
| `wlr_scene_buffer_set_dest_size()` | `sway_scene_buffer_set_dest_size()` | Set destination size |
| `wlr_scene_buffer_set_transform()` | `sway_scene_buffer_set_transform()` | Set transform |
| `wlr_scene_buffer_set_opacity()` | `sway_scene_buffer_set_opacity()` | Set opacity |
| `wlr_scene_buffer_set_filter_mode()` | `sway_scene_buffer_set_filter_mode()` | Set filter mode |
| `wlr_scene_buffer_send_frame_done()` | `sway_scene_buffer_send_frame_done()` | Send frame done |

### Rectangle Functions  
| SceneFX Function | Scroll Function | Notes |
|------------------|-----------------|-------|
| `wlr_scene_rect_create()` | `sway_scene_rect_create()` | Create rectangle |
| `wlr_scene_rect_from_node()` | `sway_scene_rect_from_node()` | Cast to rect |
| `wlr_scene_rect_set_size()` | `sway_scene_rect_set_size()` | Set size |
| `wlr_scene_rect_set_color()` | `sway_scene_rect_set_color()` | Set color |

### Surface Functions
| SceneFX Function | Scroll Function | Notes |
|------------------|-----------------|-------|
| `wlr_scene_surface_create()` | `sway_scene_surface_create()` | Create surface node |
| `wlr_scene_surface_try_from_buffer()` | `sway_scene_surface_try_from_buffer()` | Cast to surface |
| `wlr_scene_surface_send_frame_done()` | `sway_scene_surface_send_frame_done()` | Send frame done |

### Output Functions
| SceneFX Function | Scroll Function | Notes |
|------------------|-----------------|-------|
| `wlr_scene_output_create()` | `sway_scene_output_create()` | Create output |
| `wlr_scene_output_destroy()` | `sway_scene_output_destroy()` | Destroy output |
| `wlr_scene_output_set_position()` | `sway_scene_output_set_position()` | Set position |
| `wlr_scene_output_commit()` | `sway_scene_output_commit()` | Commit output |
| `wlr_scene_output_build_state()` | `sway_scene_output_build_state()` | Build state |
| `wlr_scene_output_send_frame_done()` | `sway_scene_output_send_frame_done()` | Send frame done |
| `wlr_scene_output_for_each_buffer()` | `sway_scene_output_for_each_buffer()` | Iterate buffers |
| `wlr_scene_get_scene_output()` | `sway_scene_get_scene_output()` | Get scene output |

### Helper Functions
| SceneFX Function | Scroll Function | Notes |
|------------------|-----------------|-------|
| `wlr_scene_set_linux_dmabuf_v1()` | `sway_scene_set_linux_dmabuf_v1()` | Set dmabuf |
| `wlr_scene_set_gamma_control_manager_v1()` | `sway_scene_set_gamma_control_manager_v1()` | Set gamma control |
| `wlr_scene_attach_output_layout()` | `sway_scene_attach_output_layout()` | Attach layout |
| `wlr_scene_subsurface_tree_create()` | `sway_scene_subsurface_tree_create()` | Create subsurface tree |
| `wlr_scene_xdg_surface_create()` | `sway_scene_xdg_surface_create()` | Create XDG surface |
| `wlr_scene_layer_surface_v1_create()` | `sway_scene_layer_surface_v1_create()` | Create layer surface |
| `wlr_scene_drag_icon_create()` | `sway_scene_drag_icon_create()` | Create drag icon |

### SceneFX-Specific Functions (Need New Implementation)
| SceneFX Function | Scroll Adaptation | Notes |
|------------------|-------------------|-------|
| `wlr_scene_set_blur_data()` | `sway_scene_set_blur_data()` | Set blur parameters - NEW |
| `wlr_scene_set_blur_num_passes()` | `sway_scene_set_blur_num_passes()` | Set blur passes - NEW |
| `wlr_scene_set_blur_radius()` | `sway_scene_set_blur_radius()` | Set blur radius - NEW |
| `wlr_scene_set_blur_noise()` | `sway_scene_set_blur_noise()` | Set blur noise - NEW |
| `wlr_scene_set_blur_brightness()` | `sway_scene_set_blur_brightness()` | Set blur brightness - NEW |
| `wlr_scene_set_blur_contrast()` | `sway_scene_set_blur_contrast()` | Set blur contrast - NEW |
| `wlr_scene_set_blur_saturation()` | `sway_scene_set_blur_saturation()` | Set blur saturation - NEW |
| `wlr_scene_shadow_from_node()` | `sway_scene_shadow_from_node()` | Cast to shadow - NEW |
| `wlr_scene_optimized_blur_from_node()` | `sway_scene_optimized_blur_from_node()` | Cast to blur - NEW |

### Type Definitions
| SceneFX Type | Scroll Type | Notes |
|--------------|-------------|-------|
| `wlr_scene_buffer_point_accepts_input_func_t` | `sway_scene_buffer_point_accepts_input_func_t` | Input function type |
| `wlr_scene_buffer_iterator_func_t` | `sway_scene_buffer_iterator_func_t` | Iterator function type |

## Header File Mappings

### Include Changes
| SceneFX Include | Scroll Include | Notes |
|-----------------|----------------|-------|
| `#include <wlr/types/wlr_scene.h>` | `#include <sway/tree/scene.h>` | Main scene header |
| `#include <scenefx/types/wlr_scene.h>` | `#include <scenefx/types/sway_scene.h>` | SceneFX scene header |

## Key Differences

### Additional Fields in Scroll
- `struct sway_scene_node_info` - Contains scale, output, workspace info
- `struct sway_scene_node.old_parent` - For fullscreen focusing
- Extended info tracking for Scroll's scrolling functionality

### SceneFX Extensions to Preserve
- Shadow nodes with blur_sigma, corner_radius
- Optimized blur nodes for performance
- Backdrop blur capabilities on buffers and rectangles
- Corner radius support
- Extended blur parameters (noise, brightness, contrast, saturation)

### Implementation Strategy
1. Replace all `wlr_scene` prefixes with `sway_scene`
2. Update header includes
3. Add SceneFX-specific node types to sway_scene enum
4. Implement SceneFX-specific functions for sway_scene
5. Preserve all SceneFX blur and shadow functionality
6. Test compilation and functionality

## Files Requiring Conversion
- `include/scenefx/types/wlr_scene.h` → `include/scenefx/types/sway_scene.h`
- `include/types/wlr_scene.h` → `include/types/sway_scene.h`
- `types/scene/wlr_scene.c` → `types/scene/sway_scene.c`
- `examples/scene-graph.c` - Update includes and function calls
- `tinywl/tinywl.c` - Update includes and function calls